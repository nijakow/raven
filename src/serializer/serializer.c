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
