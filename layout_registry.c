#include "layout_registry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbregistry.h>

static int64_t is_keymap_layout(struct xkb_keymap* keymap, char const* description) {
  xkb_layout_index_t num_layouts = xkb_keymap_num_layouts(keymap);
  for (xkb_layout_index_t i = 0; i < num_layouts; i++) {
    char const* name = xkb_keymap_layout_get_name(keymap, i);
    if (name && strcmp(name, description) == 0) {
      return i;
    }
  }
  return -1;
}

layout_entry* layout_registry_init(struct xkb_keymap* keymap) {
  if (!keymap) {
    return NULL;
  }

  struct rxkb_context* ctx = rxkb_context_new(RXKB_CONTEXT_NO_FLAGS);
  if (!ctx) {
    return NULL;
  }

  if (!rxkb_context_parse_default_ruleset(ctx)) {
    rxkb_context_unref(ctx);
    return NULL;
  }

  layout_entry* entries = NULL;
  struct rxkb_layout* layout = rxkb_layout_first(ctx);
  while (layout) {
    char const* const name = rxkb_layout_get_name(layout);
    char const* const variant = rxkb_layout_get_variant(layout);
    char const* const desc = rxkb_layout_get_description(layout);

    int64_t const idx = (name && desc) ? is_keymap_layout(keymap, desc) : -1;
    layout_entry* entry = (idx >= 0) ? malloc(sizeof(layout_entry)) : NULL;
    if (entry) {
      entry->description = strdup(desc);
      // For variants, combine layout+variant as the short name
      if (variant && *variant) {
        size_t len = strlen(name) + strlen(variant) + 2;
        entry->name = malloc(len);
        if (entry->name) {
          snprintf(entry->name, len, "%s(%s)", name, variant);
        }
      } else {
        entry->name = strdup(name);
      }
      entry->idx = (xkb_layout_index_t)idx;
      entry->next = entries;
      entries = entry;
    }

    layout = rxkb_layout_next(layout);
  }

  rxkb_context_unref(ctx);
  return entries;
}

char const* layout_registry_lookup_name(layout_entry const* entries, char const* description) {
  if (!description) {
    return NULL;
  }
  for (layout_entry const* e = entries; e; e = e->next) {
    if (strcmp(e->description, description) == 0) {
      return e->name;
    }
  }
  return NULL;
}

char const* layout_registry_lookup_idx(layout_entry const* entries, xkb_layout_index_t idx) {
  for (layout_entry const* e = entries; e; e = e->next) {
    if (e->idx == idx) {
      return e->name;
    }
  }
  return NULL;
}

void layout_registry_cleanup(layout_entry* entries) {
  layout_entry* e = entries;
  while (e) {
    layout_entry* next = e->next;
    free(e->description);
    free(e->name);
    free(e);
    e = next;
  }
}
