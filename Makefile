
CFLAGS= 
COPT= -O2 -fomit-frame-pointer -ffast-math -std=gnu11
CWARN= -Wall -Wextra -Wredundant-decls
CWARNIGNORE= -Wno-unused-result -Wno-strict-aliasing
CINCLUDE= -Isrc/
CDEF=

#ifdef debug
CFLAGS+= -O0 -g -Wno-format -fno-omit-frame-pointer
CDEF+= -DDEBUG
CINCLUDE+= -I/usr/include/lua5.1/
#else
#CFLAGS+= -DNDEBUG
#CINCLUDE+= -I/usr/include/luajit-2.0/
#endif

_OBJECTS=               \
 ack_mgr                \
 aligned                \
 bit                    \
 buffer                 \
 crc                    \
 db_read                \
 db_thread              \
 db_write               \
 enum2str               \
 log_thread             \
 login_client           \
 login_crypto           \
 login_thread           \
 main                   \
 main_thread            \
 ringbuf                \
 timer                  \
 tlg_packet             \
 udp_client             \
 udp_thread             \
 util_atomic_posix      \
 util_clock_posix       \
 util_ipv4              \
 util_random            \
 util_semaphore_posix   \
 util_str               \
 util_thread_posix

OBJECTS= $(patsubst %,build/%.o,$(_OBJECTS))

##############################################################################
# Core Linker flags
##############################################################################
LFLAGS= 
LDYNAMIC= -pthread -lrt -lm -lcrypto -lsqlite3
LSTATIC= 

#ifdef debug
LDYNAMIC+= -llua5.1
#else
#LDYNAMIC+= -lluajit-5.1
#endif

##############################################################################
# Util
##############################################################################
Q= @
E= @echo -e
RM= rm -f 

##############################################################################
# Build rules
##############################################################################
.PHONY: default all clean

default all: main-build

pre-build:
	$(E) "Generating code..."
	$(Q)luajit codegen/enum2str.lua
    
main-build: pre-build
	$(Q)$(MAKE) --no-print-directory zeq-server-trilogy

amalg: amalg-zeq-server-trilogy

amalg-zeq-server-trilogy:
	$(E) "Generating amalgamated source file"
	$(Q)luajit amalg/amalg.lua "amalg/amalg-zeq-server-trilogy.c" $(_OBJECTS)
	$(E) "Building amalg/amalg-zeq-client.c"
	$(Q)$(CC) -o bin/zeq-client amalg/amalg-zeq-server-trilogy.c $(CDEF) $(COPT) $(CWARN) $(CWARNIGNORE) $(CFLAGS) $(CINCLUDE) $(LSTATIC) $(LDYNAMIC) $(LFLAGS)

zeq-server-trilogy: bin/zeq-server-trilogy

bin/zeq-server-trilogy: $(OBJECTS)
	$(E) "Linking $@"
	$(Q)$(CC) -o $@ $^ $(LSTATIC) $(LDYNAMIC) $(LFLAGS)

build/%.o: src/%.c $($(CC) -M src/%.c)
	$(E) "\e[0;32mCC     $@\e(B\e[m"
	$(Q)$(CC) -c -o $@ $< $(CDEF) $(COPT) $(CWARN) $(CWARNIGNORE) $(CFLAGS) $(CINCLUDE)

clean:
	$(Q)$(RM) build/*.o
	$(E) "Cleaned build directory"
