#include <wayland-client.h>
#include "xdg-shell.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <linux/uinput.h>
#include <pthread.h>

/* saatavat */
struct wl_display*    wl;
struct wl_registry*   wlreg;
struct wl_compositor* kokoaja;
struct xdg_wm_base*   xdg_base;
struct wl_shm*        wlshm;
struct wl_seat*       istuin;
/* luotavat */
struct wl_surface*    surface;
struct xdg_surface*   xdgsurf;
struct xdg_toplevel*  xdgtop;
struct wl_buffer*     puskuri;
struct wl_callback*   framekutsuja;

int saa_piirtää, xres=300, yres=300, muuttui, hiiri_fd, verbose;
volatile int jatkakoon = 1;
int xhila=22, yhila=20, xyhila;
const char** sanat;
int kuvan_koko; // const paitsi funktiossa kiinnitä_kuva
const int hmin = 36, wmin = 36;
unsigned char* kuva;
int piirrä_uudesti = 1;

const char* sanatiedosto = "/usr/local/share/wlhiiri/sanat";

void* hiiri(void*);
void alusta_hiiri(int fd);
void nop() {}

#include "piirtäjä.h"
#include "syöte.h"

void vaadi(void* a, ...) {
    va_list vl;
    char* b;
    int kelpaa = 1;
    va_start(vl, a);
    do {
	b = va_arg(vl, char*);
	if(!a) {
	    fprintf(stderr, "%s puuttuu\n", b);
	    kelpaa = 0;
	}
    } while((a=va_arg(vl, void*)));
    va_end(vl);
    if(kelpaa) return;
    abort();
}
//static void wl_buffer_release(void* data, struct wl_buffer* buf) { }
//static const struct wl_buffer_listener wl_buf_lis = {.release=wl_buffer_release};

static void xdgconfigure(void* data, struct xdg_surface* surf, uint32_t serial) {
    xdg_surface_ack_configure(surf, serial);
    if(muuttui && puskuri) {
	wl_buffer_destroy(puskuri);
	puskuri = NULL;
    }
    if(!puskuri)
	puskuri = kiinnitä_kuva();
    //wl_buffer_add_listener(puskuri, &wl_buf_lis, NULL);
    piirrä();
    wl_surface_attach(surface, puskuri, 0, 0);
    wl_surface_commit(surface);
}
static struct xdg_surface_listener xdgsurflistener = {
    .configure = xdgconfigure,
};

static void xdg_wm_base_ping(void* data, struct xdg_wm_base *xwb, uint32_t ser) {
    xdg_wm_base_pong(xwb, ser);
}
static struct xdg_wm_base_listener xdg_base_lis = {
    .ping = xdg_wm_base_ping,
};

static void topconfigure(void* data, struct xdg_toplevel* top, int32_t w, int32_t h, struct wl_array* states) {
    muuttui = xres != w || yres != h;
    xres = w<wmin? wmin: w;
    yres = h<hmin? hmin: h; 
}
static void xdgclose(void* data, struct xdg_toplevel* top) {
    jatkakoon = 0;
}
static struct xdg_toplevel_listener xdgtoplistener = {
    .configure = topconfigure,
    .close = xdgclose,
};

#define sitomiset if(0)
#define jos(str) else if(!strcmp(interface, str))
#define sido(a,b,c) } jos(a##_interface.name) { b = wl_registry_bind(reg, id, &a##_interface, c)
static void registry_handler(void* data, struct wl_registry* reg, uint32_t id,
			     const char* interface, uint32_t version) {
    //printf("registry_handler %s, %u\n", interface, id);
    sitomiset { 
	sido(wl_compositor, kokoaja, 4);
	sido(xdg_wm_base, xdg_base, 1);
	xdg_wm_base_add_listener(xdg_base, &xdg_base_lis, NULL);
	sido(wl_shm, wlshm, 1);
	sido(wl_seat, istuin, 8);
    }
}
#undef sitomiset
#undef sido
#undef jos
static void registry_poistaja(void* data, struct wl_registry* reg, uint32_t id) {
    printf("poistaja %i\n", id);
    wl_registry_destroy(wlreg);
    wlreg = NULL;
}
static struct wl_registry_listener reg_listener = {
    .global = registry_handler,
    .global_remove = registry_poistaja,
};

static void framekutsu(void*, struct wl_callback*, uint32_t);
static const struct wl_callback_listener frame_listener = { framekutsu };
static void framekutsu(void* data, struct wl_callback* kutsu, uint32_t aika) {
    wl_callback_destroy(framekutsuja); framekutsuja=NULL;
    framekutsuja = wl_surface_frame(surface);
    wl_callback_add_listener(framekutsuja, &frame_listener, NULL);
    saa_piirtää = 1;
}

void hiiri_laita(int fd, int tyyppi, int koodi, int arvo) {
    struct input_event ie = {
	.type  = tyyppi,
	.code  = koodi,
	.value = arvo,
    };
    if(write(fd, &ie, sizeof(ie)) < sizeof(ie))
	err(1, "write rivillä %i", __LINE__);
}

int main(int argc, char** argv) {
    if((hiiri_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
	err(1, "open(/dev/uinput)");
    /* Luovutaan pääkäyttäjän oikeuksista. */
    seteuid(getuid());
    setegid(getgid());

    verbose = (argc > 1);
    xyhila = xhila*yhila;
    sanat = malloc(xyhila*sizeof(void*));
    struct stat stat;
    int fd = open(sanatiedosto, O_RDONLY);
    if (fd < 0) err(1, "open \"%s\"", sanatiedosto);
    fstat(fd, &stat);
    size_t sanatkoko = stat.st_size;
    char* tied = mmap(NULL, sanatkoko, PROT_READ, MAP_PRIVATE, fd, 0);
    const char* ptr = tied;
    for(int i=0;; i++) {
	sanat[i] = ptr;
	if(i==xyhila-1) break;
	while(*ptr++);
    }

    assert((wl = wl_display_connect(NULL)));
    assert((wlreg = wl_display_get_registry(wl)));
    wl_registry_add_listener(wlreg, &reg_listener, NULL);
    wl_display_dispatch(wl);
    wl_display_roundtrip(wl);
    vaadi(wl, "wl", kokoaja, "kokoaja", wlshm, "wl_shm", xdg_base, "xdg_wm_base", istuin, "wl_seat", NULL);

    assert(( surface = wl_compositor_create_surface(kokoaja)          ));
    framekutsuja = wl_surface_frame(surface);
    wl_callback_add_listener(framekutsuja, &frame_listener, NULL);
    assert(( xdgsurf = xdg_wm_base_get_xdg_surface(xdg_base, surface) ));
    xdg_surface_add_listener(xdgsurf, &xdgsurflistener, NULL);
    assert(( xdgtop  = xdg_surface_get_toplevel(xdgsurf)              ));
    xdg_toplevel_add_listener(xdgtop, &xdgtoplistener, NULL);
    wl_surface_commit(surface);
    xdg_toplevel_set_fullscreen(xdgtop, NULL);

    alusta_teksti();
    alusta_näppäimistö();
    alusta_kursori();

    pthread_t säie;
    alusta_hiiri(hiiri_fd);
    while (jatkakoon && wl_display_dispatch(wl) > 0) { // tämä kutsunee kuuntelijat ja tekee poll-asian
	if (hiireksi_x >= 0) {
	    short yx[] = {hiireksi_y, hiireksi_x};
	    pthread_create(&säie, NULL, hiiri, &yx);
	    hiireksi_x = -1;
	}
	usleep(1000000/50);
	if(!saa_piirtää)
	    continue;
	saa_piirtää = 0;
	puts("saa piirtää");
	if(piirrä_uudesti) {
	    puts("piirretään");
	    piirrä();
	    wl_surface_damage_buffer(surface, 0, 0, xres, yres);
	    wl_surface_attach(surface, puskuri, 0, 0); // tämä aina vapautuu automaattisesti
	    wl_surface_commit(surface);
	    piirrä_uudesti = 0;
	}
    }
    pthread_join(säie, NULL);

    munmap(tied, sanatkoko);
    close(fd);
    free(sanat);
    vapauta_teksti();
    if(kuva) { munmap(kuva, kuvan_koko); kuva = NULL; }

    wl_buffer_destroy(puskuri); puskuri=NULL;
    xdg_toplevel_destroy(xdgtop); xdgtop=NULL;
    xdg_surface_destroy(xdgsurf); xdgsurf=NULL;
    wl_surface_destroy(surface); surface=NULL;
    wl_callback_destroy(framekutsuja); framekutsuja=NULL;

    wl_keyboard_release(näppäimistö); näppäimistö=NULL; // onko turha
    xkb_context_unref(xkbasiayhteys); xkbasiayhteys=NULL;
    xkb_state_unref(xkbtila); xkbtila=NULL;

    kursori = (wl_pointer_release(kursori), NULL);

    wl_seat_destroy(istuin); istuin=NULL;
    wl_shm_destroy(wlshm); wlshm=NULL;
    xdg_wm_base_destroy(xdg_base); xdg_base=NULL;
    wl_compositor_destroy(kokoaja); kokoaja=NULL;
    wl_registry_destroy(wlreg);
    wl_display_disconnect(wl);

    usleep(100000); // Ennen klikkausta odotetaan 0,1 s, että peittävä ikkuna ehtii tuhoutua.
    hiiri_laita(hiiri_fd, EV_KEY, BTN_LEFT, 1);
    hiiri_laita(hiiri_fd, EV_SYN, SYN_REPORT, 0);
    hiiri_laita(hiiri_fd, EV_KEY, BTN_LEFT, 0);
    hiiri_laita(hiiri_fd, EV_SYN, SYN_REPORT, 0);

    close(hiiri_fd);
}

#define Ioctl(...)				\
    do {					\
	if(ioctl(__VA_ARGS__) < 0)		\
	    err(1, "ioctl rivi %i", __LINE__);	\
    } while(0)

void alusta_hiiri(int fd) {
    struct uinput_setup usetup = { .id.bustype = BUS_USB };
    strcpy(usetup.name, "wlhiiren_hiiri");
    Ioctl(fd, UI_SET_EVBIT,  EV_KEY);
    Ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    Ioctl(fd, UI_SET_EVBIT,  EV_REL);
    Ioctl(fd, UI_SET_RELBIT, REL_X);
    Ioctl(fd, UI_SET_RELBIT, REL_Y);
    Ioctl(fd, UI_DEV_SETUP, &usetup);
    Ioctl(fd, UI_DEV_CREATE);
}

const short hitaus = 4;
const short sieto = 1;
int REL_YX[] = {REL_Y, REL_X};
#define abs(i) ((i) < 0 ? -(i) : (i))
#define Printf(...) do{if(verbose)printf(__VA_ARGS__);}while(0)

void* hiiri(void* args) {
    short* yx = args;
    hiiri_laita(hiiri_fd, EV_REL, REL_X, 25); // TODO: saisiko sijainnin suoraan
    hiiri_laita(hiiri_fd, EV_SYN, SYN_REPORT, 0);
    while (hiiren_yx[0] < 0 || hiiren_yx[1] < 0)
	usleep(1000000/2);
    short yx_hitaus[] = {hitaus, hitaus};
    int lasku = 0;
    char valmis = 0;
    while (valmis != 3) {
	if(++lasku > 100) break;
	for(int i=0; i<2; i++) {
	    short siirto = yx[i]-hiiren_yx[i];
	    if (abs(siirto) <= sieto) {
		Printf("\033[4;35m%i: %i\033[0m\n", i, yx[i]);
		valmis |= 1<<i;
		continue; }
	    Printf("%i: %hi --> %hi (\033[s     / %hi) ", i, hiiren_yx[i], yx[i], siirto);
	    if (siirto < yx_hitaus[i]) {
		int yrite = yx_hitaus[i] / 2;
		yx_hitaus[i] = yrite ? yrite : 1;
	    }
	    siirto /= yx_hitaus[i];
	    Printf("\033[u%hi \n", siirto);
	    hiiren_yx[i] = -1;
	    hiiri_laita(hiiri_fd, EV_REL, REL_YX[i], siirto);
	}
	hiiri_laita(hiiri_fd, EV_SYN, SYN_REPORT, 0);

	volatile int* p = (volatile int*)hiiren_yx; // tällä tarkistetaan alla, onko kumpikaan negatiivinen
	while((usleep(1000000/1000), *p | *p<<16) < 0);
    }
    jatkakoon = 0;
    return NULL;
}
