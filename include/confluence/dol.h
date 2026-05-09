#ifndef GC_DOL_H
#define GC_DOL_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t file_off;
    uint32_t addr;
    uint32_t size;
} DOLSection;

typedef struct {
    uint8_t*   data;
    size_t     size;
    DOLSection sections[18];
} DOLFile;

int  dol_load(DOLFile* d, const char* path);
void dol_free(DOLFile* d);
int  dol_va_to_file(const DOLFile* d, uint32_t va, uint32_t size, uint32_t* file_off);

#ifdef __cplusplus
}
#endif
#endif