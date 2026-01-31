#include "layout_registry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbregistry.h>

static char* extract_layout_symbol(char const* layout_desc) {
  // Extract two-letter symbol from layout name
  // Common formats: "English (US)", "us", "Russian", etc.
  char* symbol = (char*)malloc(3 * sizeof(char));
  symbol[0] = symbol[1] = symbol[2] = 0;

  // Look for parentheses pattern like "English (US)"
  char const* const paren = layout_desc ? strchr(layout_desc, '(') : NULL;
  if (paren && paren[1] && paren[2] && paren[1] != ')' && paren[2] != ')') {
    symbol[0] = paren[1];
    symbol[1] = paren[2];
  } else if (layout_desc && *layout_desc) {
    // If it's already short (2-3 chars), use first 2 chars
    symbol[0] = layout_desc[0];
    symbol[1] = layout_desc[1];
  }

  for (int i = 0; i < 2; ++i) {
    if (symbol[i] >= 'A' && symbol[i] <= 'Z') {
      symbol[i] += ('a' - 'A');
    } else if (symbol[i] < 'a' || symbol[i] > 'z') {
      symbol[i] = '?';
    }
  }

  return symbol;
}

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
    entries[i].symbol = NULL;
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
    if (entry->name[0] && entry->name[1]) {
      entry->symbol = extract_layout_symbol(entry->name);
    } else {
      entry->symbol = extract_layout_symbol(entry->description);
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
    .symbol = "!!",
  };
  return (idx < lentries.count) ? &(lentries.entries)[idx] : &empty_entry;
}

void layout_registry_cleanup(layout_entries* lentries) {
  layout_entry* const entries = lentries ? lentries->entries : NULL;
  xkb_layout_index_t const count = entries ? lentries->count : 0;
  for (xkb_layout_index_t i = 0; i < count; ++i) {
    free(entries[i].symbol);
    free(entries[i].variant);
    free(entries[i].name);
    free(entries[i].description);
  }
  if (lentries) {
    lentries->count = 0;
    lentries->entries = NULL;
  }
}
