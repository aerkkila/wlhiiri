#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1.h"
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

char _print[256];
char* aprintf(const char* str, ...) {
    va_list valist;
    va_start(valist, str);
    vsprintf(_print, str, valist);
    va_end(valist);
    return _print;
}

struct wl_compositor* kokoaja;
struct wl_surface* surface;
struct zwlr_layer_shell_v1* wlr_layer_shell;

#define jos(str) else if(!strcmp(interface, str))
void registry_handler(void* data, struct wl_registry* reg, uint32_t id,
		      const char* interface, uint32_t version) {
    printf("registry_handler %s, %u\n", interface, id);
    if(0);
    jos("wl_compositor")
	kokoaja = wl_registry_bind(reg, id, &wl_compositor_interface, 1);
    jos("zwlr_layer_shell_v1")
	wlr_layer_shell = wl_registry_bind(reg, id, &zwlr_layer_shell_v1_interface, 1);
}
#undef jos

void registry_poistaja(void* data, struct wl_registry* reg, uint32_t id) {
    printf("poistaja %i\n", id);
}

#define poistu() _poistu(__LINE__)
void __attribute__((cold,noreturn)) _poistu(int rivi) {
    err(1, "rivi %i", rivi);
}

int main() {
    struct wl_display *wl;
    struct wl_registry *wlreg;
    
    if(!(wl = wl_display_connect(aprintf("%s/wayland-0", getenv("XDG_RUNTIME_DIR")))))
	poistu();
    if(!(wlreg = wl_display_get_registry(wl)))
	poistu();
    static const struct wl_registry_listener reg_listener = {
	registry_handler,
	registry_poistaja,
    };
    wl_registry_add_listener(wlreg, &reg_listener, NULL);
    wl_display_dispatch(wl);
    wl_display_roundtrip(wl);
    
    if(!kokoaja || !wlr_layer_shell)
	errx(1, "kokoaja = %p, wlr_layer_shell = %p", kokoaja, wlr_layer_shell);

    surface = wl_compositor_create_surface(kokoaja);
    if(!surface)
	errx(1, "surface");

    for(int i=0; i<20; i++) {
	wl_display_dispatch_pending(wl);
	wl_display_flush(wl);
	usleep(100000);
    }

    wl_registry_destroy(wlreg);
    wl_display_disconnect(wl);
}
