APPNAME = wipetest
VERSION = 0.1a
CFLAGS = -Wall -Wextra -Wconversion -O3
BINDIR = /usr/bin

wiptetest: main.c
	gcc ${CFLAGS} -DVERSION=\"${VERSION}\" -DAPPNAME=\"${APPNAME}\" -o ${APPNAME} main.c 

install:
	install -m 755 -s ${APPNAME} ${BINDIR}

clean:
	rm -f vgcore* *.o ${APPNAME}
