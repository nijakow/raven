#include "../core/objects/string.h"

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

bool serializer_write_ref(struct serializer* serializer, any any) {
    return false;
}

void serializer_write_string(struct serializer* serializer, struct string* string) {
    serializer_write_tag(serializer, SERIALIZER_TAG_STRING);
    serializer_write_with_size(serializer, string_contents(string), string_length(string));
}

void serializer_write_obj(struct serializer* serializer, any any) {
    if (serializer_write_ref(serializer, any)) {
        return;
    }

         if (any_is_obj(any, OBJ_TYPE_STRING)) { serializer_write_string(serializer, any_to_ptr(any)); }
    else                                       { serializer_write_tag(serializer, SERIALIZER_TAG_ERROR); }
}

void serializer_write_any(struct serializer* serializer, any any) {
         if (any_is_nil(any))  { serializer_write_tag(serializer, SERIALIZER_TAG_NIL); }
    else if (any_is_int(any))  { serializer_write_tag(serializer, SERIALIZER_TAG_INT);
                                 serializer_write_int(serializer, any_to_int(any)); }
    else if (any_is_char(any)) { serializer_write_tag(serializer, SERIALIZER_TAG_CHAR8);
                                 serializer_write_uint8(serializer, any_to_char(any)); }
    else                       { serializer_write_obj(serializer, any); }
}
