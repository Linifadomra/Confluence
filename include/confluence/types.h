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
    const char* name;       // full path relative to files/
    uint32_t    discOffset; // files only
    uint32_t    size;       // file size, or next-entry index for dirs
    void*       buf;        // non-nullptr for replaced/added entries; used by gc_arc_save
    int         owns_buf;   // 1 if buf was malloc'd and gc_arc_close should free it
    uint16_t    id;         // ID of the entry
} GCEntry;

#ifdef __cplusplus
}
#endif

#endif /* CONFLUENCE_TYPES_H */