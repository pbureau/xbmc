ARCH=i486-linux

.PHONY: compile

DIRS=libcmyth librefmem

CMYTH_SYSDIR=/home/pbureau/Software/xbmc/system
CMYTH_SO=libcmyth-$(ARCH).so
CMYTH_LIB=$(CMYTH_SYSDIR)/$(CMYTH_SO)

all: $(CMYTH_LIB)

ifeq ($(findstring osx,$(ARCH)), osx)
$(CMYTH_LIB): libcmyth/libcmyth.a librefmem/librefmem.a
	$(CC) $(LDFLAGS) -bundle -o $@ -L/usr/lib/i386-linux-gnu -lmysqlclient -lpthread -lz -lm -lrt -ldl \
	-Wl,-all_load libcmyth/libcmyth.a librefmem/librefmem.a 
else
$(CMYTH_LIB): libcmyth/libcmyth.a librefmem/librefmem.a
	$(CC) $(LDFLAGS) -shared -fpic -o $@ -Wl,--whole-archive \
	libcmyth/libcmyth.a librefmem/librefmem.a -Wl,--no-whole-archive -L/usr/lib/i386-linux-gnu -lmysqlclient -lpthread -lz -lm -lrt -ldl
endif

libcmyth/libcmyth.a: compile
	$(MAKE) -C libcmyth

librefmem/librefmem.a: compile
	$(MAKE) -C librefmem


CLEAN_FILES=$(CMYTH_LIB)

include /home/pbureau/Software/xbmc/Makefile.include
