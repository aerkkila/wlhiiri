#include <wayland-client.h>
#include "xdg-shell.h"
//#include "wlr-layer-shell-unstable-v1.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <sys/time.h>

/* saatavat */
struct wl_display*    wl;
struct wl_registry*   wlreg;
struct wl_compositor* kokoaja;
struct xdg_wm_base*   xdg_base;
struct wl_shm*        wlshm;
/* luotavat */
struct wl_surface*    surface;
struct xdg_surface*   xdgsurf;
struct xdg_toplevel*  xdgtop;
//struct zwlr_layer_shell_v1* wlr_layer_shell;

int xpit=700, ypit=400;
unsigned char* kuva;

#include "piirtäjä.h"

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

#define jos(str) else if(!strcmp(interface, str))
#define sido(a,b) jos(a##_interface.name) b = wl_registry_bind(reg, id, &a##_interface, 1)
void registry_handler(void* data, struct wl_registry* reg, uint32_t id,
		      const char* interface, uint32_t version) {
    //printf("registry_handler %s, %u\n", interface, id);
    if(0);
    sido(wl_compositor, kokoaja);
    sido(xdg_wm_base, xdg_base);
    sido(wl_shm, wlshm);
    //sido(zwlr_layer_shell_v1, wlr_layer_shell);
}
#undef sido
#undef jos
void registry_poistaja(void* data, struct wl_registry* reg, uint32_t id) {
    printf("poistaja %i\n", id);
    wl_registry_destroy(wlreg);
    wlreg = NULL;
}
static struct wl_registry_listener reg_listener = {
    registry_handler,
    registry_poistaja,
};

void xdgconfigure(void* data, struct xdg_surface* surf, uint32_t serial) {
    xdg_surface_ack_configure(surf, serial);
    struct wl_buffer* puskuri = kiinnitä_kuva(); // voisiko tehdä pääfunktiossa
    piirrä(kuva);
    wl_surface_attach(surface, puskuri, 0, 0);
    wl_surface_commit(surface);
}
static struct xdg_surface_listener xdgsurflistener = { xdgconfigure };

static void xdg_wm_base_ping(void* data, struct xdg_wm_base *xwb, uint32_t ser) {
    xdg_wm_base_pong(xwb, ser);
}
static struct xdg_wm_base_listener xdg_base_lis = {
    .ping = xdg_wm_base_ping,
};

int main() {
    assert((wl = wl_display_connect(NULL)));
    assert((wlreg = wl_display_get_registry(wl)));
    wl_registry_add_listener(wlreg, &reg_listener, NULL); // null voisi olla tietue kaikesta
    // wl_display_dispatch(wl);
    wl_display_roundtrip(wl);
    vaadi(wl, "wl", kokoaja, "kokoaja", wlshm, "wl_shm", NULL);
    xdg_wm_base_add_listener(xdg_base, &xdg_base_lis, NULL);

    assert(( surface = wl_compositor_create_surface(kokoaja)          ));
    assert(( xdgsurf = xdg_wm_base_get_xdg_surface(xdg_base, surface) ));
    xdg_surface_add_listener(xdgsurf, &xdgsurflistener, NULL); // pitääkö tämä olla ennen toplevelia?
    assert(( xdgtop  = xdg_surface_get_toplevel(xdgsurf)              ));
    wl_surface_commit(surface);
    wl_display_roundtrip(wl);

    for(int i=0; i<20; i++) {
	wl_display_dispatch_pending(wl);
	wl_display_flush(wl);
	usleep(100000);
    }

    xdg_toplevel_destroy(xdgtop); xdgtop = NULL;
    wl_registry_destroy(wlreg);
    wl_display_disconnect(wl);
}
