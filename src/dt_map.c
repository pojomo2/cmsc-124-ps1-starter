/*
 * dt_map.c: Associative arrays for Unit 5, Section E.
 *
 * An array does not store its indices. This map stores its keys.
 * An array calculates a position with one subtraction.
 * The map calculates a hash and then compares keys in one bucket.
 *
 * Hashing turns the key into a bucket number. Compare all keys in that bucket
 * because two keys can select it. Use a linked list for each bucket. Start the
 * unsigned accumulator at 14695981039346656037ULL. For each unsigned byte,
 * exclusive-or the byte into it and multiply by 1099511628211ULL.
 *
 * A separate list stores insertion order for stable output. dt_map_key_at
 * reads this list. Updating a key preserves its position. Removing and
 * reinserting a key moves it to the end.
 */

#include "dt.h"

#include <stdlib.h>
#include <string.h>

#define BUCKET_COUNT 16

typedef struct map_entry {
    char *key;
    dt_value value;
    struct map_entry *next;
} map_entry;

struct dt_map {
    /* Add the buckets and insertion-order data. */
    map_entry **buckets;
    size_t bucket_count;

    map_entry **order;
    size_t entry_count;
    size_t order_capacity;
};

// 64-bit FNV-1a hash
 /* Start the unsigned accumulator at 14695981039346656037ULL. 
 * For each unsigned byte, exclusive-or the byte into it and multiply by 1099511628211ULL.*/

 static size_t hash_key(const char *key) {

    unsigned long long h = 14695981039346656037ULL;
    for (const unsigned char *p = (const unsigned char *)key; *p != '\0'; p++) {
        h ^= (unsigned long long)*p;
        h *= 1099511628211ULL;
    }
    return (size_t)h;
}

// reduce a key to a bucket index
static size_t bucket_index(const dt_map *m, const char *key) {
    return hash_key(key) % m->bucket_count;
}

// traverse bucket chain 
/* Compare all keys in that bucket because two keys can select it.*/

 static map_entry *find_in_bucket(const dt_map *m, size_t index, const char *key) {
    map_entry *e = m->buckets[index];
    while (e != NULL) {
        if (strcmp(e->key, key) == 0) {
            return e;
        }
        e = e->next;
    } 
    return NULL;
 }

/*
 * dt_map_new builds an empty map. It returns NULL after an allocation failure.
 */
dt_map *dt_map_new(void)
{
    /* Return an allocated empty map. Return NULL after an allocation failure.
       dt_map_new()  -> a map whose dt_map_len is 0
       cases/normal/map_basics.case */

    dt_map *m = calloc(1, sizeof *m); //default values set to 0

    if (m == NULL) {
        return NULL;
    }

    m->bucket_count = BUCKET_COUNT;
    m->buckets = calloc(m->bucket_count, sizeof *m->buckets);
    if (m->buckets == NULL) {
        free(m);
        return NULL;
    } 
    return m;
}

/*
 * dt_map_free releases each entry, copied key, order array, and map.
 * It accepts NULL. The environment owns the values.
 */
void dt_map_free(dt_map *m)
{
    /* Release each entry, copied key, order array, and map.
       Preserve the values. The environment owns them.
       a map holding a string value  -> the nodes and keys go, the string stays
       dt_map_free(NULL)             -> returns, having done nothing
       cases/cleanup/map_churn.case */
    
    if (m == NULL) {
        return;
    }

    for (size_t i = 0; i < m->bucket_count; i++) {
        map_entry *e = m->buckets[i];
        while (e != NULL) {
            map_entry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(m->buckets);
    free(m->order);
    free(m);
}

/*
 * dt_map_len returns the number of keys in constant time.
 */
size_t dt_map_len(const dt_map *m)
{
    /* Return the current key entry_count.
       Replacing a value does not change this entry_count.
       after put alpha, beta, gamma:  dt_map_len(m) -> 3
       after put beta again:          dt_map_len(m) -> 3, still
       after del alpha:               dt_map_len(m) -> 2
       cases/normal/map_basics.case */
    
    return m->entry_count;
}

/*
 * dt_map_put binds v to key.
 * An existing key keeps its insertion position. A new key becomes the last key.
 * Copy each new key because the caller owns the source buffer.
 * Return DT_ERR_CAPACITY after an allocation failure.
 */
dt_status dt_map_put(dt_map *m, const char *key, dt_value v)
{
    /* Replace the value for an existing key.
       Add a new entry for a new key. Copy each new key.
       Hash the key. Select its bucket. Search the bucket chain.
       Add a new entry to the chain and insertion list.
       put "beta" -> 2 on an empty map    -> DT_OK, "beta" is last in order
       put "beta" -> 22 on that map       -> DT_OK, same position, new value
       an allocation failure              -> DT_ERR_CAPACITY, map unchanged
       cases/normal/map_basics.case */
    
    size_t index = bucket_index(m, key);
    map_entry *existing = find_in_bucket(m, index, key);
    if (existing != NULL) {
        existing->value = v;
        return DT_OK;
    }

    map_entry *e = malloc(sizeof *e);
    if (e == NULL) {
        return DT_ERR_CAPACITY;
    }

    size_t key_len = strlen(key);
    e->key = malloc(key_len+1);
    if (e->key == NULL) {
        free(e);
        return DT_ERR_CAPACITY;
    }   
    memcpy(e->key, key, key_len + 1);

    if (m->entry_count == m->order_capacity) { //full slots
        // grow order_capacity, if cap != 0: double, otherwise start at 8
        size_t new_capacity = m->order_capacity ? m->order_capacity * 2 : 8;
        //resize allocation
        map_entry **new_order = realloc(m->order, new_capacity * sizeof *new_order);
        //realloc failure
        if (new_order == NULL) {
            free(e->key);
            free(e);
            return DT_ERR_CAPACITY;
        }
        // write to m
        m->order = new_order;
        m->order_capacity = new_capacity;
    }

    e->value = v; // store entry's value
    // link entry to bucket chain head
    e->next = m->buckets[index];
    m->buckets[index] = e;
    // add entry to insertion order list
    m->order[m->entry_count] = e;
    m->entry_count++;

    return DT_OK;
}

/*
 * dt_map_get writes the value for key to *out.
 * It returns DT_ERR_KEY and does not change *out when the key is absent.
 * An absent key differs from a nil value.
 */
dt_status dt_map_get(const dt_map *m, const char *key, dt_value *out)
{
    /* Return DT_ERR_KEY when the key is absent.
       Preserve *out after this error. A nil value can be present.
       after put "beta" -> 22:
         dt_map_get(m, "beta", &out)   -> DT_OK, *out is the integer 22
         dt_map_get(m, "ghost", &out)  -> DT_ERR_KEY, *out untouched
       cases/normal/map_basics.case, cases/boundary/map_missing_key.case */
    
    size_t index = bucket_index(m, key);
    map_entry *e = find_in_bucket(m, index, key);
    if (e == NULL) {
        return DT_ERR_KEY;
    }

    *out = e->value;
    return DT_OK;
}

/*
 * dt_map_remove removes key from its bucket and insertion position.
 * It releases the copied key. It returns DT_ERR_KEY when the key is absent.
 */
dt_status dt_map_remove(dt_map *m, const char *key)
{
    /* Remove the entry from its bucket and insertion position.
       Release the copied key. Return DT_ERR_KEY when the key is absent.
       a map holding alpha, beta, gamma:
         dt_map_remove(m, "alpha")  -> DT_OK, order is now beta, gamma
         dt_map_remove(m, "ghost")  -> DT_ERR_KEY, nothing changes
       reinserting "alpha" appends it after "gamma"
       cases/normal/map_basics.case, cases/boundary/map_remove_missing_key.case */

    size_t index = bucket_index(m, key); //select bucket
    map_entry **slot = &m->buckets[index];  // slot = address of slot that holds pointer to current cell

    // traverse bucket chain
    while (*slot != NULL) {
        if (strcmp((*slot)->key, key) == 0) {
            map_entry *e = *slot;   //save cell to remove
            *slot = e->next;    //skip cell, go to next

            // traverse insertion order list
            for (size_t i = 0; i < m->entry_count; i++) {
                if (m->order[i] == e) {     //position located
                    for (size_t j = i; j + 1 < m->entry_count; j++) {
                        m->order[j] = m->order[j + 1];  //left shift entries after e 
                    }
                    break;
                }
            }
            m->entry_count--;
            free(e->key);
            free(e);
            return DT_OK;
        }
        slot = &(*slot)->next; //no match, slot moves to next field
    }

    return DT_ERR_KEY;
}

/*
 * dt_map_key_at writes the key at insertion position index to *out.
 * It returns DT_ERR_RANGE and does not change *out for an invalid index.
 */
dt_status dt_map_key_at(const dt_map *m, size_t index, const char **out)
{
    /* Write the key at the specified insertion position to *out.
       Return DT_ERR_RANGE for an invalid position. Preserve *out after this error.
       The printer uses this order.
       a map holding alpha, beta, gamma:
         dt_map_key_at(m, 0, &out)  -> DT_OK, *out = "alpha"
         dt_map_key_at(m, 3, &out)  -> DT_ERR_RANGE, *out untouched
       cases/normal/map_basics.case */

    if (index >= m->entry_count) {
        return DT_ERR_RANGE;
    }

    *out = m->order[index]->key;
    return DT_OK;
}