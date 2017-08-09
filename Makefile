
CFLAGS= 
COPT= -O2 -fomit-frame-pointer -ffast-math -std=gnu11
CWARN= -Wall -Wextra -Wredundant-decls
CWARNIGNORE= -Wno-unused-result -Wno-strict-aliasing
CINCLUDE= -Isrc/ -I/usr/include/luajit-2.0/
CDEF=

#ifdef debug
CFLAGS+= -O0 -g -Wno-format -fno-omit-frame-pointer
CDEF+= -DDEBUG -DZEQ_LOG_DUMP_ALL_TO_STDOUT -DZEQ_UDP_DUMP_PACKETS
#else
#CFLAGS+= -DNDEBUG
#endif

_OBJECTS=               \
 ack_mgr                \
 aligned                \
 bit                    \
 buffer                 \
 char_select_client     \
 char_select_thread     \
 class_id               \
 client                 \
 client_mgr             \
 client_packet_recv     \
 client_packet_send     \
 crc                    \
 db_read                \
 db_thread              \
 db_write               \
 deity_id               \
 enum2str               \
 gender_id              \
 hash                   \
 inventory              \
 item_proto             \
 log_thread             \
 login_client           \
 login_crypto           \
 login_thread           \
 main                   \
 main_thread            \
 main_timers            \
 mob                    \
 packet_create          \
 packet_static          \
 race_id                \
 ringbuf                \
 skills                 \
 spellbook              \
 timer                  \
 tlg_packet             \
 udp_client             \
 udp_thread             \
 util_atomic_posix      \
 util_clock_common      \
 util_clock_posix       \
 util_hash_tbl          \
 util_ipv4              \
 util_lua               \
 util_random            \
 util_semaphore_posix   \
 util_socket_lib        \
 util_str               \
 util_thread_posix      \
 zone                   \
 zone_id                \
 zone_mgr               \
 zone_thread

OBJECTS= $(patsubst %,build/%.o,$(_OBJECTS))

##############################################################################
# Core Linker flags
##############################################################################
LFLAGS= -rdynamic
LDYNAMIC= -pthread -lrt -lm -lz -lcrypto -lsqlite3 -lluajit-5.1
LSTATIC= 

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

amalg: main-build-amalg

pre-build:
	$(E) "Generating code..."
	$(Q)luajit codegen/enum2str.lua
    
main-build: pre-build
	$(Q)$(MAKE) --no-print-directory zeq-server-trilogy
    
pre-build-amalg: pre-build
	$(E) "Generating amalgamated source file..."
	$(Q)luajit codegen/amalg.lua "codegen/amalg-zeq-server-trilogy.c" $(_OBJECTS)
    
main-build-amalg: pre-build-amalg
	$(Q)$(MAKE) --no-print-directory amalg-zeq-server-trilogy

amalg-zeq-server-trilogy:
	$(E) "\e[0;32mCC     codegen/amalg-zeq-server-trilogy.c\e(B\e[m"
	$(Q)$(CC) -o bin/zeq-server-trilogy codegen/amalg-zeq-server-trilogy.c $(CDEF) $(COPT) $(CWARN) $(CWARNIGNORE) $(CFLAGS) $(CINCLUDE) $(LSTATIC) $(LDYNAMIC) $(LFLAGS)

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
