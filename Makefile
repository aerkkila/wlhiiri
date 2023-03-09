src = xdg-shell.c
xdg = /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
libs = -lwayland-client -lwlroots -lutf8proc -lxkbcommon
prefix = /usr/local

all: main.out

xdg-shell.h: $(xdg)
	wayland-scanner client-header $< $@

xdg-shell.c: $(xdg)
	wayland-scanner private-code $< $@

fonttitiedosto.h:
	printf "static const char* fonttitiedosto = %s;\n" \
		`fc-match -v monospace | sed --silent 's/\W*file:\W*\(\"[^\"]*\"\).*/\1/p'` \
		> fonttitiedosto.h

main.out: main.c piirtäjä.h xdg-shell.c xdg-shell.h syöte.h fonttitiedosto.h
	gcc -Wall -g -o $@ $< ${src} ${libs} `pkg-config --cflags --libs freetype2`

sanat: luo_sanat.pl
	./luo_sanat.pl LICENSE > sanat

install: main.out sanat
	cp $< $(prefix)/bin/wlhiiri
	chmod +s $(prefix)/bin/wlhiiri
	mkdir -p $(prefix)/share/wlhiiri
	cp sanat $(prefix)/share/wlhiiri/

uninstall:
	rm -rf $(prefix)/bin/wlhiiri $(prefix)/share/wlhiiri
