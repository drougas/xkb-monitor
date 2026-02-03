PKG_CONFIG ?= pkg-config
LIBS = wayland-client xkbcommon xkbregistry

CFLAGS ?= -Wall
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(LIBS))
LDLIBS = $(shell $(PKG_CONFIG) --libs $(LIBS))

SRCS = xkb-monitor.c layout-registry.c
OBJS = $(SRCS:.c=.o)

# Add layer-shell support if wayland-protocols and wayland-scanner are available
# Set SKIP_WLR_LAYER_SHELL=1 to disable layer-shell support
ifndef SKIP_WLR_LAYER_SHELL

# Check if wayland-protocols and wayland-scanner are available for layer-shell support
WAYLAND_PROTOCOLS_DIR := $(shell $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols 2>/dev/null)
WAYLAND_SCANNER := $(shell command -v wayland-scanner 2>/dev/null)
WLR_LAYER_SHELL_URL = https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/raw/master/unstable/wlr-layer-shell-unstable-v1.xml

ifneq ($(WAYLAND_PROTOCOLS_DIR),)
ifneq ($(WAYLAND_SCANNER),)
CFLAGS += -DHAVE_LAYER_SHELL
SRCS += xdg-shell-protocol.c wlr-layer-shell-protocol.c
PROTOCOL_HDRS = xdg-shell-client-protocol.h wlr-layer-shell-client-protocol.h
PROTOCOL_SRCS = xdg-shell-protocol.c wlr-layer-shell-protocol.c
OBJS = $(SRCS:.c=.o)
endif
endif
endif

.PHONY: all debug release clean distclean

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

# Protocol file generation (only when wayland-protocols and wayland-scanner are available)
ifndef SKIP_WLR_LAYER_SHELL
ifneq ($(WAYLAND_PROTOCOLS_DIR),)
ifneq ($(WAYLAND_SCANNER),)
xdg-shell-client-protocol.h:
	$(WAYLAND_SCANNER) client-header $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml $@

xdg-shell-protocol.c:
	$(WAYLAND_SCANNER) private-code $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml $@

wlr-layer-shell-unstable-v1.xml:
	curl -sL $(WLR_LAYER_SHELL_URL) -o $@

wlr-layer-shell-client-protocol.h: wlr-layer-shell-unstable-v1.xml
	$(WAYLAND_SCANNER) client-header $< $@

wlr-layer-shell-protocol.c: wlr-layer-shell-unstable-v1.xml
	$(WAYLAND_SCANNER) private-code $< $@

xkb-monitor.o: xkb-monitor.c $(PROTOCOL_HDRS)
endif
endif
endif

clean:
	rm -f xkb-monitor $(OBJS)

distclean: clean
	rm -f $(PROTOCOL_HDRS) $(PROTOCOL_SRCS) wlr-layer-shell-unstable-v1.xml
