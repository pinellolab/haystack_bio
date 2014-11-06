/**************************************************************************
 * FILE: json-checker.h
 * AUTHOR: James Johnson 
 * CREATE DATE: 26 February 2012
 * PROJECT: shared
 * COPYRIGHT: UQ, 2013
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: Provides a interface for checking that JSON data follows
 * a specified structure and extracting data from that structure.
 **************************************************************************/

#ifndef JSON_CHECKER_H
#define JSON_CHECKER_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct jsonchk JSONCHK_T;

/*****************************************************************************
 * Callbacks for use with the JSON checker.
 ****************************************************************************/

// Initilize whatever structure is need for the object or list. The returned
// structure will be passed in as the owner for any properties/items. Returning
// NULL indicates a error and will cause the parser to stop generating events.
// The index parameter is only set when the parent is a list.
typedef void* (*JsonSetupFn)(void* user_data, void* owner, const int *index);

// Finish all checks on this object or list and convert the structure into the
// form needed for returning it to the owner. Returning NULL indicates a error
// and will cause the parser to stop generating events. Additionally the
// object/list will not be considered finalized for the purposes of cleanup.
// The index parameter is only set when the parent is a list.
typedef void* (*JsonFinalizeFn)(void* user_data, void* owner, void* self);

// Destroy a object or list created by setup if an error occurs before it is
// passed to the owner. Any objects or lists passed to the owner are
// expected to be handled as part of the owner for cleanup.
typedef void (*JsonAbortFn)(void* self);

// Handle a list item. Different functions are called depending on the type of
// the list item. Returning false indicates a error and will cause the parser to
// stop generating events.
typedef bool (*JsonListNullFn)(void *user_data, void *list, const int *index);
typedef bool (*JsonListBoolFn)(void *user_data, void *list, const int *index, bool value);
typedef bool (*JsonListNumberFn)(void *user_data, void *list, const int *index, long double value);
typedef bool (*JsonListStringFn)(void *user_data, void *list, const int *index, const char *value, size_t value_length);
typedef bool (*JsonListObjectFn)(void *user_data, void *list, const int *index, void *value);
typedef bool (*JsonListListFn)(void *user_data, void *list, const int *index, void *value);

// Handle a property. Different functions are called depending on the type of
// the property. Returning false indicates a error and will cause the parser to
// stop generating events.
typedef bool (*JsonPropNullFn)(void *user_data, void *object, const char *property);
typedef bool (*JsonPropBoolFn)(void *user_data, void *object, const char *property, bool value);
typedef bool (*JsonPropNumberFn)(void *user_data, void *object, const char *property, long double value);
typedef bool (*JsonPropStringFn)(void *user_data, void *object, const char *property, const char *value, size_t value_length);
typedef bool (*JsonPropObjectFn)(void *user_data, void *object, const char *property, void *value);
typedef bool (*JsonPropListFn)(void *user_data, void *object, const char *property, void *value);

// Define the structure of JSON sections
typedef struct json_obj_def JSON_OBJ_DEF_T;
typedef struct json_lst_def JSON_LST_DEF_T;
typedef struct json_prop_def JSON_PROP_DEF_T;

/*****************************************************************************
 * Create a JSON reader with document schema
 * When this object is destroyed it also destroys the schema it uses.
 ****************************************************************************/
JSONCHK_T* jsonchk_create(void (*error)(void *, char *, va_list), 
    void *user_data, JSON_OBJ_DEF_T *schema);

/*****************************************************************************
 * Destroy a JSON reader with document schema
 ****************************************************************************/
void jsonchk_destroy(JSONCHK_T* parser);

/*****************************************************************************
 * Update a JSON reader with document schema
 ****************************************************************************/
void jsonchk_update(JSONCHK_T *reader, const char *chunk, size_t size, bool end);

/*****************************************************************************
 * Define the structure of the JSON document and supply callbacks for parts.
 * Note that all callbacks are optional which is useful if for example you just
 * want to validate the structure of the document.
 ****************************************************************************/
// Create a JSON object definition
JSON_OBJ_DEF_T* jd_obj(JsonSetupFn setup, JsonFinalizeFn finalize,
    JsonAbortFn abort, bool ignore_unknown, int property_count, ...);
// Create a JSON boolean property definition
JSON_PROP_DEF_T* jd_pbool(char *name, bool required, JsonPropBoolFn prop_bool);
// Create a JSON numeric property definition
JSON_PROP_DEF_T* jd_pnum(char *name, bool required, JsonPropNumberFn prop_number);
// Create a JSON string property definition
JSON_PROP_DEF_T* jd_pstr(char *name, bool required, JsonPropStringFn prop_string);
// Create a JSON list property definition
JSON_PROP_DEF_T* jd_plst(char *name, bool required,
    JsonPropListFn prop_list, JSON_LST_DEF_T *lst_def);
// Create a JSON object property definition
JSON_PROP_DEF_T* jd_pobj(char *name, bool required,
    JsonPropObjectFn prop_object, JSON_OBJ_DEF_T *obj_def);
// Create a JSON list of numbers definition
JSON_LST_DEF_T* jd_lnum(int dimensions, JsonSetupFn setup,
    JsonFinalizeFn finalize, JsonAbortFn abort, JsonListNumberFn item_number);
// Create a JSON list of strings definition
JSON_LST_DEF_T* jd_lstr(int dimensions, JsonSetupFn setup,
    JsonFinalizeFn finalize, JsonAbortFn abort, JsonListStringFn item_string);
// Create a JSON list of lists definition
JSON_LST_DEF_T* jd_llst(int dimensions, JsonSetupFn setup,
    JsonFinalizeFn finalize, JsonAbortFn abort, JsonListListFn item_list,
    JSON_LST_DEF_T *lst_def);
// Create a JSON list of objects definition
JSON_LST_DEF_T* jd_lobj(int dimensions, JsonSetupFn setup,
    JsonFinalizeFn finalize, JsonAbortFn abort, JsonListObjectFn item_object,
    JSON_OBJ_DEF_T *obj_def);

#endif
