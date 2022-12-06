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
struct wl_buffer*     puskuri;

const char kirjaimet[] = "selonita";
int kirj_ind[sizeof(kirjaimet)-1];

int jatkakoon = 1, xres=300, yres=300, muuttui;
int kuvan_koko; // const paitsi funktiossa kiinnitä_kuva
const int hmin = 36, wmin = 36;
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

#define jos(str) else if(!strcmp(interface, str))
#define sido(a,b,c) } jos(a##_interface.name) { b = wl_registry_bind(reg, id, &a##_interface, c)
static void registry_handler(void* data, struct wl_registry* reg, uint32_t id,
	const char* interface, uint32_t version) {
    //printf("registry_handler %s, %u\n", interface, id);
    if(0) { // Tämä on supermegajättiovela makro. Nämä käydään, vaikka näyttävät olevan if(0):n sisällä.
	sido(wl_compositor, kokoaja, 4);
	sido(xdg_wm_base, xdg_base, 1);
	xdg_wm_base_add_listener(xdg_base, &xdg_base_lis, NULL);
	sido(wl_shm, wlshm, 1);
    }
}
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

int main() {
    assert((wl = wl_display_connect(NULL)));
    assert((wlreg = wl_display_get_registry(wl)));
    wl_registry_add_listener(wlreg, &reg_listener, NULL);
    wl_display_dispatch(wl);
    wl_display_roundtrip(wl);
    vaadi(wl, "wl", kokoaja, "kokoaja", wlshm, "wl_shm", xdg_base, "xdg_wm_base", NULL);

    assert(( surface = wl_compositor_create_surface(kokoaja)          ));
    assert(( xdgsurf = xdg_wm_base_get_xdg_surface(xdg_base, surface) ));
    xdg_surface_add_listener(xdgsurf, &xdgsurflistener, NULL);
    assert(( xdgtop  = xdg_surface_get_toplevel(xdgsurf)              ));
    xdg_toplevel_add_listener(xdgtop, &xdgtoplistener, NULL);
    wl_surface_commit(surface);
    xdg_toplevel_set_fullscreen(xdgtop, NULL);

    alusta_teksti();

    while (wl_display_dispatch(wl) > 0 && jatkakoon) { // tämä kutsunee kuuntelijat ja tekee poll-asian
	usleep(1000000/30);
	piirrä();
	wl_surface_damage_buffer(surface, 0, 0, xres, yres);
	wl_surface_attach(surface, puskuri, 0, 0); // tämä aina vapautuu automaattisesti
	wl_surface_commit(surface);
    }

    vapauta_teksti();
    if(kuva) { munmap(kuva, kuvan_koko); kuva = NULL; }

    wl_buffer_destroy(puskuri); puskuri=NULL;
    xdg_toplevel_destroy(xdgtop); xdgtop=NULL;
    xdg_surface_destroy(xdgsurf); xdgsurf=NULL;
    wl_surface_destroy(surface); surface=NULL;

    wl_shm_destroy(wlshm); wlshm=NULL;
    xdg_wm_base_destroy(xdg_base); xdg_base=NULL;
    wl_compositor_destroy(kokoaja); kokoaja=NULL;
    wl_registry_destroy(wlreg);
    wl_display_disconnect(wl);
}
