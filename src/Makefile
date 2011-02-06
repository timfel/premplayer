CC=gcc
STAGING=
ifeq ($(XCOMPILE),false)
	STAGING = /media/Data/preware/emu-compile/
endif
CFLAGS=-std=gnu99 -I$(STAGING)/usr/include -I$(STAGING)/usr/include/SDL -I$(STAGING)/usr/local/include -I$(STAGING)/usr/local/include/SDL
LDFLAGS=-L$(STAGING)/usr/lib -L$(STAGING)/usr/local/lib -lpdl -lcjson -lSDL -Wl,-rpath-link=$(STAGING)/usr/lib -Wl,-rpath-link=$(STAGING)/usr/local/lib
ifeq ($(XCOMPILE),false)
	CFLAGS += -m32 -march=i386
endif

premplayer:
