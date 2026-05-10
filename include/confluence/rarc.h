#ifndef CONFLUENCE_RARC_H
#define CONFLUENCE_RARC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Writing */

typedef struct GCArc GCArc;
GCArc* gc_arc_open_file(const char* path);
GCArc* gc_arc_open_mem(const void* data, size_t size);
void gc_arc_close(GCArc* arc);
int gc_arc_entry_count(const GCArc* arc);
const GCEntry* gc_arc_entry(const GCArc* arc, int index);
int gc_arc_extract_all(GCArc* arc, const char* outputDir);
int gc_arc_read_file(GCArc* arc, int index, void** out_data, size_t* out_size);

/* Reading */

int gc_arc_replace_file(GCArc* arc, int index, void* new_data, size_t new_size);
int gc_arc_add_file(GCArc* arc, const char* node_type, const char* filename, void* data, size_t size);
int gc_arc_save(GCArc* arc, void** out_data, size_t* out_size);

#ifdef __cplusplus
}
#endif

#endif /* GC_RARC */