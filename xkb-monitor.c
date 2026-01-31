#include "layout_registry.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#ifndef NDEBUG
#define DEBUG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...) (void)0
#endif

typedef struct kb_info_t
{
  xkb_layout_index_t layout;
  int caps_lock;
  int num_lock;
  int scroll_lock;
} kb_info;

typedef struct app_state_t
{
  struct wl_display* display;
  struct wl_registry* registry;
  struct wl_seat* seat;
  struct wl_keyboard* keyboard;

  struct xkb_context* xkb_context;
  struct xkb_keymap* xkb_keymap;
  struct xkb_state* xkb_state;

  layout_entries layout_entries;

  uint32_t seat_version;

  kb_info last;

  // Options
  uint8_t json_format;
  uint8_t symbol_only;
} app_state;

static void print_keyboard_state(app_state* state) {
  if (!state->xkb_state || !state->xkb_keymap) {
    fprintf(stderr, "XKB state not initialized\n");
    return;
  }

  // Get current layout
  xkb_layout_index_t const layout = xkb_state_serialize_layout(state->xkb_state, XKB_STATE_LAYOUT_EFFECTIVE);

  // Check lock states
  int const caps_lock = xkb_state_mod_name_is_active(state->xkb_state, XKB_MOD_NAME_CAPS, XKB_STATE_MODS_LOCKED);
  int const num_lock = xkb_state_mod_name_is_active(state->xkb_state, XKB_MOD_NAME_NUM, XKB_STATE_MODS_LOCKED);

  // Check Scroll Lock LED
  xkb_led_index_t const scroll_led = xkb_keymap_led_get_index(state->xkb_keymap, XKB_LED_NAME_SCROLL);
  int const scroll_lock = xkb_state_led_index_is_active(state->xkb_state, scroll_led);

  kb_info* last = &(state->last);
  if (layout == last->layout && caps_lock == last->caps_lock) {
    if (num_lock == last->num_lock && scroll_lock == last->scroll_lock) {
      return;
    }
  }

  last->layout = layout;
  last->caps_lock = caps_lock;
  last->num_lock = num_lock;
  last->scroll_lock = scroll_lock;

  layout_entry const* const entry = layout_registry_lookup_idx(state->layout_entries, layout);

  if (state->json_format) {
    printf(
      "{\"index\":%u,\"description\":\"%s\",\"name\":\"%s\",\"variant\",\"%s\","
      "\"symbol\":\"%s\",\"caps\":%s,\"num\":%s,\"scroll\":%s}\n",
      layout, entry->description, entry->name, entry->variant, entry->symbol, caps_lock ? "true" : "false",
      num_lock ? "true" : "false", scroll_lock ? "true" : "false");
  } else {
    printf(
      "%d,%s,%s,%s,%s,%d,%d,%d\n", layout, entry->description, entry->name, entry->variant, entry->symbol, caps_lock,
      num_lock, scroll_lock);
  }
}

static void print_symbol_keyboard_state(app_state* state) {
  if (!state->xkb_state || !state->xkb_keymap) {
    fprintf(stderr, "XKB state not initialized\n");
    return;
  }

  // Get current layout
  xkb_layout_index_t const layout = xkb_state_serialize_layout(state->xkb_state, XKB_STATE_LAYOUT_EFFECTIVE);
  kb_info* last = &(state->last);
  if (layout == last->layout) {
    return;
  }

  last->layout = layout;

  layout_entry const* entry = layout_registry_lookup_idx(state->layout_entries, layout);
  if (state->json_format) {
    printf("{\"text\":\"%s\"}\n", entry->symbol);
  } else {
    printf("%s\n", entry->symbol);
  }
}

// Keyboard listeners
static void keyboard_keymap(void* data, struct wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size) {
  app_state* state = (app_state*)data;

  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    fprintf(stderr, "Unsupported keymap format: %u\n", format);
    close(fd);
    return;
  }

  char* map_shm = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map_shm == MAP_FAILED) {
    fprintf(stderr, "Failed to mmap keymap\n");
    close(fd);
    return;
  }

  struct xkb_keymap* keymap =
    xkb_keymap_new_from_string(state->xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

  munmap(map_shm, size);
  close(fd);

  if (!keymap) {
    fprintf(stderr, "Failed to compile keymap\n");
    return;
  }

  struct xkb_state* xkb_state = xkb_state_new(keymap);
  if (!xkb_state) {
    fprintf(stderr, "Failed to create XKB state\n");
    xkb_keymap_unref(keymap);
    return;
  }

  // Replace old state
  if (state->xkb_state) {
    xkb_state_unref(state->xkb_state);
  }
  if (state->xkb_keymap) {
    xkb_keymap_unref(state->xkb_keymap);
  }

  state->xkb_keymap = keymap;
  state->xkb_state = xkb_state;

  // Initialize or reinitialize the layout registry with the new keymap
  layout_registry_cleanup(&state->layout_entries);
  state->layout_entries = layout_registry_init(keymap);
  if (state->layout_entries.count == 0) {
    fprintf(stderr, "Warning: Failed to initialize layout registry\n");
  }

  DEBUG("\n[Keymap loaded]");
  if (state->symbol_only) {
    print_symbol_keyboard_state(state);
  } else {
    print_keyboard_state(state);
  }
}

static void keyboard_enter(
  void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys) {
  DEBUG("\n[Keyboard focus gained]\n");
}

static void keyboard_leave(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface) {
  DEBUG("\n[Keyboard focus lost]\n");
}

static void keyboard_key(
  void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t key_state) {
  // We don't need to do anything here for monitoring state
  DEBUG("\n[Key pressed]: %u %u\n", key, key_state);
}

static void keyboard_modifiers(
  void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
  uint32_t mods_locked, uint32_t group) {
  app_state* state = (app_state*)data;

  if (!state->xkb_state) {
    return;
  }

  xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);

  DEBUG(
    "\n[Modifiers/Layout changed]: serial=%u depressed=%u latched=%u locked=%u group=%u", //
    serial, mods_depressed, mods_latched, mods_locked, group);
  if (state->symbol_only) {
    print_symbol_keyboard_state(state);
  } else {
    print_keyboard_state(state);
  }
}

static void keyboard_repeat_info(void* data, struct wl_keyboard* keyboard, int32_t rate, int32_t delay) {
  // Not needed for our purposes
  DEBUG("\n[Key repeat]: %d %d\n", rate, delay);
}

static const struct wl_keyboard_listener keyboard_listener = {
  .keymap = keyboard_keymap,
  .enter = keyboard_enter,
  .leave = keyboard_leave,
  .key = keyboard_key,
  .modifiers = keyboard_modifiers,
  .repeat_info = keyboard_repeat_info,
};

// Seat listeners
static void seat_capabilities(void* data, struct wl_seat* seat, uint32_t capabilities) {
  app_state* state = (app_state*)data;

  if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
    if (!state->keyboard) {
      state->keyboard = wl_seat_get_keyboard(seat);
      wl_keyboard_add_listener(state->keyboard, &keyboard_listener, state);
      DEBUG("Keyboard capability available\n");
    }
  } else {
    if (state->keyboard) {
      wl_keyboard_release(state->keyboard);
      state->keyboard = NULL;
      DEBUG("Keyboard capability lost\n");
    }
  }
}

static void seat_name(void* data, struct wl_seat* seat, char const* name) {
  DEBUG("Seat name: %s\n", name);
}

static const struct wl_seat_listener seat_listener = {
  .capabilities = seat_capabilities,
  .name = seat_name,
};

// Registry listeners
static void registry_global(
  void* data, struct wl_registry* registry, uint32_t name, char const* interface, uint32_t version) {
  app_state* state = (app_state*)data;

  if (strcmp(interface, wl_seat_interface.name) == 0) {
    state->seat_version = version;
    state->seat = (struct wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, version < 7 ? version : 7);
    wl_seat_add_listener(state->seat, &seat_listener, state);
  }
}

static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
  // Not handling removals
}

static const struct wl_registry_listener registry_listener = {
  .global = registry_global,
  .global_remove = registry_global_remove,
};

static app_state s_state = {
  .display = NULL,
  .registry = NULL,
  .seat = NULL,
  .keyboard = NULL,

  .xkb_context = NULL,
  .xkb_keymap = NULL,
  .xkb_state = NULL,

  .seat_version = 0,

  .last =
    {
      .layout = XKB_LAYOUT_INVALID,
      .caps_lock = -1,
      .num_lock = -1,
      .scroll_lock = -1,
    },

  .json_format = 0,
  .symbol_only = 0,

  .layout_entries = {0},
};

static void cleanup() {
  DEBUG("Cleaning up\n");
  layout_registry_cleanup(&s_state.layout_entries);
  if (s_state.keyboard) {
    wl_keyboard_release(s_state.keyboard);
    s_state.keyboard = NULL;
  }
  if (s_state.seat) {
    wl_seat_release(s_state.seat);
    s_state.seat = NULL;
  }
  if (s_state.registry) {
    wl_registry_destroy(s_state.registry);
    s_state.registry = NULL;
  }
  if (s_state.xkb_state) {
    xkb_state_unref(s_state.xkb_state);
    s_state.xkb_state = NULL;
  }
  if (s_state.xkb_keymap) {
    xkb_keymap_unref(s_state.xkb_keymap);
    s_state.xkb_keymap = NULL;
  }
  if (s_state.display) {
    wl_display_disconnect(s_state.display);
    s_state.display = NULL;
  }
  if (s_state.xkb_context) {
    xkb_context_unref(s_state.xkb_context);
    s_state.xkb_context = NULL;
  }
}

static void signalHandler(int sig) {
  DEBUG("Caught signal %d\n", sig);
  cleanup();
}

int main(int argc, char* argv[]) {
  atexit(cleanup);
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  setlinebuf(stdout);

  for (int opt; (opt = getopt(argc, argv, "jsh")) != -1;) {
    switch (opt) {
      case 'j': s_state.json_format = 1; break;
      case 's': s_state.symbol_only = 1; break;
      case 'h':
        fprintf(stderr, "Usage: %s [-j] [-s] [-r]\n", argv[0]);
        fprintf(stderr, "  -j: JSON output\n");
        fprintf(stderr, "  -s: symbol only (layout changes only)\n");
        fprintf(stderr, "  -js: Waybar output with CSS classes for symbols\n");
        return 0;
      default: fprintf(stderr, "Usage: %s [-j] [-s]\n", argv[0]); return 1;
    }
  }

  // Create XKB context
  s_state.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!s_state.xkb_context) {
    fprintf(stderr, "Failed to create XKB context\n");
    return 1;
  }

  // Connect to Wayland display
  s_state.display = wl_display_connect(NULL);
  if (!s_state.display) {
    fprintf(stderr, "Failed to connect to Wayland display\n");
    fprintf(stderr, "Make sure you're running this in a Wayland session\n");
    return 1;
  }

  DEBUG("Connected to Wayland display\n");

  // Get registry
  s_state.registry = wl_display_get_registry(s_state.display);
  wl_registry_add_listener(s_state.registry, &registry_listener, &s_state);

  // Wait for registry events
  wl_display_roundtrip(s_state.display);

  if (!s_state.seat) {
    fprintf(stderr, "No seat found\n");
    return 1;
  }

  DEBUG("\nMonitoring keyboard state (press Ctrl+C to exit)...\n");
  DEBUG("Note: You need to focus this terminal window to receive keyboard events\n\n");

  // Main event loop
  while (wl_display_dispatch(s_state.display) != -1) {
    // Continue processing events
  }

  return 0;
}
