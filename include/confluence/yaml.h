#ifndef CONFLUENCE_YAML_H
#define CONFLUENCE_YAML_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Generic YAML tree built on top of libyaml.
//   - Mapping nodes have children with non-NULL key.
//   - Sequence nodes have children with NULL key.
//   - Scalar nodes have non-NULL value and no children.
typedef enum {
    CONFLUENCE_YAML_SCALAR = 0,
    CONFLUENCE_YAML_MAPPING,
    CONFLUENCE_YAML_SEQUENCE,
} ConfluenceYamlKind;

typedef struct ConfluenceYamlNode {
    ConfluenceYamlKind         kind;
    char*                      key;       // NULL inside sequences
    char*                      value;     // non-NULL on scalars
    struct ConfluenceYamlNode* children;
    struct ConfluenceYamlNode* next;
} ConfluenceYamlNode;

ConfluenceYamlNode* confluence_yaml_parse_file(const char* path);
void confluence_yaml_free(ConfluenceYamlNode* node);

const ConfluenceYamlNode* confluence_yaml_find(const ConfluenceYamlNode* parent, const char* key);
const char* confluence_yaml_get_str(const ConfluenceYamlNode* parent, const char* key);
int confluence_yaml_seq_count(const ConfluenceYamlNode* node);

#ifdef __cplusplus
}
#endif

#endif /* CONFLUENCE_YAML_H */
