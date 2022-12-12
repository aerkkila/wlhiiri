#include <xkbcommon/xkbcommon.h>

static struct wl_keyboard* näppäimistö;
static struct xkb_context* xkbasiayhteys;
static struct xkb_state* xkbtila;
static struct xkb_keymap* keymap;
static int toistonopeus, toistoviive;

static void kb_modifiers_kutsu(void* data, struct wl_keyboard* wlkb, uint32_t serial, uint32_t mods_depr,
			       uint32_t mods_latch, uint32_t mods_lock, uint32_t group) {
    xkb_state_update_mask(xkbtila, mods_depr, mods_latch, mods_lock, 0, 0, group);
}
static void kb_keymap_kutsu(void* data, struct wl_keyboard* wlkb, uint32_t muoto, int32_t fd, uint32_t koko) {
    char* map_shm = mmap(NULL, koko, PROT_READ, MAP_SHARED, fd, 0);
    if(!map_shm)
	warn("mmap keymap rivillä %i tiedostossa %s", __LINE__, __FILE__);
    //printf("map_shm:\n%*s\n", koko, map_shm);
    keymap = xkb_keymap_new_from_string(xkbasiayhteys, map_shm,
	    				XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    xkbtila = xkb_state_new(keymap);
    munmap(map_shm, koko);
    close(fd);
}
static void kb_key_kutsu(void* data, struct wl_keyboard* wlkb, uint32_t serial,
			 uint32_t aika, uint32_t näpp, uint32_t tila) {
    näpp += 8;
    char puskuri[32];
    xkb_state_key_get_utf8(xkbtila, näpp, puskuri, sizeof(puskuri));
    //printf("'%s', tila = %u\n", puskuri, tila);
}
static void kb_repeat_kutsu(void* data, struct wl_keyboard* wlkb, int32_t nopeus, int32_t viive) {
    toistonopeus = nopeus;
    toistoviive = viive;
}
static struct wl_keyboard_listener näppäimistökuuntelija = {
    .keymap = kb_keymap_kutsu,
    .enter = nop, // pitääkö tässä ottaa, mitkä olivat jo painettuina?
    .leave = nop,
    .key = kb_key_kutsu,
    .modifiers = kb_modifiers_kutsu,
    .repeat_info = kb_repeat_kutsu,
};

static void alusta_näppäimistö() {
    xkbasiayhteys = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    näppäimistö = wl_seat_get_keyboard(istuin);
    wl_keyboard_add_listener(näppäimistö, &näppäimistökuuntelija, NULL);
}
