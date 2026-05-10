#include "confluence/rarc.h"
#include "confluence/endian.h"
#include "confluence/macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    char     type[5];
    char*    name;
    uint32_t name_offset;
    uint16_t name_hash;
    uint16_t num_files;
    uint32_t first_file_index;
    int      dir_entry_index;
} GCNode;

struct GCArc {
    unsigned char* data;
    size_t         size;
    int            owns_data;
    GCEntry*       entries;
    int            entry_count;
    char*          name_pool;
    GCNode*        nodes;
    int            num_nodes;
    uint8_t        keep_ids_synced;
    uint16_t       next_free_file_id;
};

static GCArc* gc_arc_open_common(unsigned char* data, size_t size, int owns) {
    if (size < 0x40 || memcmp(data, "RARC", 4) != 0) return NULL;

    unsigned int header_size = gc_be32(data + 0x08);
    unsigned int data_off    = gc_be32(data + 0x0C) + header_size;
    if (header_size < 0x20 || data_off > size) return NULL;

    const unsigned char* fst = data + header_size;
    unsigned int num_dirs    = gc_be32(fst + 0x00);
    unsigned int dirs_off    = gc_be32(fst + 0x04) + header_size;
    unsigned int num_entries = gc_be32(fst + 0x08);
    unsigned int files_off   = gc_be32(fst + 0x0C) + header_size;
    unsigned int str_len     = gc_be32(fst + 0x10);
    unsigned int str_off     = gc_be32(fst + 0x14) + header_size;

    if (dirs_off + num_dirs * 0x10 > size) return NULL;
    if (files_off + num_entries * 0x14 > size) return NULL;
    if (str_off + str_len > size) return NULL;

    GCArc* arc = (GCArc*)calloc(1, sizeof(GCArc));
    if (!arc) return NULL;
    arc->data = data;
    arc->size = size;
    arc->owns_data = owns;
    arc->entry_count = (int)num_entries;
    arc->entries = (GCEntry*)calloc(num_entries ? num_entries : 1, sizeof(GCEntry));
    if (!arc->entries) { gc_arc_close(arc); return NULL; }

    const unsigned char* files = data + files_off;
    const unsigned char* dirs  = data + dirs_off;
    const char*          strs  = (const char*)(data + str_off);

    size_t pool_cap = str_len * 8; if (pool_cap < 0x10000) pool_cap = 0x10000;
    arc->name_pool = (char*)malloc(pool_cap);
    if (!arc->name_pool) { gc_arc_close(arc); return NULL; }
    size_t pool_used = 0;

    char** dir_prefix = (char**)calloc(num_dirs, sizeof(char*));
    if (!dir_prefix) { gc_arc_close(arc); return NULL; }
    dir_prefix[0] = arc->name_pool;
    arc->name_pool[pool_used++] = '\0';
    arc->num_nodes = (int)num_dirs;
    arc->nodes = (GCNode*)calloc(num_dirs ? num_dirs : 1, sizeof(GCNode));
    if (!arc->nodes) { gc_arc_close(arc); return NULL; }
    
    for (unsigned int d = 0; d < num_dirs; d++) {
        const unsigned char* de = dirs + d * 0x10;
        unsigned short num_child = gc_be16(de + 0x0A);
        unsigned int   first     = gc_be32(de + 0x0C);
        const char*    parent_prefix = dir_prefix[d] ? dir_prefix[d] : "";

        for (unsigned int k = 0; k < num_child; k++) {
            unsigned int idx = first + k;
            if (idx >= num_entries) continue;
            const unsigned char* fe = files + idx * 0x14;
            unsigned short type     = gc_be16(fe + 0x04);
            unsigned short name_off = gc_be16(fe + 0x06);
            unsigned int   off_field= gc_be32(fe + 0x08);
            unsigned int   size     = gc_be32(fe + 0x0C);
            const char*    name     = strs + name_off;

            if (type == 0x0200 && (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)) continue;

            size_t plen = strlen(parent_prefix);
            size_t nlen = strlen(name);
            size_t need = plen + (plen ? 1 : 0) + nlen + 1;
            if (pool_used + need > pool_cap) continue;
            char* full = arc->name_pool + pool_used;
            if (plen) { memcpy(full, parent_prefix, plen); full[plen] = '/'; memcpy(full + plen + 1, name, nlen); full[plen + 1 + nlen] = '\0'; }
            else      { memcpy(full, name, nlen); full[nlen] = '\0'; }
            pool_used += need;

            GCEntry* out = &arc->entries[idx];
            out->name = full;
            if (type == 0x0200) {
                out->type = GC_ENTRY_DIR;
                out->discOffset = 0;
                out->size = 0;
                // off_field on dir entries is the subdir index into the dir table.
                if (off_field < num_dirs) { 
                    dir_prefix[off_field] = full;
                    arc->nodes[off_field].dir_entry_index = idx;
                }
            } else {
                out->type = GC_ENTRY_FILE;
                out->discOffset = data_off + off_field;
                out->size = size;
            }
        }
    }

    free(dir_prefix);
    arc->next_free_file_id = gc_be16(fst + 0x18);
    arc->keep_ids_synced   = fst[0x1A];

    arc->num_nodes = (int)num_dirs;
    arc->nodes = (GCNode*)calloc(num_dirs ? num_dirs : 1, sizeof(GCNode));
    if (!arc->nodes) { gc_arc_close(arc); return NULL; }

    for (unsigned int d = 0; d < num_dirs; d++) {
        const unsigned char* de = dirs + d * 0x10;
        GCNode* n        = &arc->nodes[d];
        unsigned int noff = gc_be32(de + 0x04);
        memcpy(n->type, data + dirs_off + d * 0x10, 4);
        n->type[4]        = '\0';
        n->name_offset    = noff;
        n->name           = (char*)(strs + noff);
        n->name_hash      = gc_be16(de + 0x08);
        n->num_files      = gc_be16(de + 0x0A);
        n->first_file_index = gc_be32(de + 0x0C);
    }
    return arc;
}

GCArc* gc_arc_open_mem(const void* data, size_t size) {
    return gc_arc_open_common((unsigned char*)data, size, 0);
}

GCArc* gc_arc_open_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    unsigned char* buf = (unsigned char*)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    GCArc* arc = gc_arc_open_common(buf, (size_t)sz, 1);
    if (!arc) free(buf);
    return arc;
}

void gc_arc_close(GCArc* arc) {
    if (!arc) return;
    if (arc->owns_data) free(arc->data);
    for (int i = 0; i < arc->entry_count; i++) {
        if (arc->entries[i].owns_buf) free(arc->entries[i].buf);
    }
    free(arc->entries);
    free(arc->nodes);
    free(arc->name_pool);
    free(arc);
}

int gc_arc_entry_count(const GCArc* arc) {
    return arc ? arc->entry_count : 0;
}

const GCEntry* gc_arc_entry(const GCArc* arc, int index) {
    if (!arc || index < 0 || index >= arc->entry_count) return NULL;
    return &arc->entries[index];
}

static void mkdir_p(const char* path) {
    char buf[4096];
    size_t n = strlen(path);
    if (n >= sizeof(buf)) return;
    memcpy(buf, path, n + 1);
    for (size_t i = 1; i < n; i++) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            MKDIR_ONE(buf);
            buf[i] = '/';
        }
    }
    MKDIR_ONE(buf);
}

int gc_arc_extract_all(GCArc* arc, const char* outputDir) {
    if (!arc || !outputDir) return -1;
    mkdir_p(outputDir);

    char path[4096];
    size_t olen = strlen(outputDir);
    for (int i = 0; i < arc->entry_count; i++) {
        const GCEntry* e = &arc->entries[i];
        if (!e->name) continue;

        size_t nlen = strlen(e->name);
        if (olen + 1 + nlen + 1 > sizeof(path)) return -1;
        memcpy(path, outputDir, olen);
        path[olen] = '/';
        memcpy(path + olen + 1, e->name, nlen + 1);

        if (e->type == GC_ENTRY_DIR) {
            mkdir_p(path);
            continue;
        }

        for (size_t j = olen + 1 + nlen; j > olen; j--) {
            if (path[j] == '/') {
                path[j] = '\0';
                mkdir_p(path);
                path[j] = '/';
                break;
            }
        }
        FILE* f = fopen(path, "wb");
        if (!f) return -1;
        if (fwrite(arc->data + e->discOffset, 1, e->size, f) != e->size) {
            fclose(f);
            return -1;
        }
        fclose(f);
    }
    return 0;
}

int gc_arc_read_file(GCArc* arc, int index, void** out_data, size_t* out_size) {
    if (!arc || index < 0 || index >= arc->entry_count) return -1;
    const GCEntry* e = &arc->entries[index];
    if (e->type != GC_ENTRY_FILE) return -1;
    if ((size_t)e->discOffset + e->size > arc->size) return -1;
    void* buf = malloc(e->size);
    if (!buf) return -1;
    memcpy(buf, arc->data + e->discOffset, e->size);
    *out_data = buf;
    *out_size = e->size;
    return 0;
}

int gc_arc_replace_file(GCArc* arc, int index, void* new_data, size_t new_size) {
    if (!arc || index < 0 || index >= arc->entry_count) return -1;
    GCEntry* e = &arc->entries[index];
    if (e->type != GC_ENTRY_FILE) return -1;
    if (e->owns_buf) free(e->buf);

    e->buf      = new_data;
    e->owns_buf = 1;
    e->size     = (uint32_t)new_size;
    return 0;
}

int gc_arc_add_file(GCArc* arc, const char* node_type, const char* filename,
                    void* data, size_t size) {
    if (!arc || !node_type || !filename || !data) return -1;

    int node_idx = -1;
    for (int i = 0; i < arc->num_nodes; i++) {
        if (strncasecmp(arc->nodes[i].type, node_type, 4) == 0) {
            node_idx = i;
            break;
        }
    }
    if (node_idx < 0) return -1;

    GCNode* node = &arc->nodes[node_idx];

    int new_count = arc->entry_count + 1;
    GCEntry* new_entries = (GCEntry*)realloc(arc->entries, new_count * sizeof(GCEntry));
    if (!new_entries) return -1;
    arc->entries = new_entries;

    int insert_at = (int)(node->first_file_index + node->num_files);

    memmove(&arc->entries[insert_at + 1],
            &arc->entries[insert_at],
            (arc->entry_count - insert_at) * sizeof(GCEntry));

    for (int i = 0; i < arc->num_nodes; i++) {
        if ((int)arc->nodes[i].first_file_index >= insert_at && i != node_idx)
            arc->nodes[i].first_file_index++;
    }

    GCEntry* e  = &arc->entries[insert_at];
    memset(e, 0, sizeof(GCEntry));

    size_t namelen = strlen(filename) + 1;
    size_t pool_used = 0;
    for (int i = 0; i < new_count; i++)
        if (arc->entries[i].name)
            pool_used = (size_t)((arc->entries[i].name + strlen(arc->entries[i].name) + 1)
                                 - arc->name_pool);

    char* new_pool = (char*)realloc(arc->name_pool, pool_used + namelen);
    if (!new_pool) return -1;

    if (new_pool != arc->name_pool) {
        ptrdiff_t delta = new_pool - arc->name_pool;
        for (int i = 0; i < new_count; i++)
            if (arc->entries[i].name)
                arc->entries[i].name += delta;
        for (int i = 0; i < arc->num_nodes; i++)
            if (arc->nodes[i].name)
                arc->nodes[i].name += delta;
        arc->name_pool = new_pool;
    }

    memcpy(arc->name_pool + pool_used, filename, namelen);
    e->name     = arc->name_pool + pool_used;
    e->type     = GC_ENTRY_FILE;
    e->buf      = data;
    e->owns_buf = 1;
    e->size     = (uint32_t)size;
    e->discOffset = 0;

    node->num_files++;
    arc->entry_count = new_count;

    if (arc->keep_ids_synced) {
        arc->next_free_file_id = (uint16_t)new_count;
    }

    return insert_at;
}

int gc_arc_save(GCArc* arc, void** out_data, size_t* out_size) {
    if (!arc || !out_data || !out_size) return -1;

    size_t cap = 1 << 20;
    uint8_t* buf = (uint8_t*)calloc(1, cap);
    if (!buf) return -1;
    size_t pos = 0;

#define ENSURE(n) do { \
    while (pos + (n) > cap) { \
        cap *= 2; \
        uint8_t* tmp = (uint8_t*)realloc(buf, cap); \
        if (!tmp) { free(buf); return -1; } \
        buf = tmp; \
    } \
} while(0)

#define ALIGN(a) do { \
    size_t rem = pos % (a); \
    if (rem) { size_t pad = (a) - rem; ENSURE(pad); memset(buf+pos,0,pad); pos+=pad; } \
} while(0)

    ENSURE(0x40);
    memset(buf, 0, 0x40);
    pos = 0x40;

    uint32_t node_list_off = (uint32_t)pos;
    ENSURE(arc->num_nodes * 0x10);
    memset(buf + pos, 0, arc->num_nodes * 0x10);
    pos += arc->num_nodes * 0x10;

    ALIGN(0x20);
    uint32_t entries_list_off = (uint32_t)pos;
    ENSURE(arc->entry_count * 0x14);
    memset(buf + pos, 0, arc->entry_count * 0x14);
    pos += arc->entry_count * 0x14;

    ALIGN(0x20);
    uint32_t str_list_off = (uint32_t)pos;

    typedef struct { const char* str; uint32_t off; } StrEntry;
    StrEntry str_cache[4096];
    int str_cache_count = 0;

    #define INTERN(s, out_off) do { \
        int _found = 0; \
        for (int _i = 0; _i < str_cache_count; _i++) { \
            if (strcmp(str_cache[_i].str, (s)) == 0) { \
                (out_off) = str_cache[_i].off; _found = 1; break; \
            } \
        } \
        if (!_found) { \
            size_t _len = strlen(s) + 1; \
            ENSURE(_len); \
            memcpy(buf + pos, (s), _len); \
            (out_off) = (uint32_t)(pos - str_list_off); \
            str_cache[str_cache_count].str = (s); \
            str_cache[str_cache_count].off = (out_off); \
            str_cache_count++; \
            pos += _len; \
        } \
    } while(0)

    uint32_t dot_off, dotdot_off;
    INTERN(".", dot_off);
    INTERN("..", dotdot_off);

    for (int i = 0; i < arc->num_nodes; i++) {
        uint32_t off;
        INTERN(arc->nodes[i].name, off);
        arc->nodes[i].name_offset = off;
    }

    uint32_t* entry_name_offs = (uint32_t*)malloc(arc->entry_count * sizeof(uint32_t));
    if (!entry_name_offs) { free(buf); return -1; }
    for (int i = 0; i < arc->entry_count; i++) {
        const char* name = arc->entries[i].name ? arc->entries[i].name : "";
        if (strcmp(name, ".") == 0)       entry_name_offs[i] = dot_off;
        else if (strcmp(name, "..") == 0) entry_name_offs[i] = dotdot_off;
        else { uint32_t off; INTERN(name, off); entry_name_offs[i] = off; }
    }

    for (int i = 0; i < arc->num_nodes; i++) {
        GCNode* n = &arc->nodes[i];
        uint8_t* np = buf + node_list_off + i * 0x10;

        uint16_t hash = 0;
        for (const char* c = n->name; *c; c++) { hash *= 3; hash += (uint8_t)*c; }
        n->name_hash = hash;

        memcpy(np, n->type, 4);
        uint32_t no = n->name_offset; np[4]=no>>24; np[5]=(no>>16)&0xFF; np[6]=(no>>8)&0xFF; np[7]=no&0xFF;
        np[8]=n->name_hash>>8; np[9]=n->name_hash&0xFF;
        np[10]=n->num_files>>8; np[11]=n->num_files&0xFF;
        uint32_t fi=n->first_file_index; np[12]=fi>>24; np[13]=(fi>>16)&0xFF; np[14]=(fi>>8)&0xFF; np[15]=fi&0xFF;
    }

    ALIGN(0x20);
    uint32_t file_data_off = (uint32_t)pos;
    uint32_t mram_size = 0, aram_size = 0;
    uint32_t next_data_off = 0;

    for (int pass = 0; pass < 2; pass++) {
        uint32_t pass_start = next_data_off;
        for (int i = 0; i < arc->entry_count; i++) {
            GCEntry* e = &arc->entries[i];
            if (e->type != GC_ENTRY_FILE) continue;

            int is_aram = 0;
            if (e->name) {
                size_t nl = strlen(e->name);
                if (nl >= 4 && strcmp(e->name + nl - 4, ".rel") == 0) is_aram = 1;
            }
            if (pass == 0 && is_aram)  continue;
            if (pass == 1 && !is_aram) continue;

            const void* src  = e->owns_buf ? e->buf : (arc->data + e->discOffset);
            uint32_t    sz   = e->size;

            ENSURE(next_data_off + sz + 0x20);
            memcpy(buf + file_data_off + next_data_off, src, sz);
            e->discOffset = next_data_off;
            next_data_off += sz;

            uint32_t rem = next_data_off % 0x20;
            if (rem) next_data_off += 0x20 - rem;
        }
        if (pass == 0) mram_size = next_data_off;
    }
    aram_size = next_data_off - mram_size;
    pos = file_data_off + next_data_off;

    uint16_t file_id = 0;
    for (int i = 0; i < arc->entry_count; i++) {
        GCEntry* e  = &arc->entries[i];
        uint8_t* ep = buf + entries_list_off + i * 0x14;

        uint16_t hash = 0;
        const char* nm = e->name ? e->name : "";
        for (const char* c = nm; *c; c++) { hash *= 3; hash += (uint8_t)*c; }

        uint16_t id = file_id;

        uint8_t attr = (e->type == GC_ENTRY_FILE)
            ? (uint8_t)(GC_ENTRY_FILE | 0x10)
            : (uint8_t)0x02;

        if (e->type == GC_ENTRY_FILE && e->name) {
            size_t nl = strlen(e->name);
            if (nl >= 4 && strcmp(e->name + nl - 4, ".rel") == 0)
                attr = (uint8_t)(GC_ENTRY_FILE | 0x20);
        }

        uint32_t type_name = ((uint32_t)attr << 24) | (entry_name_offs[i] & 0x00FFFFFF);
        uint32_t data_or_node;
        uint32_t data_sz;

        if (e->type == GC_ENTRY_FILE) {
            data_or_node = e->discOffset;
            data_sz      = e->size;
        } else {
            data_or_node = 0xFFFFFFFF;
            for (int ni = 0; ni < arc->num_nodes; ni++) {
                if (arc->nodes[ni].dir_entry_index == i) {
                    data_or_node = (uint32_t)ni;
                    break;
                }
            }
            data_sz = 0x10;
        }

        ep[0]=id>>8;       ep[1]=id&0xFF;
        ep[2]=hash>>8;     ep[3]=hash&0xFF;
        ep[4]=type_name>>24; ep[5]=(type_name>>16)&0xFF; ep[6]=(type_name>>8)&0xFF; ep[7]=type_name&0xFF;
        ep[8]=data_or_node>>24; ep[9]=(data_or_node>>16)&0xFF; ep[10]=(data_or_node>>8)&0xFF; ep[11]=data_or_node&0xFF;
        ep[12]=data_sz>>24; ep[13]=(data_sz>>16)&0xFF; ep[14]=(data_sz>>8)&0xFF; ep[15]=data_sz&0xFF;
        ep[16]=ep[17]=ep[18]=ep[19]=0;
    }

    free(entry_name_offs);

    uint32_t total_size = (uint32_t)pos;
    memcpy(buf, "RARC", 4);
    buf[4]=total_size>>24; buf[5]=(total_size>>16)&0xFF; buf[6]=(total_size>>8)&0xFF; buf[7]=total_size&0xFF;
    buf[8]=0; buf[9]=0; buf[10]=0; buf[11]=0x20;
    uint32_t fdo = file_data_off - 0x20;
    buf[12]=fdo>>24; buf[13]=(fdo>>16)&0xFF; buf[14]=(fdo>>8)&0xFF; buf[15]=fdo&0xFF;
    buf[16]=next_data_off>>24; buf[17]=(next_data_off>>16)&0xFF; buf[18]=(next_data_off>>8)&0xFF; buf[19]=next_data_off&0xFF;
    buf[20]=mram_size>>24; buf[21]=(mram_size>>16)&0xFF; buf[22]=(mram_size>>8)&0xFF; buf[23]=mram_size&0xFF;
    buf[24]=aram_size>>24; buf[25]=(aram_size>>16)&0xFF; buf[26]=(aram_size>>8)&0xFF; buf[27]=aram_size&0xFF;
    buf[28]=buf[29]=buf[30]=buf[31]=0;

    uint8_t* dh = buf + 0x20;
    uint32_t nn = (uint32_t)arc->num_nodes;
    dh[0]=nn>>24; dh[1]=(nn>>16)&0xFF; dh[2]=(nn>>8)&0xFF; dh[3]=nn&0xFF;
    uint32_t nlo = node_list_off - 0x20;
    dh[4]=nlo>>24; dh[5]=(nlo>>16)&0xFF; dh[6]=(nlo>>8)&0xFF; dh[7]=nlo&0xFF;
    uint32_t ne = (uint32_t)arc->entry_count;
    dh[8]=ne>>24; dh[9]=(ne>>16)&0xFF; dh[10]=(ne>>8)&0xFF; dh[11]=ne&0xFF;
    uint32_t elo = entries_list_off - 0x20;
    dh[12]=elo>>24; dh[13]=(elo>>16)&0xFF; dh[14]=(elo>>8)&0xFF; dh[15]=elo&0xFF;
    uint32_t str_sz = file_data_off - str_list_off;
    dh[16]=str_sz>>24; dh[17]=(str_sz>>16)&0xFF; dh[18]=(str_sz>>8)&0xFF; dh[19]=str_sz&0xFF;
    uint32_t slo = str_list_off - 0x20;
    dh[20]=slo>>24; dh[21]=(slo>>16)&0xFF; dh[22]=(slo>>8)&0xFF; dh[23]=slo&0xFF;
    uint16_t nfid = arc->next_free_file_id;
    dh[24]=nfid>>8; dh[25]=nfid&0xFF;
    dh[26]=arc->keep_ids_synced;
    dh[27]=0;
    dh[28]=dh[29]=dh[30]=dh[31]=0;

    *out_data = buf;
    *out_size = pos;
    return 0;

#undef ENSURE
#undef ALIGN
#undef INTERN
}