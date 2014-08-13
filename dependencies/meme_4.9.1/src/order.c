/***********************************************************************
 * FILE: order.c
 * AUTHOR: William Stafford Noble
 * PROJECT: MHMM
 * COPYRIGHT: 2001-2008, WSN
 * DESCRIPTION: Data structure for representing the order and spacing
 *              of motifs in a linear model.
 ***********************************************************************/
#include <assert.h>
#include <limits.h>
#include <string.h>

#include "order.h"
#include "regex-utils.h"

int END_TRANSITION = INT_MAX;

/*
 * The motifs are indexed from 1 to n.  In motif_spacers, the value at
 * position i is the length of the spacer following motif i, so
 * position 0 contains the length of the spacer before the first
 * motif.
 * */
struct order {
  double pvalue;      // p-value of sequence from which this order and spacing was derived.
  int num_occurs;     // Total number of motif occurrences. 
  int num_alloc;      // Number of motif occurrence slots.
  RBTREE_T *distinct; // Set of motif indexes
  int *motif_occurs;  // Order of motif occurrences. Negative for rc.
  int *motif_spacers; // Distance between motif occurrences. 
};

/***********************************************************************
 * Create an order-and-spacing object from a user-specified string.
 * <spacer>=<strand><motif #>=<spacer>=<strand><motif #>=<spacer>
 * eg
 * 5=+1=12=-1=15
 ***********************************************************************/
ORDER_T* create_order
  (const char* order_string)
{
  ORDER_T* order;
  int num_equals, i, i_spacer, i_motif, motif;
  regex_t regex_sp_eq, regex_mn_eq, regex_sp;
  regmatch_t matches[2];
  const char *str;


  // Don't do anything if no string was given. 
  if (order_string == NULL) {
    return NULL;
  }

  regcomp_or_die("<spacer>=", &regex_sp_eq, 
      "^[[:space:]]*([0-9]+)[[:space:]]*=", REG_EXTENDED);
  regcomp_or_die("<motif #>=", &regex_mn_eq, 
      "^[[:space:]]*([+-]?[0-9]+)[[:space:]]*=", REG_EXTENDED);
  regcomp_or_die("<spacer>", &regex_sp, 
      " ^[[:space:]]*([0-9]+)[[:space:]]*$", REG_EXTENDED);


  for (str = order_string, num_equals = 0; *str != '\0'; ++str) {
    if (*str == '=') ++num_equals;
  }
  // Allocate the data structure. 
  if (num_equals % 2 != 0) die("Error reading order and spacing. "
      "The spacing string must have an even number of seperators (=).");
  order = create_empty_order(num_equals / 2, 0.0);

  str = order_string;
  i_spacer = 0;
  i_motif = 0;
  for (i = 0; i < num_equals; i++) {
    if ((i % 2) == 0) { // spacer
      if (regexec_or_die("spacer", &regex_sp_eq, str, 2, matches, 0)) {
        order->motif_spacers[i_spacer++] = regex_int(matches+1, str, 0);
      } else {
        die("Error reading order and spacing at spacer %d.\n", i_spacer + 1);
      }
    } else { // motif
      if (regexec_or_die("motif num", &regex_mn_eq, str, 2, matches, 0)) {
        motif = regex_int(matches+1, str, 0);
        if (motif == 0 || motif == END_TRANSITION) 
          die("Error reading order and spacing at motif %d.\n", i_motif + 1);
        order->motif_occurs[i_motif++] = motif;
        order->num_occurs += 1;
        rbtree_make(order->distinct, &motif, NULL);
      } else {
        die("Error reading order and spacing at motif %d.\n", i_motif + 1);
      }
    }
    str += matches[0].rm_eo;
  }
  if (regexec_or_die("spacer", &regex_sp, str, 2, matches, 0)) {
    order->motif_spacers[i_spacer++] = regex_int(matches+1, str, 0);
  } else {
    die("Error reading order and spacing at spacer %d.\n", i_spacer + 1);
  }
  assert((order->num_alloc == order->num_occurs));

  // Tell the user. 
  if (verbosity >= NORMAL_VERBOSE) {
    fprintf(stderr, "Using order: ");
    print_order_and_spacing(stderr, order);
  }

  return(order);
}

/***********************************************************************
 * Create an order-and-spacing object from a user-specified string.
 * <spacer>=<strand><motif ID>=<spacer>=<strand><motif ID>=<spacer>
 * eg
 * 5=+m1=12=-m1=15
 ***********************************************************************/
ORDER_T* create_order_from_ids
  (const char* order_string, RBTREE_T *ids2nums)
{
  ORDER_T* order;
  RBNODE_T *node;
  int num_equals, i, i_spacer, i_motif, motif;
  regex_t regex_sp_eq, regex_mn_eq, regex_sp;
  regmatch_t matches[2];
  const char *str;
  char motif_id_buf[MAX_MOTIF_ID_LENGTH + 2];
  char *motif_id, strand;

  // Don't do anything if no string was given. 
  if (order_string == NULL) {
    return NULL;
  }

  regcomp_or_die("<spacer>=", &regex_sp_eq, 
      "^[[:space:]]*([0-9]+)[[:space:]]*=", REG_EXTENDED);
  regcomp_or_die("<motif id>=", &regex_mn_eq, 
      "^[[:space:]]*([+-]?[^=[:space:]]+)[[:space:]]*=", REG_EXTENDED);
  regcomp_or_die("<spacer>", &regex_sp, 
      " ^[[:space:]]*([0-9]+)[[:space:]]*$", REG_EXTENDED);


  for (str = order_string, num_equals = 0; *str != '\0'; ++str) {
    if (*str == '=') ++num_equals;
  }
  // Allocate the data structure. 
  if (num_equals % 2 != 0) die("Error reading order and spacing. "
      "The spacing string must have an even number of seperators (=).");
  order = create_empty_order(num_equals / 2, 0.0);

  str = order_string;
  i_spacer = 0;
  i_motif = 0;
  for (i = 0; i < num_equals; i++) {
    if ((i % 2) == 0) { // spacer
      if (regexec_or_die("spacer", &regex_sp_eq, str, 2, matches, 0)) {
        order->motif_spacers[i_spacer++] = regex_int(matches+1, str, 0);
      } else {
        die("Error reading order and spacing at spacer %d.\n", i_spacer + 1);
      }
    } else { // motif
      if (regexec_or_die("motif", &regex_mn_eq, str, 2, matches, 0)) {
        // copy the motif id and strand
        regex_strncpy(matches+1, str, motif_id_buf, MAX_MOTIF_ID_LENGTH + 2);
        // get the strand
        if (motif_id_buf[0] == '+' || motif_id_buf[0] == '-') {
          strand = motif_id[0];
          motif_id = motif_id_buf+1;
        } else {
          strand = '+';
          motif_id = motif_id_buf;
        }
        // find the number (if it exists)
        node = rbtree_find(ids2nums, motif_id);
        if (node == NULL)
          die("Error reading order and spacing at motif %d: "
              "Motif ID \"%s\" unknown.\n", i_motif + 1, motif_id);
        motif = *((int*)rbtree_value(node));
        order->motif_occurs[i_motif++] = motif;
        order->num_occurs += 1;
        rbtree_make(order->distinct, &motif, NULL);
      } else {
        die("Error reading order and spacing at motif %d.\n", i_motif + 1);
      }
    }
    str += matches[0].rm_eo;
  }
  if (regexec_or_die("spacer", &regex_sp, str, 2, matches, 0)) {
    order->motif_spacers[i_spacer++] = regex_int(matches+1, str, 0);
  } else {
    die("Error reading order and spacing at spacer %d.\n", i_spacer + 1);
  }
  assert((order->num_alloc == order->num_occurs));

  // Tell the user. 
  if (verbosity >= NORMAL_VERBOSE) {
    fprintf(stderr, "Using order: ");
    print_order_and_spacing(stderr, order);
  }

  return(order);
  
}

/***********************************************************************
 * Create an empty order-and-spacing object.
 ***********************************************************************/
ORDER_T* create_empty_order
  (int    num_alloc,
   double sequence_p)
{
  ORDER_T* new_order;

  // Allocate the data structure. 
  new_order = (ORDER_T*)mm_malloc(sizeof(ORDER_T));

  new_order->num_alloc = num_alloc;
  new_order->num_occurs = 0;
  new_order->pvalue = sequence_p;

  // Allocate the set. 
  new_order->distinct = rbtree_create(rbtree_intcmp, rbtree_intcpy, free, NULL, NULL);

  // Allocate the arrays. 
  new_order->motif_occurs = (int*)mm_malloc(sizeof(int) * (num_alloc));
  memset(new_order->motif_occurs, 0, sizeof(int) * (num_alloc));
  // Add 1 for end transition.
  new_order->motif_spacers = (int*)mm_malloc(sizeof(int) * (num_alloc + 1));
  memset(new_order->motif_spacers, 0, sizeof(int) * (num_alloc + 1));

  return new_order;
}

/***********************************************************************
 * Add one spacer and the following motif to an order object.
 *
 * Does nothing if the order parameter is NULL.
 ***********************************************************************/
void add_occurrence
  (int motif_i,
   int spacer_length,
   ORDER_T* order)
{

  if (order == NULL) {
    return;
  }

  assert(order->num_alloc >= order->num_occurs);

  // Count distinct motifs.
  if (!order_contains(motif_i, order)) {
    rbtree_make(order->distinct, &motif_i, NULL);
  }

  // Record the spacer before this occurrence.
  order->motif_spacers[order->num_occurs] = spacer_length;

  // Don't include the end transition as a motif occurence
  if (motif_i != END_TRANSITION) {
    assert(order->num_alloc > order->num_occurs);
    order->motif_occurs[order->num_occurs] = motif_i;
    (order->num_occurs)++;
  }
}

/***********************************************************************
 * Determine whether a given motif order-and-spacing object contains a
 * given motif index (where index counts from 1). Note reverse complement
 * motifs are given negative numbers.
 ***********************************************************************/
BOOLEAN_T order_contains
  (int motif_i, ORDER_T* order)
{
  return rbtree_find(order->distinct, &motif_i) != NULL;
}


/***********************************************************************
 * Get the number of motif occurences 
 ***********************************************************************/
int get_order_occurs
  (ORDER_T* order) 
{
  return order->num_occurs;
}

/***********************************************************************
 * Get the length of the spacer after a given motif.  Motifs are
 * indexed from 1, so 0 gets the initial spacer.
 ***********************************************************************/
int get_spacer_length
  (ORDER_T* order,
   int      index)
{
  assert(index <= order->num_occurs);
  return(order->motif_spacers[index]);
}

/***********************************************************************
 * Get the motif number at a position.
 ***********************************************************************/
int get_order_motif
  (ORDER_T* order,
   int      index)
{
  assert(index < order->num_occurs);
  return(order->motif_occurs[index]);
}

/***********************************************************************
 * Get and set the p-value of a given order object.
 *
 * The p-value of a null object is 1.0.
 ***********************************************************************/
void set_pvalue
  (double pvalue,
   ORDER_T* order)
{
  assert(order != NULL);
  order->pvalue = pvalue;
}

double get_pvalue
  (ORDER_T* order)
{
  if (order == NULL) {
    return(1.0);
  }
  return order->pvalue;
}

/***********************************************************************
 * Get the number of distinct motif occurrences. Reverse complement 
 * motifs are counted as distinct.
 ***********************************************************************/
int get_num_distinct
  (ORDER_T* order)
{
  if (order == NULL) {
    return(0);
  }
  return rbtree_size(order->distinct);
}


/***********************************************************************
 * Re-order the motifs in the order in which they will appear in the model.
 ***********************************************************************/
/*
void reorder_motifs
  (ORDER_T*  order,
   ARRAYLST_T *motifs)
{
  MOTIF_T* temp_motifs; // Pointer to a copying buffer.
  MOTIF_T *motif;
  int      i_motif;     // Index of the desired motif.
  int      i_order;     // Index into the order object.

  assert(order != NULL);
  
  // Allocate memory for a temporary copy of the motif array.
  temp_motifs = (MOTIF_T *)mm_malloc(sizeof(MOTIF_T) * order->num_alloc);

  // Print the original list.
  if (verbosity >= NORMAL_VERBOSE) {
    fprintf(stderr, "Motifs:");
    for (i_motif = 0; i_motif < arraylst_size(motifs); i_motif++) {
      motif = (MOTIF_T*)arraylst_get(i_motif, motifs);
      fprintf(stderr, " %s", get_motif_id(motif));
    }
    fprintf(stderr, ".\n");
    fprintf(stderr, "Motif order: ");
    print_order_and_spacing(stderr, order);
  }

  // Copy the original motifs into the temporary space in the right order.
  for (i_order = 0; i_order < order->num_occurs; i_order++) {
    BOOLEAN_T found_motif_token = FALSE;
    char *order_motif_token = get_nth_string(i_order, order->motif_occurs);
    for (i_motif = 0; i_motif < arraylst_size(motifs); i_motif++) {
      if (strcmp(motifs[i_motif].id, order_motif_token) == 0) {
        found_motif_token = TRUE;
        copy_motif(&(motifs[i_motif]), &(temp_motifs[i_order]));
        break;
      }
    }
    if (found_motif_token == FALSE) {
      die(
          "The motif %s in the order string was not "
          "found in the set of allowed motifs.",
          order_motif_token
      );
    }
  }
  // Free the original motifs since we've copied them 
  // and are about to write over them. (free_motif actually frees
  // the frequency matrix contained in the motif).
  for (i_motif = 0; i_motif < *num_motifs; i_motif++) {
    free_motif(&(motifs[i_motif])); 
  }

  // Copy back into the original place.
  for (i_order = 0; i_order < order->num_occurs; i_order++) {
    copy_motif(&(temp_motifs[i_order]), &(motifs[i_order]));
    free_motif(&(temp_motifs[i_order]));
  }
  *num_motifs = order->num_occurs;

  // Print the re-ordered list.
  if (verbosity >= NORMAL_VERBOSE) {
    fprintf(stderr, "Re-ordered motifs: ");
    for (i_motif = 0; i_motif < *num_motifs; i_motif++) {
      fprintf(stderr, "%s ", motifs[i_motif].id);
    }
    fprintf(stderr, "\n");
  }

  // Free the temporary space.
  myfree(temp_motifs);
}
*/

/***********************************************************************
 * Print the order and spacing with equal signs between.
 ***********************************************************************/
void print_order_and_spacing
  (FILE*    outfile,
   ORDER_T* order)
{
  int i_order;

  fprintf(outfile, "%d", order->motif_spacers[0]);
  for (i_order = 0; i_order < order->num_occurs; i_order++) {
    fprintf(outfile, "=%d=%d", order->motif_occurs[i_order], 
        order->motif_spacers[i_order + 1]);
  }
  fprintf(outfile, "\n");
}

/***********************************************************************
 * Free dynamic memory used by an order object.
 ***********************************************************************/
void free_order
  (ORDER_T* order)
{
  /* Don't deallocate an empty struct. */
  if (order == NULL) {
    return;
  }

  rbtree_destroy(order->distinct);
  myfree(order->motif_occurs);
  myfree(order->motif_spacers);
  myfree(order);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * End:
 */

