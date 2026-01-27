#ifndef LAYOUT_REGISTRY_H
#define LAYOUT_REGISTRY_H

#include <xkbcommon/xkbcommon.h>

typedef struct layout_entry_t
{
  char* description;
  char* name;
  xkb_layout_index_t idx;
  struct layout_entry_t* next;
} layout_entry;

// Initialize the layout registry by parsing evdev.xml via libxkbregistry.
// Only loads entries for layouts present in the given keymap.
// Returns pointer to linked list of entries, or NULL on failure.
// Caller is responsible for calling layout_registry_cleanup() to free.
layout_entry* layout_registry_init(struct xkb_keymap* keymap);

// Look up the short layout name (e.g., "us") from the description
// (e.g., "English (US)"). Returns NULL if not found.
// The returned string is valid until layout_registry_cleanup() is called.
char const* layout_registry_lookup_name(layout_entry const* entries, char const* description);
char const* layout_registry_lookup_idx(layout_entry const* entries, xkb_layout_index_t idx);

// Free resources used by the registry.
void layout_registry_cleanup(layout_entry* entries);

#endif
