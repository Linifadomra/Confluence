#ifndef CONFLUENCE_ENDIAN
#define CONFLUENCE_ENDIAN

#include <string.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint16_t gc_be16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

static inline uint32_t gc_be32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static inline uint64_t gc_be64(const uint8_t* p) {
    return (uint64_t)gc_be32(p) << 32 | gc_be32(p + 4);
}

static inline uint32_t gc_le32(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static inline void gc_write_be16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFF);
}

static inline void gc_write_be32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)((v >> 16) & 0xFF);
    p[2] = (uint8_t)((v >>  8) & 0xFF);
    p[3] = (uint8_t)(v & 0xFF);
}

static inline float gc_be_f32(const uint8_t* p) {
    uint32_t v = gc_be32(p);
    float f;
    memcpy(&f, &v, sizeof(f));
    return f;
}

static inline void gc_write_be_f32(uint8_t* p, float v) {
    uint32_t u;
    memcpy(&u, &v, sizeof(u));
    gc_write_be32(p, u);
}

#ifdef __cplusplus
}
#endif

#endif /* GC_ENDIAN */