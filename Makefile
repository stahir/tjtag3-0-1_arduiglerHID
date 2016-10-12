#OS = LINUX
#OS = MACOSX
OS = WINDOWS

CFLAGS += -Wall -O2 
CC = x86_64-w64-mingw32-gcc
LIBS = -lhid -lsetupapi

WRT54GMEMOBJS := hid.o tjtag.o jt_mods.o

all: tjtag

tjtag : $(WRT54GMEMOBJS)
	$(CC) $(CFLAGS) -o $@ $(WRT54GMEMOBJS) $(LIBS)

hid.o: hid_$(OS).c hid.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o tjtag test
