/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../defs.h"

#include "memory.h"

#include "stringbuilder.h"


void stringbuilder_create(struct stringbuilder* sb) {
    sb->alloc = 0;
    sb->fill  = 0;
    sb->data  = NULL;
}

void stringbuilder_destroy(struct stringbuilder* sb) {
    if (sb->data != NULL)
        memory_free(sb->data);
}

void stringbuilder_clear(struct stringbuilder* sb) {
    sb->fill = 0;
}

static bool stringbuilder_resize(struct stringbuilder* sb) {
    char*  data;

    if (sb->data == NULL) {
        sb->alloc = 32;
        sb->data  = memory_alloc(32 * sizeof(char));
    } else {
        data = memory_alloc((sb->alloc * 2) * sizeof(char));
        if (data == NULL)
            return false;
        memcpy(data, sb->data, sb->fill);
        memory_free(sb->data);
        sb->data  = data;
        sb->alloc = sb->alloc * 2;
    }
    return true;
}

void stringbuilder_append_char(struct stringbuilder* sb, char c) {
    if (sb->fill + 1 >= sb->alloc)
        if (!stringbuilder_resize(sb))
            return;
    sb->data[sb->fill++] = c;
    sb->data[sb->fill] = '\0';
}

void stringbuilder_append_rune(struct stringbuilder* sb, raven_rune_t rune) {
    char    buf[5];
    size_t  index;
    size_t  length;

    length = utf8_encode(rune, buf);
    for (index = 0; index < length; index++)
        stringbuilder_append_char(sb, buf[index]);
}

void stringbuilder_append_str(struct stringbuilder* sb, const char* str) {
    unsigned int i;

    for (i = 0; str[i] != '\0'; i++)
        stringbuilder_append_char(sb, str[i]);
}

bool stringbuilder_get(struct stringbuilder* sb, char** loc) {
    if (sb->data == NULL)
        *loc = memory_strdup("");
    else
        *loc = memory_strdup(sb->data);
    return (*loc != NULL);
}
