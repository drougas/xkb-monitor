#include "layout-registry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbregistry.h>

static char* extract_layout_name(char const* layout_desc) {
  char const* end = layout_desc ? strchr(layout_desc, '(') : NULL;
  uint8_t spacesFound = 0;
  if (end == NULL && layout_desc != NULL) {
    end = layout_desc + strlen(layout_desc);
  }
  while (end != layout_desc && *end == ' ') {
    --end;
    spacesFound = 1;
  }
  end += spacesFound;

  size_t const len = end - layout_desc;
  char* name = (char*)malloc((len + 1) * sizeof(char));

  memcpy(name, layout_desc, len);
  name[len] = '\0';
  return name;
}

layout_entries layout_registry_init(struct xkb_keymap* keymap) {
  xkb_layout_index_t const count = keymap ? xkb_keymap_num_layouts(keymap) : 0;
  layout_entry* const entries = (count > 0) ? (layout_entry*)malloc(count * sizeof(layout_entry)) : NULL;

  for (xkb_layout_index_t i = 0; i < count; i++) {
    char const* desc = xkb_keymap_layout_get_name(keymap, i);
    entries[i].description = strdup(desc ? desc : "??");
    entries[i].name = NULL;
    entries[i].variant = NULL;
  }

  struct rxkb_context* ctx = rxkb_context_new(RXKB_CONTEXT_NO_FLAGS);
  if (ctx) {
    if (!rxkb_context_parse_default_ruleset(ctx)) {
      rxkb_context_unref(ctx);
      ctx = NULL;
    }
  }

  struct rxkb_layout* layout = ctx ? rxkb_layout_first(ctx) : NULL;
  while (layout) {
    char const* const desc = rxkb_layout_get_description(layout);
    xkb_layout_index_t i = 0;

    while ((i < count) && (entries[i].name || strcmp(entries[i].description, desc))) {
      ++i;
    }

    if (i < count) {
      char const* const name = rxkb_layout_get_name(layout);
      char const* const variant = rxkb_layout_get_variant(layout);
      if (name && *name) {
        entries[i].name = strdup(name);
        entries[i].variant = strdup(variant ? variant : "");
      }
    }

    layout = rxkb_layout_next(layout);
  }
  if (ctx != NULL) {
    rxkb_context_unref(ctx);
    ctx = NULL;
  }

  for (xkb_layout_index_t i = 0; i < count; i++) {
    layout_entry* entry = &entries[i];
    if (entry->name == NULL) {
      entry->name = extract_layout_name(entry->description);
    }
    if (entry->variant == NULL) {
      entry->variant = strdup("");
    }
  }

  layout_entries const ret = {
    .entries = entries,
    .count = count,
  };
  return ret;
}

layout_entry const* layout_registry_lookup_idx(layout_entries const lentries, xkb_layout_index_t idx) {
  static layout_entry const empty_entry = {
    .description = "!!",
    .name = "!!",
    .variant = "!!",
  };
  return (idx < lentries.count) ? &(lentries.entries)[idx] : &empty_entry;
}

void layout_registry_cleanup(layout_entries* lentries) {
  layout_entry* const entries = lentries ? lentries->entries : NULL;
  xkb_layout_index_t const count = entries ? lentries->count : 0;
  for (xkb_layout_index_t i = 0; i < count; ++i) {
    free(entries[i].variant);
    free(entries[i].name);
    free(entries[i].description);
  }
  if (entries) {
    free(entries);
  }
  if (lentries) {
    lentries->count = 0;
    lentries->entries = NULL;
  }
}
