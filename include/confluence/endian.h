#ifndef CONFLUENCE_ENDIAN
#define CONFLUENCE_ENDIAN

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

#ifdef __cplusplus
}
#endif

#endif /* GC_ENDIAN */