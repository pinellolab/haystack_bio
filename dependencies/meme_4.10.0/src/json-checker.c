

#include <assert.h>

#include "json-checker.h"
#include "json-reader.h"
#include "red-black-tree.h"
#include "string-builder.h"
#include "utils.h"

#define ALLOW_NULL 1
#define ALLOW_BOOL 2
#define ALLOW_NUMBER 4
#define ALLOW_STRING 8
#define ALLOW_OBJECT 16
#define ALLOW_LIST 32

typedef enum node_type NT_EN;
enum node_type {
  NT_NONE,
  NT_OBJ,
  NT_LST,
  NT_PROP
};

struct jsonchk {
  void (*error)(void *, char *, va_list);
  void *user_data;
  JSON_OBJ_DEF_T* schema;
  NT_EN current_type;
  void* current_node;
  int udepth;
  bool has_error;
  STR_T* path_buf;
  JSONRD_T *parser;
};

struct json_obj_def {
  JsonSetupFn setup;
  JsonFinalizeFn finalize;
  JsonAbortFn abort;
  RBTREE_T *properties; // set of property definitions
  bool ignore_unknown; // should unknown properties be ignored?
  // parent node
  NT_EN parent_type;
  void* parent_node;
  // mutable data
  void *data;
};

struct json_lst_def {
  int allowed_types; // bit field
  int dimensions; // number of nested lists of the same definition
  // list callbacks
  JsonSetupFn setup;
  JsonFinalizeFn finalize;
  JsonAbortFn abort;
  // item callbacks
  JsonListNullFn item_null;
  JsonListBoolFn item_bool;
  JsonListNumberFn item_number;
  JsonListStringFn item_string;
  JsonListListFn item_list;
  JsonListObjectFn item_object;
  // child definitions
  JSON_OBJ_DEF_T *child_obj;
  JSON_LST_DEF_T *child_lst;
  // parent node
  NT_EN parent_type;
  void* parent_node;
  // mutable data
  int dimension;
  int *index;
  void *data;
};

struct json_prop_def {
  char *name; // name of the property
  bool required; // is the property required
  int allowed_types; // bit field
  // property callbacks
  JsonPropNullFn prop_null;
  JsonPropBoolFn prop_bool;
  JsonPropNumberFn prop_number;
  JsonPropStringFn prop_string;
  JsonPropListFn prop_list;
  JsonPropObjectFn prop_object;
  // child definitions
  JSON_OBJ_DEF_T *child_obj;
  JSON_LST_DEF_T *child_lst;
  // parent node
  JSON_OBJ_DEF_T *parent_node;
  // mutable data
  bool seen;
};

// need this defined so we can call it
void jsonchk_error(void *user_data, char *fmt, va_list args);

//****************************************************************************
// Report an error
//****************************************************************************
static void error(JSONCHK_T *jsonchk, char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  jsonchk_error(jsonchk, fmt, ap);
  va_end(ap);
}

//****************************************************************************
// Make a path string
//****************************************************************************
static char* path(JSONCHK_T* doc) {
  STR_T *path;
  void *node;
  NT_EN type;
  int i;
  JSON_LST_DEF_T *list;
  path = doc->path_buf;
  str_clear(path);
  node = doc->current_node;
  type = doc->current_type;
  while (node != NULL) {
    switch (type) {
      case NT_NONE:
        node = NULL;
        break;
      case NT_OBJ:
        type = ((JSON_OBJ_DEF_T*)node)->parent_type;
        node = ((JSON_OBJ_DEF_T*)node)->parent_node;
        break;
      case NT_LST:
        list = (JSON_LST_DEF_T*)node;
        for (i = list->dimension - 1; i >= 0; i--) {
          str_insertf(path, 0, "[%d]", list->index[i]);
        }
        type = ((JSON_LST_DEF_T*)node)->parent_type;
        node = ((JSON_LST_DEF_T*)node)->parent_node;
        break;
      case NT_PROP:
        //TODO I suppose I should escape this string...
        str_insertf(path, 0, "[\"%s\"]", ((JSON_PROP_DEF_T*)node)->name);
        node = ((JSON_PROP_DEF_T*)node)->parent_node;
        type = NT_OBJ;
        break;
    }
  }
  str_insert(path, 0, "data", 4);
  return str_internal(path);
}

//****************************************************************************
// Get the data section of the parent node
//****************************************************************************
static inline void* get_owner_data(NT_EN parent_type, void *parent_node, void *user_data) {
  switch (parent_type) {
    case NT_NONE:
      return user_data;
    case NT_OBJ:
      return ((JSON_OBJ_DEF_T*)parent_node)->data;
    case NT_LST:
      return ((JSON_LST_DEF_T*)parent_node)->data;
    case NT_PROP:
      return ((JSON_PROP_DEF_T*)parent_node)->parent_node->data;
    default:
      die("Unknown node type: %d", parent_type);
      return NULL;
  }
}


//****************************************************************************
// Destroy a JSON object definition
//****************************************************************************
void jd_obj_destroy(void *obj) {
  JSON_OBJ_DEF_T *o;
  o = (JSON_OBJ_DEF_T*)obj;
  rbtree_destroy(o->properties);
  memset(o, 0, sizeof(JSON_OBJ_DEF_T));
  free(o);
}

//****************************************************************************
// Destroy a JSON list definition
//****************************************************************************
void jd_lst_destroy(void *lst) {
  JSON_LST_DEF_T *l;
  l = (JSON_LST_DEF_T*)lst;
  memset(l->index, 0, sizeof(int) * l->dimensions);
  free(l->index);
  if (l->child_obj != NULL) jd_obj_destroy(l->child_obj);
  if (l->child_lst != NULL) jd_lst_destroy(l->child_lst);
  memset(l, 0, sizeof(JSON_LST_DEF_T));
  free(l);
}

//****************************************************************************
// Destroy a JSON property definition
//****************************************************************************
void jd_prop_destroy(void *prop) {
  JSON_PROP_DEF_T *p;
  p = (JSON_PROP_DEF_T*)prop;
  if (p->child_obj != NULL) jd_obj_destroy(p->child_obj);
  if (p->child_lst != NULL) jd_lst_destroy(p->child_lst);
  memset(p, 0, sizeof(JSON_PROP_DEF_T));
  free(p);
}

//****************************************************************************
// Create a JSON object definition
//****************************************************************************
JSON_OBJ_DEF_T* jd_obj(JsonSetupFn setup, JsonFinalizeFn finalize,
    JsonAbortFn abort, bool ignore_unknown, int property_count, ...) {
  JSON_OBJ_DEF_T *obj_def;
  JSON_PROP_DEF_T *prop_def;
  va_list ap;
  int i;
  obj_def = mm_malloc(sizeof(JSON_OBJ_DEF_T));
  memset(obj_def, 0, sizeof(JSON_OBJ_DEF_T));
  obj_def->setup = setup;
  obj_def->finalize = finalize;
  obj_def->abort = abort;
  obj_def->ignore_unknown = ignore_unknown;
  obj_def->properties = rbtree_create(rbtree_strcmp, rbtree_strcpy, free, NULL, jd_prop_destroy);
  va_start(ap, property_count);
  for (i = 0; i < property_count; i++) {
    prop_def = va_arg(ap, JSON_PROP_DEF_T*);
    rbtree_put(obj_def->properties, prop_def->name, prop_def);
    prop_def->parent_node = obj_def;
  }
  va_end(ap);
  return obj_def;
}

//****************************************************************************
// Create a JSON boolean property definition
//****************************************************************************
JSON_PROP_DEF_T* jd_pbool(char *name, bool required, JsonPropBoolFn prop_bool) {
  JSON_PROP_DEF_T *prop;
  prop = mm_malloc(sizeof(JSON_PROP_DEF_T));
  memset(prop, 0, sizeof(JSON_PROP_DEF_T));
  prop->name = name;
  prop->required = required;
  prop->allowed_types = ALLOW_BOOL;
  prop->prop_bool = prop_bool;
  return prop;
}

//****************************************************************************
// Create a JSON numeric property definition
//****************************************************************************
JSON_PROP_DEF_T* jd_pnum(char *name, bool required, JsonPropNumberFn prop_number) {
  JSON_PROP_DEF_T *prop;
  prop = mm_malloc(sizeof(JSON_PROP_DEF_T));
  memset(prop, 0, sizeof(JSON_PROP_DEF_T));
  prop->name = name;
  prop->required = required;
  prop->allowed_types = ALLOW_NUMBER;
  prop->prop_number = prop_number;
  return prop;
}

//****************************************************************************
// Create a JSON string property definition
//****************************************************************************
JSON_PROP_DEF_T* jd_pstr(char *name, bool required, JsonPropStringFn prop_string) {
  JSON_PROP_DEF_T *prop;
  prop = mm_malloc(sizeof(JSON_PROP_DEF_T));
  memset(prop, 0, sizeof(JSON_PROP_DEF_T));
  prop->name = name;
  prop->required = required;
  prop->allowed_types = ALLOW_STRING;
  prop->prop_string = prop_string;
  return prop;
}

//****************************************************************************
// Create a JSON list property definition
//****************************************************************************
JSON_PROP_DEF_T* jd_plst(char *name, bool required, JsonPropListFn prop_list,
    JSON_LST_DEF_T *lst_def) {
  JSON_PROP_DEF_T *prop;
  prop = mm_malloc(sizeof(JSON_PROP_DEF_T));
  memset(prop, 0, sizeof(JSON_PROP_DEF_T));
  prop->name = name;
  prop->required = required;
  prop->allowed_types = ALLOW_LIST;
  prop->prop_list = prop_list;
  prop->child_lst = lst_def;
  lst_def->parent_node = prop;
  lst_def->parent_type = NT_PROP;
  return prop;
}

//****************************************************************************
// Create a JSON object property definition
//****************************************************************************
JSON_PROP_DEF_T* jd_pobj(char *name, bool required,
    JsonPropObjectFn prop_object, JSON_OBJ_DEF_T *obj_def) {
  JSON_PROP_DEF_T *prop;
  prop = mm_malloc(sizeof(JSON_PROP_DEF_T));
  memset(prop, 0, sizeof(JSON_PROP_DEF_T));
  prop->name = name;
  prop->required = required;
  prop->allowed_types = ALLOW_OBJECT;
  prop->prop_object = prop_object;
  prop->child_obj = obj_def;
  obj_def->parent_node = prop;
  obj_def->parent_type = NT_PROP;
  return prop;
}

//****************************************************************************
// Create a JSON list of numbers definition
//****************************************************************************
JSON_LST_DEF_T* jd_lnum(int dimensions, JsonSetupFn setup,
    JsonFinalizeFn finalize, JsonAbortFn abort, JsonListNumberFn item_number) {
  JSON_LST_DEF_T *lst;
  assert(dimensions >= 1);
  lst = mm_malloc(sizeof(JSON_LST_DEF_T));
  memset(lst, 0, sizeof(JSON_LST_DEF_T));
  lst->dimensions = dimensions;
  lst->setup = setup;
  lst->finalize = finalize;
  lst->abort = abort;
  lst->allowed_types = ALLOW_NUMBER;
  lst->item_number = item_number;
  lst->index = mm_malloc(sizeof(int) * dimensions);
  return lst;
}

//****************************************************************************
// Create a JSON list of strings definition
//****************************************************************************
JSON_LST_DEF_T* jd_lstr(int dimensions, JsonSetupFn setup,
    JsonFinalizeFn finalize, JsonAbortFn abort, JsonListStringFn item_string) {
  JSON_LST_DEF_T *lst;
  assert(dimensions >= 1);
  lst = mm_malloc(sizeof(JSON_LST_DEF_T));
  memset(lst, 0, sizeof(JSON_LST_DEF_T));
  lst->dimensions = dimensions;
  lst->setup = setup;
  lst->finalize = finalize;
  lst->abort = abort;
  lst->allowed_types = ALLOW_STRING;
  lst->item_string = item_string;
  lst->index = mm_malloc(sizeof(int) * dimensions);
  return lst;
}

//****************************************************************************
// Create a JSON list of lists definition
//****************************************************************************
JSON_LST_DEF_T* jd_llst(int dimensions, JsonSetupFn setup,
    JsonFinalizeFn finalize, JsonAbortFn abort, JsonListListFn item_list,
    JSON_LST_DEF_T *lst_def) {
  JSON_LST_DEF_T *lst;
  assert(dimensions >= 1);
  lst = mm_malloc(sizeof(JSON_LST_DEF_T));
  memset(lst, 0, sizeof(JSON_LST_DEF_T));
  lst->dimensions = dimensions;
  lst->setup = setup;
  lst->finalize = finalize;
  lst->abort = abort;
  lst->allowed_types = ALLOW_LIST;
  lst->item_list = item_list;
  lst->child_lst = lst_def;
  lst->index = mm_malloc(sizeof(int) * dimensions);
  lst_def->parent_node = lst;
  lst_def->parent_type = NT_LST;
  return lst;
}

//****************************************************************************
// Create a JSON list of objects definition
//****************************************************************************
JSON_LST_DEF_T* jd_lobj(int dimensions, JsonSetupFn setup,
    JsonFinalizeFn finalize, JsonAbortFn abort, JsonListObjectFn item_object,
    JSON_OBJ_DEF_T *obj_def) {
  JSON_LST_DEF_T *lst;
  assert(dimensions >= 1);
  lst = mm_malloc(sizeof(JSON_LST_DEF_T));
  memset(lst, 0, sizeof(JSON_LST_DEF_T));
  lst->dimensions = dimensions;
  lst->setup = setup;
  lst->finalize = finalize;
  lst->abort = abort;
  lst->allowed_types = ALLOW_OBJECT;
  lst->item_object = item_object;
  lst->child_obj = obj_def;
  lst->index = mm_malloc(sizeof(int) * dimensions);
  obj_def->parent_node = lst;
  obj_def->parent_type = NT_LST;
  return lst;
}

//****************************************************************************
// Pass the error on to the user
//****************************************************************************
void jsonchk_error(void *user_data, char *fmt, va_list args) {
  JSONCHK_T *reader;
  reader = (JSONCHK_T*)user_data;
  reader->has_error = true;
  if (reader->error) {
    reader->error(reader->user_data, fmt, args);
  }
}

//****************************************************************************
// The JSON parser found an annotated JSON section
//****************************************************************************
void jsonchk_start_data(void *user_data, char *section_name, size_t section_name_len) {
  JSONCHK_T *reader;
  reader = (JSONCHK_T*)user_data;
  reader->current_node = NULL;
  reader->current_type = NT_NONE;
  reader->udepth = 0;
}
//****************************************************************************
// The JSON parser found the end of the annotated JSON section
//****************************************************************************
void jsonchk_end_data(void *user_data) {
  JSONCHK_T *reader;
  reader = (JSONCHK_T*)user_data;

}
//****************************************************************************
// The JSON parser started reading a object property
//****************************************************************************
void jsonchk_start_property(void *user_data, char *property_name, int property_name_len) {
  JSONCHK_T *reader;
  JSON_OBJ_DEF_T *obj_def;
  JSON_PROP_DEF_T *prop_def;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) {
    reader->udepth++;
    return;
  }
  assert(reader->current_type == NT_OBJ);
  obj_def = (JSON_OBJ_DEF_T*)reader->current_node;
  prop_def = rbtree_get(obj_def->properties, property_name);
  if (prop_def != NULL) {
    prop_def->seen = true;
    reader->current_type = NT_PROP;
    reader->current_node = prop_def;
  } else {
    reader->udepth = 1;
    if (!obj_def->ignore_unknown) {
        error(reader, "Object %s contains unknown property %s", 
            path(reader), property_name);
    }
  }
}
//****************************************************************************
// The JSON parser finished reading a object property
//****************************************************************************
void jsonchk_end_property(void *user_data) {
  JSONCHK_T *reader;
  JSON_OBJ_DEF_T *obj_def;
  JSON_PROP_DEF_T *prop_def;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) {
    reader->udepth--;
    return;
  }
  assert(reader->current_type == NT_PROP);
  prop_def = (JSON_PROP_DEF_T*)reader->current_node;
  reader->current_type = NT_OBJ;
  reader->current_node = prop_def->parent_node;
}

//****************************************************************************
// The JSON parser read the start of a object
//****************************************************************************
void jsonchk_start_object(void *user_data) {
  JSONCHK_T *reader;
  JSON_OBJ_DEF_T *child_def;
  JSON_LST_DEF_T *lst_def;
  JSON_PROP_DEF_T *prop_def;
  RBNODE_T *node;
  void *owner;
  int *index;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) {
    reader->udepth++;
    return;
  }
  switch (reader->current_type) {
    case NT_NONE:
      child_def = reader->schema;
      owner = reader->user_data;
      index = NULL;
      break;
    case NT_LST:
      lst_def = (JSON_LST_DEF_T*)reader->current_node;
      if (lst_def->dimension != lst_def->dimensions || 
          (lst_def->allowed_types & ALLOW_OBJECT) == 0) {
        error(reader, "List item %s should not be a object", path(reader));
        reader->udepth = 1;
        return;
      }
      child_def = lst_def->child_obj;
      owner = lst_def->data;
      index = lst_def->index;
      break;
    case NT_PROP:
      prop_def = (JSON_PROP_DEF_T*)reader->current_node;
      if ((prop_def->allowed_types & ALLOW_OBJECT) == 0) {
        error(reader, "Property %s should not be a object", path(reader));
        reader->udepth = 1;
        return;
      }
      child_def = prop_def->child_obj;
      owner = prop_def->parent_node->data;
      index = NULL;
      break;
    case NT_OBJ:
      die("The node type OBJ should not occur for the parent of an object.");
    default:
      die("Unknown node type: %d", reader->current_type);
      return; // satisfy compiler
  }
  if (child_def->setup != NULL) {
    child_def->data = child_def->setup(reader->user_data, owner, index);
    if (child_def->data == NULL) {
      reader->has_error = true;
      return;
    }
  } else {
    child_def->data = owner;
  }
  reader->current_node = child_def;
  reader->current_type = NT_OBJ;
  // reset properties to unseen
  for (node = rbtree_first(child_def->properties); node != NULL; node = rbtree_next(node)) {
    prop_def = (JSON_PROP_DEF_T*)rbtree_value(node);
    prop_def->seen = false;
  }
}

//****************************************************************************
// The JSON parser read the end of an object
//****************************************************************************
void jsonchk_end_object(void *user_data) {
  JSONCHK_T *reader;
  JSON_OBJ_DEF_T *current_def;
  JSON_PROP_DEF_T *prop_def;
  JSON_LST_DEF_T *lst_def;
  bool missing_properties;
  RBNODE_T *i;
  void *temp;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) {
    reader->udepth--;
    return;
  }
  assert(reader->current_type == NT_OBJ);
  current_def = (JSON_OBJ_DEF_T*)reader->current_node;
  // check all required properties have been seen
  missing_properties = false;
  for (i = rbtree_first(current_def->properties); i != NULL; i = rbtree_next(i)) {
    prop_def = (JSON_PROP_DEF_T*)rbtree_value(i);
    if (prop_def->required && !prop_def->seen) {
      missing_properties = true;
      error(reader, "Object %s is missing required property %s",
          path(reader), prop_def->name);
    }
  }
  if (missing_properties) return;
  if (current_def->finalize != NULL) {
    temp = current_def->finalize(
        reader->user_data, get_owner_data(
          current_def->parent_type, current_def->parent_node, reader->user_data),
        current_def->data);
    if (temp == NULL) {
      reader->has_error = true;
      return;
    }
    current_def->data = temp;
  }
  switch (current_def->parent_type) {
    case NT_NONE:
      break;
    case NT_LST:
      lst_def = (JSON_LST_DEF_T*)current_def->parent_node;
      if (lst_def->item_object != NULL) {
        if (!(lst_def->item_object(reader->user_data, lst_def->data, lst_def->index, current_def->data))) {
          reader->has_error = true;
          return;
        }
      }
      lst_def->index[lst_def->dimension - 1]++;
      current_def->data = NULL;
      reader->current_node = lst_def;
      reader->current_type = NT_LST;
      break;
    case NT_PROP:
      prop_def = (JSON_PROP_DEF_T*)current_def->parent_node;
      if (prop_def->prop_object != NULL) {
        if (!(prop_def->prop_object(reader->user_data, prop_def->parent_node->data, prop_def->name, current_def->data))) {
          reader->has_error = true;
          return;
        }
      }
      current_def->data = NULL;
      reader->current_node = prop_def;
      reader->current_type = NT_PROP;
      break;
    case NT_OBJ:
      die("The node type OBJ should not occur for the parent of an object.");
      break;
    default:
      die("Unknown node type: %d", current_def->parent_type);
  }
}

//****************************************************************************
// The JSON parser read the start of a list
//****************************************************************************
void jsonchk_start_list(void *user_data) {
  JSONCHK_T *reader;
  JSON_LST_DEF_T *lst_def, *child_def;
  JSON_PROP_DEF_T *prop_def;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) {
    reader->udepth++;
    return;
  }
  switch (reader->current_type) {
    case NT_LST:
      lst_def = (JSON_LST_DEF_T*)reader->current_node;
      if (lst_def->dimension < lst_def->dimensions) {
        lst_def->index[lst_def->dimension] = 0;
        lst_def->dimension++;
      } else if ((lst_def->allowed_types & ALLOW_LIST) != 0) {
        child_def = lst_def->child_lst;
        child_def->index[0] = 0;
        child_def->dimension = 1;
        if (child_def->setup != NULL) {
          child_def->data = child_def->setup(reader->user_data, lst_def->data, lst_def->index);
          if (child_def->data == NULL) {
            reader->has_error = true;
            return;
          }
        } else {
          child_def->data = lst_def->data;
        }
        reader->current_node = child_def;
        reader->current_type = NT_LST;
      } else {
        error(reader, "List item %s should not be a list", path(reader));
        reader->udepth = 1;
      }
      break;
    case NT_PROP:
      prop_def = (JSON_PROP_DEF_T*)reader->current_node;
      if ((prop_def->allowed_types & ALLOW_LIST) != 0) {
        child_def = prop_def->child_lst;
        child_def->index[0] = 0;
        child_def->dimension = 1;
        if (child_def->setup != NULL) {
          child_def->data = child_def->setup(reader->user_data, prop_def->parent_node->data, NULL);
          if (child_def->data == NULL) {
            reader->has_error = true;
            return;
          }
        } else {
          child_def->data = prop_def->parent_node->data;
        }
        reader->current_node = child_def;
        reader->current_type = NT_LST;
      } else {
        error(reader, "Property %s should not be a list", path(reader));
        reader->udepth = 1;
      }
      break;
    case NT_NONE:
    case NT_OBJ:
      die("The node types NONE and OBJ should not occur for the parent of a list.");
      break;
    default:
      die("Unknown node type: %d", reader->current_type);
  }
}
//****************************************************************************
// The JSON parser read the end of a list
//****************************************************************************
void jsonchk_end_list(void *user_data) {
  JSONCHK_T *reader;
  JSON_LST_DEF_T *current_def, *lst_def;
  JSON_PROP_DEF_T *prop_def;
  void *temp;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) {
    reader->udepth--;
    return;
  }
  current_def = (JSON_LST_DEF_T*)reader->current_node;
  current_def->dimension--;
  if (current_def->dimension > 0) {
    current_def->index[current_def->dimension - 1]++; 
    return;
  }
  if (current_def->finalize != NULL) {
    temp = current_def->finalize(
        reader->user_data, get_owner_data(
          current_def->parent_type, current_def->parent_node, NULL),
        current_def->data);
    if (temp == NULL) {
      reader->has_error = true;
      return;
    }
    current_def->data = temp;
  }
  switch (current_def->parent_type) {
    case NT_LST:
      lst_def = (JSON_LST_DEF_T*)current_def->parent_node;
      if (lst_def->item_list != NULL) {
        if (!(lst_def->item_list(reader->user_data, lst_def->data, lst_def->index, current_def->data))) {
          reader->has_error = true;
          return;
        }
      }
      lst_def->index[lst_def->dimension - 1]++;
      current_def->data = NULL;
      reader->current_node = lst_def;
      reader->current_type = NT_LST;
      break;
    case NT_PROP:
      prop_def = (JSON_PROP_DEF_T*)current_def->parent_node;
      if (prop_def->prop_list != NULL) {
        if (!(prop_def->prop_list(reader->user_data, prop_def->parent_node->data, prop_def->name, current_def->data))) {
          reader->has_error = true;
          return;
        }
      }
      reader->current_node = prop_def;
      reader->current_type = NT_PROP;
      break;
    case NT_NONE:
    case NT_OBJ:
      die("The node types NONE and OBJ should not occur for the parent of a list.");
      break;
    default:
      die("Unknown node type: %d", current_def->parent_type);
  }
}
//****************************************************************************
// The JSON parser read a null value
//****************************************************************************
void jsonchk_null(void *user_data) {
  JSONCHK_T *reader;
  JSON_LST_DEF_T *lst_def;
  JSON_PROP_DEF_T *prop_def;
  bool ok;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) return;
  if (reader->current_type == NT_LST) {
    lst_def = (JSON_LST_DEF_T*)reader->current_node;
    if (lst_def->dimension == lst_def->dimensions && 
        (lst_def->allowed_types & ALLOW_NULL) != 0) {
      if (lst_def->item_null != NULL) {
        if (!(lst_def->item_null(reader->user_data, lst_def->data, lst_def->index))) {
          reader->has_error = true;
          return;
        }
      }
      lst_def->index[lst_def->dimension - 1]++;
    } else {
        error(reader, "List item %s should not be null", path(reader));
    }
  } else {
    assert(reader->current_type == NT_PROP);
    prop_def = (JSON_PROP_DEF_T*)reader->current_node;
    if ((prop_def->allowed_types & ALLOW_NULL) != 0) {
      if (prop_def->prop_null != NULL) {
        if (!(prop_def->prop_null(reader->user_data, prop_def->parent_node->data, prop_def->name))) {
          reader->has_error = true;
          return;
        }
      }
    } else {
      error(reader, "Property %s should not be null", path(reader));
    }
  }
}
//****************************************************************************
// The JSON parser read a boolean value
//****************************************************************************
void jsonchk_bool(void *user_data, bool value) {
  JSONCHK_T *reader;
  JSON_LST_DEF_T *lst_def;
  JSON_PROP_DEF_T *prop_def;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) return;
  if (reader->current_type == NT_LST) {
    lst_def = (JSON_LST_DEF_T*)reader->current_node;
    if (lst_def->dimension == lst_def->dimensions && 
        (lst_def->allowed_types & ALLOW_BOOL) != 0) {
      if (lst_def->item_bool != NULL) {
        if (!(lst_def->item_bool(reader->user_data, lst_def->data, lst_def->index, value))) {
          reader->has_error = true;
          return;
        }
      }
      lst_def->index[lst_def->dimension - 1]++;
    } else {
        error(reader, "List item %s should not be a boolean", path(reader));
    }
  } else {
    assert(reader->current_type == NT_PROP);
    prop_def = (JSON_PROP_DEF_T*)reader->current_node;
    if ((prop_def->allowed_types & ALLOW_BOOL) != 0) {
      if (prop_def->prop_bool != NULL) {
        if (!(prop_def->prop_bool(reader->user_data, prop_def->parent_node->data, prop_def->name, value))) {
          reader->has_error = true;
          return;
        }
      }
    } else {
      error(reader, "Property %s should not be a boolean", path(reader));
    }
  }
}
//****************************************************************************
// The JSON parser read a numeric value
//****************************************************************************
void jsonchk_number(void *user_data, long double value) {
  JSONCHK_T *reader;
  JSON_LST_DEF_T *lst_def;
  JSON_PROP_DEF_T *prop_def;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) return;
  if (reader->current_type == NT_LST) {
    lst_def = (JSON_LST_DEF_T*)reader->current_node;
    if (lst_def->dimension == lst_def->dimensions && 
        (lst_def->allowed_types & ALLOW_NUMBER) != 0) {
      if (lst_def->item_number != NULL) {
        if (!(lst_def->item_number(reader->user_data, lst_def->data, lst_def->index, value))) {
          reader->has_error = true;
          return;
        }
      }
      lst_def->index[lst_def->dimension - 1]++;
    } else {
        error(reader, "List item %s should not be a number", path(reader));
    }
  } else {
    assert(reader->current_type == NT_PROP);
    prop_def = (JSON_PROP_DEF_T*)reader->current_node;
    if ((prop_def->allowed_types & ALLOW_NUMBER) != 0) {
      if (prop_def->prop_number != NULL) {
        if (!(prop_def->prop_number(reader->user_data, prop_def->parent_node->data, prop_def->name, value))) {
          reader->has_error = true;
          return;
        }
      }
    } else {
      error(reader, "Property %s should not be a number", path(reader));
    }
  }
}
//****************************************************************************
// The JSON parser read a string value
//****************************************************************************
void jsonchk_string(void *user_data, char *value, size_t value_len) {
  JSONCHK_T *reader;
  JSON_LST_DEF_T *lst_def;
  JSON_PROP_DEF_T *prop_def;
  reader = (JSONCHK_T*)user_data;
  if (reader->has_error) return;
  if (reader->udepth > 0) return;
  if (reader->current_type == NT_LST) {
    lst_def = (JSON_LST_DEF_T*)reader->current_node;
    if (lst_def->dimension == lst_def->dimensions &&
        (lst_def->allowed_types & ALLOW_STRING) != 0) {
      if (lst_def->item_string != NULL) {
        if (!(lst_def->item_string(reader->user_data, lst_def->data, lst_def->index, value, value_len))) {
          reader->has_error = true;
          return;
        }
      }
      lst_def->index[lst_def->dimension - 1]++;
    } else {
        error(reader, "List item %s should not be a string", path(reader));
    }
  } else {
    assert(reader->current_type == NT_PROP);
    prop_def = (JSON_PROP_DEF_T*)reader->current_node;
    if ((prop_def->allowed_types & ALLOW_STRING) != 0) {
      if (prop_def->prop_string != NULL) {
        if (!(prop_def->prop_string(reader->user_data, prop_def->parent_node->data, prop_def->name, value, value_len))) {
          reader->has_error = true;
          return;
        }
      }
    } else {
      error(reader, "Property %s should not be a string", path(reader));
    }
  }
}

//****************************************************************************
// Create a JSON reader with document schema
// When this object is destroyed it also destroys the schema it uses.
//****************************************************************************
JSONCHK_T* jsonchk_create(void (*error)(void *, char *, va_list), 
    void *user_data, JSON_OBJ_DEF_T *schema) {
  JSONRD_CALLBACKS_T callbacks;
  JSONCHK_T* reader;
  // configure schema parent node
  schema->parent_node = NULL;
  schema->parent_type = NT_NONE;
  // configure callbacks to use the definition testing functions
  callbacks.error = jsonchk_error;
  callbacks.start_data = jsonchk_start_data;
  callbacks.end_data = jsonchk_end_data;
  callbacks.start_property = jsonchk_start_property;
  callbacks.end_property = jsonchk_end_property;
  callbacks.start_object = jsonchk_start_object;
  callbacks.end_object = jsonchk_end_object;
  callbacks.start_list = jsonchk_start_list;
  callbacks.end_list = jsonchk_end_list;
  callbacks.atom_null = jsonchk_null;
  callbacks.atom_bool = jsonchk_bool;
  callbacks.atom_number = jsonchk_number;
  callbacks.atom_string = jsonchk_string;
  // allocate memory for the JSONCHK_T
  reader = mm_malloc(sizeof(JSONCHK_T));
  memset(reader, 0, sizeof(JSONCHK_T));
  reader->error = error;
  reader->has_error = false;
  reader->user_data = user_data;
  reader->schema = schema;
  reader->current_node = NULL;
  reader->current_type = NT_NONE;
  reader->udepth = 0;
  reader->path_buf = str_create(0);
  reader->parser = jsonrd_create(&callbacks, reader);
  return reader;
}

//****************************************************************************
// Destroy any unreturned data
//****************************************************************************
static inline void jsonchk_data_destroy(JSONCHK_T* reader) {
  void *node;
  NT_EN type;
  JSON_OBJ_DEF_T *obj_def;
  JSON_LST_DEF_T *lst_def;
  node = reader->current_node;
  type = reader->current_type;
  while (node != NULL) {
    switch (type) {
      case NT_NONE:
        node = NULL;
        break;
      case NT_OBJ:
        obj_def = ((JSON_OBJ_DEF_T*)node);
        if (obj_def->abort) {
          obj_def->abort(obj_def->data);
        }
        obj_def->data = NULL;
        type = obj_def->parent_type;
        node = obj_def->parent_node;
        break;
      case NT_LST:
        lst_def = ((JSON_LST_DEF_T*)node);
        if (lst_def->abort) {
          lst_def->abort(lst_def->data);
        }
        lst_def->data = NULL;
        type = lst_def->parent_type;
        node = lst_def->parent_node;
        break;
      case NT_PROP:
        node = ((JSON_PROP_DEF_T*)node)->parent_node;
        type = NT_OBJ;
        break;
    }
  }
  reader->current_node = NULL;
  reader->current_type = NT_NONE;
}

//****************************************************************************
// Destroy a JSON reader with document schema
//****************************************************************************
void jsonchk_destroy(JSONCHK_T* reader) {
  jsonchk_data_destroy(reader);
  jsonrd_destroy(reader->parser);
  str_destroy(reader->path_buf, false);
  jd_obj_destroy(reader->schema);
  memset(reader, 0, sizeof(JSONCHK_T));
  free(reader);
}

//****************************************************************************
// Update a JSON reader with document schema
//****************************************************************************
void jsonchk_update(JSONCHK_T *reader, const char *chunk, size_t size, bool end) {
  jsonrd_update(reader->parser, chunk, size, end);
}
