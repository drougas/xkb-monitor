#ifndef LAYOUT_REGISTRY_H
#define LAYOUT_REGISTRY_H

#include <xkbcommon/xkbcommon.h>

typedef struct layout_entry_t
{
  char* description;
  char* name;
  char* variant;
} layout_entry;

typedef struct layout_entries_t {
  layout_entry* entries;
  xkb_layout_index_t count;
} layout_entries;

// Initialize the layout registry by parsing evdev.xml via libxkbregistry.
// Only loads entries for layouts present in the given keymap.
// Returns pointer to linked list of entries, or NULL on failure.
// Caller is responsible for calling layout_registry_cleanup() to free.
layout_entries layout_registry_init(struct xkb_keymap* keymap);

// Look up the short layout name (e.g., "us") from the description
// (e.g., "English (US)"). Returns NULL if not found.
// The returned string is valid until layout_registry_cleanup() is called.
layout_entry const* layout_registry_lookup_idx(layout_entries const lentries, xkb_layout_index_t idx);

// Free resources used by the registry.
void layout_registry_cleanup(layout_entries* entries);

#endif
