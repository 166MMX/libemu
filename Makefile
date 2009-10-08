# Settings via env variables
#  CC          - Your C compiler
#  AR          - Your AR archiver
#  CFLAGS      - Your CFLAGS (we add a few we find essential)
#  LDFLAGS     - Your LDFLAGS (we add a few we find essential)
#  SDL_INCLUDE - Where to find SDL includes
#  LIBS        - Which libs to include (SDL.dll for windows)
#
# Settings via parameters
#  WIN32:=1    - To compile a .dll

LDFLAGS := $(LDFLAGS) -shared
ifdef WIN32
LIB_EXTENSION := .dll
CFLAGS := -DWIN32
else
LIB_EXTENSION := .so
CFLAGS := $(CFLAGS) -fPIC
endif
ifndef SDL_INCLUDE
SDL_INCLUDE := /usr/include/SDL
endif
ifndef LIBS
LIBS :=
endif

CFLAGS := $(CFLAGS) -g -Wall -Wextra -ansi -pedantic -Wno-unused-parameter -Werror
LDFLAGS := $(LDFLAGS) -g

SOURCE := $(shell ls src/*.c src/*/*.c 2>/dev/null)
SOURCE := $(SOURCE:%.c=objs/%.o)
RES := $(shell mkdir -p objs/src)

ifdef VERBOSE
	Q =
else
	Q = @
endif

all: libemu$(LIB_EXTENSION) libemu.a

objs/%.o: %.c
	$(shell mkdir -p `dirname $@`)
	@echo "[Compiling] $^"
	$(Q)$(CC) $(CFLAGS) -c $^ -o $@ -I include/ -I $(SDL_INCLUDE)

libemu$(LIB_EXTENSION): $(SOURCE)
	@echo "[Dynamic] $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

libemu.a: $(SOURCE)
	@echo "[Static] $@"
	$(Q)$(AR) rcs $@ $(SOURCE)

clean:
	@echo "[Cleaning] libemu"
	$(Q)rm -f libemu$(LIB_EXTENSION) libemu.a $(SOURCE)

