src = xdg-shell.c
otsake = ${src:.c=.h}
xdg = /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
libs = -lwayland-client -lwlroots -lutf8proc

all: main.out

xdg-shell.h: $(xdg)
	wayland-scanner client-header $< $@

xdg-shell.c: $(xdg)
	wayland-scanner private-code $< $@

wlr-output-management-unstable-v1.c:
	wayland-scanner private-code ${@:.c=.xml} $@

wlr-output-management-unstable-v1.h:
	wayland-scanner client-header ${@:.h=.xml} $@

main.out: main.c piirtäjä.h xdg-shell.c xdg-shell.h
	gcc -Wall -g -o $@ main.c ${src} ${libs} `pkg-config --cflags --libs freetype2`
