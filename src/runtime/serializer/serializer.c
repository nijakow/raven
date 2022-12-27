/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../util/memory.h"

#include "../core/blueprint.h"
#include "../core/objects/object/object.h"

#include "../core/objects/array.h"
#include "../core/objects/mapping.h"
#include "../core/objects/string.h"
#include "../core/objects/symbol.h"
#include "../core/objects/funcref.h"

#include "serializer.h"


struct serializer_object_page* serializer_object_page_new() {
    struct serializer_object_page* page;
    
    page = memory_alloc(sizeof(struct serializer_object_page) + sizeof(struct serializer_object_page_entry) * 256);
    page->next = NULL;
    page->fill = 0;
    return page;
}

void serializer_object_page_delete(struct serializer_object_page* page) {
    memory_free(page);
}

bool serializer_object_page_has_space(struct serializer_object_page* page) {
    return page->fill < 256;
}

bool serializer_object_page_add(struct serializer_object_page* page, void* obj, uint32_t id) {
    if (!serializer_object_page_has_space(page))
        return false;
    page->entries[page->fill].id = id;
    page->entries[page->fill].value = obj;
    page->fill++;
    return true;
}


void serializer_object_pages_create(struct serializer_object_pages* pages) {
    pages->first   = NULL;
    pages->next_id = 1;
}

void serializer_object_pages_destroy(struct serializer_object_pages* pages) {
    struct serializer_object_page* page = pages->first;
    while (page != NULL) {
        struct serializer_object_page* next = page->next;
        serializer_object_page_delete(page);
        page = next;
    }
}

bool serializer_object_pages_add(struct serializer_object_pages* pages, void* obj, uint32_t* id) {
    struct serializer_object_page**  page;
    uint32_t                         lid;
    bool                             result;

    page = &pages->first;
    lid  =  pages->next_id++;

    while (*page != NULL) {
        if (serializer_object_page_add(*page, obj, lid))
            return true;
        page = &(*page)->next;
    }

    *page  = serializer_object_page_new();
    result = serializer_object_page_add(*page, obj, lid);

    if (id != NULL)
        *id = lid;

    return result;
}

bool serializer_object_pages_lookup(struct serializer_object_pages* pages, void* obj, uint32_t* id) {
    struct serializer_object_page* page = pages->first;
    while (page != NULL) {
        for (size_t i = 0; i < page->fill; i++) {
            if (page->entries[i].value == obj) {
                if (id != NULL)
                    *id = page->entries[i].id;
                return true;
            }
        }
        page = page->next;
    }
    return false;
}


void serializer_create(struct serializer* serializer) {
    serializer_object_pages_create(&serializer->object_pages);
    serializer->buffer = NULL;
}

void serializer_destroy(struct serializer* serializer) {
    serializer_object_pages_destroy(&serializer->object_pages);
}

void serializer_write_to_bytebuffer(struct serializer* serializer, struct bytebuffer* buffer) {
    serializer->buffer = buffer;
}

void serializer_write(struct serializer* serializer, const void* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        serializer_write_uint8(serializer, ((uint8_t*)data)[i]);
    }
}

void serializer_write_with_size(struct serializer* serializer, const void* data, size_t size) {
    serializer_write_uint(serializer, size);
    serializer_write(serializer, data, size);
}

void serializer_write_uint8(struct serializer* serializer, uint8_t value) {
    if (serializer->buffer != NULL)
        bytebuffer_write_uint8(serializer->buffer, value);
}

void serializer_write_uint(struct serializer* serializer, uint32_t value) {
    serializer_write(serializer, &value, sizeof(value));
}

void serializer_write_int(struct serializer* serializer, int32_t value) {
    serializer_write(serializer, &value, sizeof(value));
}

void serializer_write_cstr(struct serializer* serializer, const char* str) {
    serializer_write_with_size(serializer, str, strlen(str));
}

void serializer_write_tag(struct serializer* serializer, enum serializer_tag tag) {
    serializer_write_uint8(serializer, tag);
}

bool serializer_write_ref(struct serializer* serializer, void* ptr) {
    uint32_t  id;

    if (serializer_object_pages_lookup(&serializer->object_pages, ptr, NULL)) {
        serializer_write_tag(serializer, SERIALIZER_TAG_REF);
        serializer_write_uint(serializer, id);
        return true;
    }

    if (serializer_object_pages_add(&serializer->object_pages, ptr, &id)) {
        serializer_write_tag(serializer, SERIALIZER_TAG_LABEL);
        serializer_write_uint(serializer, id);
    }
    
    return false;
}

void serializer_write_nil(struct serializer* serializer) {
    serializer_write_tag(serializer, SERIALIZER_TAG_NIL);
}

void serializer_write_string(struct serializer* serializer, struct string* string) {
    serializer_write_tag(serializer, SERIALIZER_TAG_STRING);
    serializer_write_cstr(serializer, string_contents(string));
}

void serializer_write_symbol(struct serializer* serializer, struct symbol* symbol) {
    serializer_write_tag(serializer, SERIALIZER_TAG_SYMBOL);
    serializer_write_cstr(serializer, symbol_name(symbol));
}

void serializer_write_array(struct serializer* serializer, struct array* array) {
    serializer_write_tag(serializer, SERIALIZER_TAG_ARRAY);
    serializer_write_uint(serializer, array_size(array));
    for (size_t i = 0; i < array_size(array); i++) {
        serializer_write_any(serializer, array_get(array, i));
    }
}

void serializer_write_mapping(struct serializer* serializer, struct mapping* mapping) {
    serializer_write_tag(serializer, SERIALIZER_TAG_MAPPING);
    serializer_write_uint(serializer, mapping_size(mapping));
    for (size_t i = 0; i < mapping_size(mapping); i++) {
        serializer_write_any(serializer, mapping_key(mapping, i));
        serializer_write_any(serializer, mapping_value(mapping, i));
    }
}

void serializer_write_funcref(struct serializer* serializer, struct funcref* funcref) {
    serializer_write_tag(serializer, SERIALIZER_TAG_FUNCREF);
    serializer_write_any(serializer, funcref_receiver(funcref));
    serializer_write_symbol(serializer, funcref_message(funcref));
}

void serializer_write_blueprint(struct serializer* serializer, struct blueprint* blueprint) {
    serializer_write_tag(serializer, SERIALIZER_TAG_BLUEPRINT);
    if (blueprint_parent(blueprint) == NULL)
        serializer_write_nil(serializer);
    else
        serializer_write_ptr(serializer, blueprint_parent(blueprint));
}

void serializer_write_object(struct serializer* serializer, struct object* object) {
    serializer_write_tag(serializer, SERIALIZER_TAG_OBJECT);
    serializer_write_ptr(serializer, object_blueprint(object));
    serializer_write_any(serializer, any_from_ptr(object_parent(object)));
    serializer_write_any(serializer, any_from_ptr(object_sibling(object)));
    serializer_write_any(serializer, any_from_ptr(object_children(object)));
    serializer_write_any(serializer, object_stash(object));
}

void serializer_write_ptr(struct serializer* serializer, void* obj) {
    if (serializer_write_ref(serializer, obj)) {
        return;
    }

         if (base_obj_is(obj, OBJ_TYPE_STRING))    { serializer_write_string(serializer, obj);               }
    else if (base_obj_is(obj, OBJ_TYPE_SYMBOL))    { serializer_write_symbol(serializer, obj);               }
    else if (base_obj_is(obj, OBJ_TYPE_ARRAY))     { serializer_write_array(serializer, obj);                }
    else if (base_obj_is(obj, OBJ_TYPE_MAPPING))   { serializer_write_mapping(serializer, obj);              }
    else if (base_obj_is(obj, OBJ_TYPE_FUNCREF))   { serializer_write_funcref(serializer, obj);              }
    else if (base_obj_is(obj, OBJ_TYPE_BLUEPRINT)) { serializer_write_blueprint(serializer, obj);            }
    else                                           { serializer_write_tag(serializer, SERIALIZER_TAG_ERROR); }
}

void serializer_write_any(struct serializer* serializer, any any) {
         if (any_is_nil(any))  { serializer_write_tag(serializer, SERIALIZER_TAG_NIL);   }
    else if (any_is_int(any))  { serializer_write_tag(serializer, SERIALIZER_TAG_INT);
                                 serializer_write_int(serializer, any_to_int(any));      }
    else if (any_is_char(any)) { serializer_write_tag(serializer, SERIALIZER_TAG_CHAR8);
                                 serializer_write_uint8(serializer, any_to_char(any));   }
    else                       { serializer_write_ptr(serializer, any_to_ptr(any));      }
}
