#include <xkbcommon/xkbcommon.h>

static struct wl_keyboard* näppäimistö;
static struct xkb_context* xkbasiayhteys;
static struct xkb_state* xkbtila;
static struct xkb_keymap* keymap;
static int toistonopeus, toistoviive;

void käsittele_syötetty(char* syöte) {
    for(int i=0; i<xyhila; i++)
	if(!strcmp(sanat[i], syöte)) {
	    hiiri(ykoord(i), xkoord(i));
	    printf("y=%i, x=%i\n", ykoord(i), xkoord(i));
	    return; }
}

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
    static char syöte[64];
    static int isyöte;
    char puskuri[32];
    const xkb_keysym_t* syms_out;
    int ival;
    ival = xkb_state_key_get_syms(xkbtila, näpp, &syms_out);
    if(!tila)
	return;
    if(!xkb_state_key_get_utf8(xkbtila, näpp, puskuri, sizeof(puskuri))) // ei lue esim. nuolinäppäimiä
	;
    for(int i=0; i<ival; i++) {
	switch(syms_out[i]) {
	    case XKB_KEY_BackSpace:
		while(isyöte && (syöte[--isyöte] & (3<<6)) == 3<<6);
		syöte[isyöte] = '\0';
		goto laita;
	    case XKB_KEY_Return:
		käsittele_syötetty(syöte);
		syöte[isyöte=0] = '\0';
		jatkakoon = 0;
		return;
	}
    }
    if(0 <= puskuri[0] && puskuri[0] < ' ')
	return;
    strcpy(syöte+isyöte, puskuri);
    isyöte += strlen(puskuri);
laita:
    //printf("„%s“, tila = %u, syöte = „%s“\n", puskuri, tila, syöte);
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
