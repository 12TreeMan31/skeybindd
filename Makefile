CC=clang
CFLAGS= -Wall -Werror
PREFIX = /usr

config.h:
	cp config.def.h $@
	
build: main.c config.h
	$(CC) -O3 $(CFLAGS) -o skeybindd main.c `pkg-config --cflags --libs libevdev`

debug: main.c config.h
	$(CC) -O0 $(CFLAGS) -o skeybindd main.c `pkg-config --cflags --libs libevdev`

install: build
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f skeybindd ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/skeybindd

skeybindd.service:
	cp skeybindd.def.service $@

systemd: skeybindd.service
	cp skeybindd.service /etc/systemd/user/
	systemctl daemon-reload

clean:
	rm skeybindd