#ifndef GC_DISC_H
#define GC_DISC_H
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
} GCEntry;

#ifdef __cplusplus
}
#endif
#endif