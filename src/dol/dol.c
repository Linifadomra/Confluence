#include "confluence/dol.h"
#include "confluence/endian.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dol_load(DOLFile* d, const char* path) {
    memset(d, 0, sizeof(*d));
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0x100) { fclose(f); return -1; }
    d->data = (uint8_t*)malloc((size_t)sz);
    if (!d->data) { fclose(f); return -1; }
    if (fread(d->data, 1, (size_t)sz, f) != (size_t)sz) {
        free(d->data); fclose(f); return -1;
    }
    fclose(f);
    d->size = (size_t)sz;
    for (int i = 0; i < 18; i++) {
        d->sections[i].file_off = gc_be32(d->data + 0x00 + i * 4);
        d->sections[i].addr     = gc_be32(d->data + 0x48 + i * 4);
        d->sections[i].size     = gc_be32(d->data + 0x90 + i * 4);
    }
    return 0;
}

void dol_free(DOLFile* d) {
    free(d->data);
    d->data = NULL;
    d->size = 0;
}

int dol_va_to_file(const DOLFile* d, uint32_t va, uint32_t size, uint32_t* file_off) {
    for (int i = 0; i < 18; i++) {
        const DOLSection* s = &d->sections[i];
        if (s->size == 0) continue;
        if (va >= s->addr && va + size <= s->addr + s->size) {
            *file_off = s->file_off + (va - s->addr);
            return 0;
        }
    }
    return -1;
}