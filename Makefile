CC=gcc
CFLAGS= -Wall -Werror -O3
PREFIX = /usr/local

build: main.c
	$(CC) $(CFLAGS) -o skeybindd main.c `pkg-config --cflags --libs libseat libevdev`

install: build
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f skeybindd ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/skeybindd

clean:
	rm skeybindd