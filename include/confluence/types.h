#ifndef CONFLUENCE_TYPES_H
#define CONFLUENCE_TYPES_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GC_ENTRY_FILE = 0,
    GC_ENTRY_DIR  = 1,
} GCEntryType;

typedef struct {
    GCEntryType type;
    const char* name;
    uint32_t    discOffset;
    uint32_t    size;
    void*       buf;
    int         owns_buf;
    uint16_t    id;
    uint8_t     attr;
} GCEntry;

#ifdef __cplusplus
}
#endif

#endif /* CONFLUENCE_TYPES_H */