/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../core/blueprint.h"
#include "../core/objects/object.h"

#include "../core/objects/array.h"
#include "../core/objects/mapping.h"
#include "../core/objects/string.h"
#include "../core/objects/symbol.h"
#include "../core/objects/funcref.h"

#include "serializer.h"


void serializer_create(struct serializer* serializer) {

}

void serializer_destroy(struct serializer* serializer) {

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
    // TODO
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
