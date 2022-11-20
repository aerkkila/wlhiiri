#include <sys/mman.h>
#include <fcntl.h>

static void wl_buffer_release(void* data, struct wl_buffer* buf) {
    munmap(kuva, xpit*ypit*4);
    kuva = NULL;
    wl_buffer_destroy(buf);
}
static const struct wl_buffer_listener wl_buf_lis = {.release=wl_buffer_release};

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
    int koko = xpit*ypit*4;
    int fd = alusta_shm_file(koko);
    assert(fd >= 0);
    kuva = mmap(NULL, koko, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(kuva == MAP_FAILED) err(1, "mmap");

    struct wl_shm_pool* pool  = wl_shm_create_pool(wlshm, fd, koko);
    struct wl_buffer* puskuri = wl_shm_pool_create_buffer(pool, 0, xpit, ypit, xpit*4, WL_SHM_FORMAT_RGBA8888);
    wl_shm_pool_destroy(pool);
    close(fd);
    wl_buffer_add_listener(puskuri, &wl_buf_lis, NULL);
    return puskuri;
}

static void piirrä(unsigned char* data) {
    int pit = xpit*ypit;
    for(int i=0; i<pit; i++) {
	int arvo = rand() & 0xff;
	data[i] = arvo;
	data[i+1] = arvo;
	data[i+2] = arvo;
	data[i+3] = 0xff;
    }
}
