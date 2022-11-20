src = wlr-layer-shell-unstable-v1.c xdg-shell.c
obj = ${src:.c=.o} main.o
xdg = /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
wlr = ../ohjelmat/dwl/protocols/wlr-layer-shell-unstable-v1.xml

all: pää.out

xdg-shell.h: $(xdg)
	wayland-scanner client-header $< $@

xdg-shell.c: $(xdg)
	wayland-scanner private-code $< $@

wlr-layer-shell-unstable-v1.h: $(wlr) 
	wayland-scanner client-header $< $@

wlr-layer-shell-unstable-v1.c: $(wlr) 
	wayland-scanner private-code $< $@

%.o: %.c
	gcc -c $< -g -Wall

pää.o: main.c ${src} piirtäjä.h
	gcc -c $< -g -Wall

pää.out: xdg-shell.h piirtäjä.h $(obj)
	gcc -o $@ $(obj) -lwayland-client -lwlroots -lrt

testi.out: testi.c xdg-shell.c xdg-shell.h
	gcc -o $@ $< xdg-shell.c -lwayland-client -lwlroots -lrt 
