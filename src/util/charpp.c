/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "memory.h"

#include "charpp.h"

void charpp_create(struct charpp* charpp) {
    charpp->data  = NULL;
    charpp->alloc = 0;
    charpp->fill  = 0;
}

void charpp_destroy(struct charpp* charpp) {
    memory_free_charpp(charpp->data);
}

static bool charpp_ensure_space(struct charpp* charpp, size_t size) {
    char**  new_data;

    if (charpp->alloc >= size) return true;

    if (size < charpp->alloc * 2)
        size = charpp->alloc * 2;
    
    new_data = memory_realloc(charpp->data, size * sizeof(char*));

    if (new_data != NULL) {
        charpp->data  = new_data;
        charpp->alloc = size;
        return true;
    }
    return false;
}

bool charpp_put(struct charpp* charpp, size_t index, const char* str) {
    if (index >= charpp->fill) return false;
    memory_free(charpp->data[index]);
    charpp->data[index] = memory_strdup(str);
    return true;
}

void charpp_append(struct charpp* charpp, const char* str) {
    if (!charpp_ensure_space(charpp, charpp->fill + 2))
        return;
    charpp->data[charpp->fill++] = memory_strdup(str);
    charpp->data[charpp->fill]   = NULL;
}

char*const* charpp_get_static(struct charpp* charpp) {
    return charpp->data;
}

const char* charpp_get_static_at(struct charpp* charpp, size_t index) {
    if (index >= charpp->fill) return NULL;
    return charpp->data[index];
}

size_t charpp_get_size(struct charpp* charpp) {
    return charpp->fill;
}
