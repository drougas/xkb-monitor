PKG_CONFIG ?= pkg-config
LIBS = wayland-client xkbcommon xkbregistry

CFLAGS ?= -Wall
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(LIBS))
LDLIBS = $(shell $(PKG_CONFIG) --libs $(LIBS))

SRCS = xkb-monitor.c layout-registry.c
OBJS = $(SRCS:.c=.o)

.PHONY: all debug release clean

all: release

debug: CFLAGS += -g3 -O0 -fno-omit-frame-pointer -fsanitize=address
debug: xkb-monitor

release: CFLAGS += -O3 -DNDEBUG
release: xkb-monitor
	strip xkb-monitor

xkb-monitor: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f xkb-monitor $(OBJS)
