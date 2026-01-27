CFLAGS ?= -Wall
LDLIBS = -lwayland-client -lxkbcommon -lxkbregistry

SRCS = xkb-monitor.c layout_registry.c
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
