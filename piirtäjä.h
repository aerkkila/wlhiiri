#include <sys/mman.h>
#include <fcntl.h>
#include <ft2build.h>
#include <utf8proc.h>
#include FT_FREETYPE_H
#include "fonttitiedosto.h"

FT_Library library;
FT_Face face;

static int alusta_shm_file(int koko) {
    char nimi[32];
    struct timeval t;
    gettimeofday(&t, NULL);
    sprintf(nimi, "wl_%lx%lx", (long unsigned)t.tv_sec, (long unsigned)t.tv_usec/1000);
    int fd = shm_open(nimi, O_CREAT | O_RDWR, 0600);
    if(fd < 0)
	err(1, "shm_open %s", nimi);
    shm_unlink(nimi); // Tiedosto pysyy, kunnes fd suljetaan.
    if(ftruncate(fd, koko) < 0)
	err(1, "ftruncate %i: %s, koko = %i t", fd, nimi, koko);
    return fd;
}

static struct wl_buffer* kiinnitä_kuva() {
    if(kuva) { munmap(kuva, kuvan_koko); kuva = NULL; }
    kuvan_koko = xres*yres*4;
    int fd = alusta_shm_file(kuvan_koko);
    assert(fd >= 0);
    kuva = mmap(NULL, (size_t)kuvan_koko, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(kuva == MAP_FAILED) err(1, "mmap");
    piirrä_uudesti = 1;

    struct wl_shm_pool* pool  = wl_shm_create_pool(wlshm, fd, kuvan_koko);
    struct wl_buffer* puskuri = wl_shm_pool_create_buffer(pool, 0, xres, yres, xres*4, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
    return puskuri;
}

static void alusta_teksti() {
    assert(!FT_Init_FreeType(&library));
    assert(!FT_New_Face(library, fonttitiedosto, 0, &face));
    assert(!FT_Set_Pixel_Sizes(face, 0, fonttikoko));
}

static void no_niin_ja_laitapas_nyt_sitten_teksti_vaikka_tuohon(const char* teksti, int y, int x) {
    int* ikuva = (int*)kuva;
    int pit = strlen(teksti);
    int32_t merkit[pit];
    if((pit = utf8proc_decompose((const utf8proc_uint8_t*)teksti, pit, merkit, pit, 0)) < 0)
	warn("utf8proc_decompose");

    for(int m=0; m<pit; m++) {
	face->glyph->format = FT_GLYPH_FORMAT_BITMAP;
	int kirj_ind = FT_Get_Char_Index(face, merkit[m]);
	assert(!FT_Load_Glyph(face, kirj_ind, 0));
	assert(!FT_Render_Glyph(face->glyph, 0)); // TODO: pitäisikö siirtää alustukseen
	FT_Bitmap *bitm = &face->glyph->bitmap;
	FT_Glyph_Metrics *metrics = &face->glyph->metrics;
	int bw = bitm->width,
	    bh = bitm->rows,
	    max = xres*yres;
	for(int j=0; j<bh; j++) {
	    for(int i=0; i<bw; i++) {
		unsigned arvo = ((unsigned char*)bitm->buffer)[bw*j + i];
		int kuvaind = xres*(y+j-metrics->horiBearingY/64) + x + i;
		if(kuvaind >= max || kuvaind < 0 || !arvo) continue;
		if(arvo < 255)
		    arvo = arvo*arvo/255;
		ikuva[kuvaind] = (0xff<<24) + (arvo<<16) + (arvo<<8) + arvo;
	    }
	}
	x += metrics->horiAdvance / 64;
    }
}

static void vapauta_teksti() {
    // FT_Done_Face(face);
    FT_Done_FreeType(library); // vapauttaa kaiken muunkin
    face = (FT_Face){0};
    library = (FT_Library){0};
}

static int xkoord(int n) {
    return n%xhila * xres/xhila;
}

/* Tämä päättää, miten tekstit sijoitetaan sarakkeissa. */
static int ykoordfun(int j, int i, int ydiff, int asc) {
    return j*ydiff+asc + (i%2)*0.5*ydiff - (i%2)*(j==yhila-1)*0.4*ydiff;
}

static int ykoord(int n) {
    int asc = face->size->metrics.ascender/64,
	ydiff = (yres-asc)/(yhila-1);
    int j = n/xhila;
    int i = n%xhila;
    return ykoordfun(j, i, ydiff, asc);
}

static void piirrä() {
    for(int i=0; i<kuvan_koko; i+=4)
	*(int*)(kuva+i) = 0xb0000000;

    int asc = face->size->metrics.ascender/64,
	xdiff = xres/xhila,
	ydiff = (yres-asc)/(yhila-1);
    for(int j=0; j<yhila; j++)
	for(int i=0; i<xhila; i++) {
	    int jkoord= ykoordfun(j, i, ydiff, asc);
	    no_niin_ja_laitapas_nyt_sitten_teksti_vaikka_tuohon(sanat[j*xhila+i], jkoord, i*xdiff);
	}
}

static void siirrä_kuvaa_x(int n) {
    kuvan_sij_x += n;
    void* apu = malloc(xres*yres*4);
    memcpy(apu, kuva, xres*yres*4);
    for(int i=0; i<yres; i++)
	memcpy(kuva+(i*xres+n)*4, apu+i*xres*4, (xres-n)*4);
    free(apu);
    wl_surface_damage_buffer(surface, 0, 0, xres, yres);
    wl_surface_attach(surface, puskuri, 0, 0); // tämä aina vapautuu automaattisesti
    wl_surface_commit(surface);
}

static void siirrä_kuvaa_y(int n) {
    kuvan_sij_y += n;
    void* apu = malloc(xres*yres*4);
    memcpy(apu, kuva, xres*yres*4);
    for(int i=0; i<yres-n; i++)
	memcpy(kuva+(n+i)*xres*4, apu+i*xres*4, xres*4);
    free(apu);
    wl_surface_damage_buffer(surface, 0, 0, xres, yres);
    wl_surface_attach(surface, puskuri, 0, 0); // tämä aina vapautuu automaattisesti
    wl_surface_commit(surface);
}
