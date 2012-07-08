CC=gcc
AR=ar
LN=ln
INSTALL=/usr/bin/install
TARGET=/usr/local
#CFLAGS=-O -march=athlon-xp -pipe -I/usr/local/include -I./
CFLAGS=-O -pipe -fPIC -I./  -I/usr/include -I/usr/include
DFLAGS=
LFLAGS= -levent -lcrypto -lssl  -L/usr/lib -L/usr/lib

SRC=base64.c conn.c dns.c file.c lms.c mux.c passwords.c rand.c socket.c ssl.c str.c
OBJ=${SRC:%.c=%.o}
ARLIB=libmsocket.a
SOLIB=libmsocket.so

.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) -c $<

all:	$(OBJ)
	$(CC) -shared $(CFLAGS) $(DFLAGS) -o $(SOLIB) $(OBJ)
	$(AR) rcs $(ARLIB) $(OBJ)

build:	all

install:	all
	$(INSTALL) -m 0644 msocket.h $(TARGET)/include/msocket.h
	$(INSTALL) -m 0644 msocketutils.h $(TARGET)/include/msocketutils.h
	$(INSTALL) -m 0644 libmsocket.so $(TARGET)/lib/libmsocket.so.0.1
	$(INSTALL) -m 0644 libmsocket.a $(TARGET)/lib/libmsocket.a
	-$(LN) -s $(TARGET)/lib/libmsocket.so.0.1 $(TARGET)/lib/libmsocket.so

example:
	$(CC) $(CFLAGS) $(DFLAGS) -o lms-ex-server Example-server.c $(LFLAGS) -L./ -lmsocket
	$(CC) $(CFLAGS) $(DFLAGS) -o lms-ex-client Example-client.c $(LFLAGS) -L./ -lmsocket

clean:
	rm -f $(OBJ) $(ARLIB) $(SOLIB)

base64.o:	base64.c msocketutils.h msocket-internal.h
conn.o:		conn.c msocket.h msocket-internal.h
dns.o:		dns.c msocket.h msocket-internal.h
file.o:		file.c msocketutils.h msocket-internal.h
lms.o:		lms.c msocket.h msocketutils.h msocket-internal.h
mux.o:		mux.c msocket.h msocket-internal.h
passwords.o:	passwords.c msocketutils.h msocket-internal.h
rand.o:		rand.c msocket.h msocket-internal.h
socket.o:	socket.c msocket.h msocket-internal.h
ssl.o:		ssl.c msocket.h msocket-internal.h
str.o:		str.c msocketutils.h msocket-internal.h
