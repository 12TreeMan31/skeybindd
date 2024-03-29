CC=clang
CFLAGS= -Wall -Werror
PREFIX = /usr

config.h:
	cp config.def.h $@
	
build: main.c config.h
	$(CC) $(CFLAGS) -o skeybindd main.c `pkg-config --cflags --libs libevdev`

install: build
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f skeybindd ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/skeybindd

systemd:
	cp skeybindd.service /etc/systemd/user/
	systemctl daemon-reload

clean:
	rm skeybindd