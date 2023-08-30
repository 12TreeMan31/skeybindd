CC=clang
CFLAGS= -Wall -Werror
PREFIX = /usr

build: main.c
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