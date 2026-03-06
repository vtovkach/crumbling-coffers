/*
 * ds_lib-c - Data Structures Library
 * Designed and implemented by Vadym Tovkach
 * Repository: https://github.com/vtovkach/ds-lib-c/tree/main
 *
 * Copyright (c) 2025 Vadym Tovkach
 * Licensed under the MIT License.
 *
 */

#ifndef _HASHTABLE_
#define _HASHTABLE_

typedef unsigned int (*hsh_func)(const void *, unsigned int);

typedef struct Node
{
    void  *value;
    void  *key;
    struct Node  *nextNode;
} Node;

typedef struct HashTable
{
    size_t          value_size;         // Size of a single data block for the value (in bytes)
    size_t          key_size;           // Size of a single data block for the key (in bytes)
    size_t          value_blocks;       // Number of blocks that make up the value
    size_t          key_blocks;         // Number of blocks that make up the key
    Node            **hash_table;       // Array with Nodes 
    unsigned int    num_elements;       // Current number of elements in the hash table 
    unsigned int    hash_table_size;    // Maximum number of elements in a hash_table 
    double          load_factor;
    hsh_func        hash_function;      // User-implemented hash function 
} HashTable;

// C23 typeof 

typedef struct HashTable HashTable;

HashTable *ht_create(size_t key_size, size_t key_blocks, size_t data_size, size_t data_blocks, hsh_func hash_func_ptr, unsigned int init_size);

void ht_destroy(HashTable *hashtable);

// ============================ Internal Functions =============================

int ht__insert_internal(HashTable *hashtable, const void *key, const void *value);

int ht__remove_internal(HashTable *hashtable, const void *key);

void *ht__get_internal(const HashTable *hashtable, const void *key, size_t bytes_comp);

// ================================ Public API ==================================


#define ht_insert(hashtable, key, value) \
    ht__insert_internal((hashtable), &(__typeof__(key)){(key)}, &(__typeof__(value)){(value)})

#define ht_remove(hashtable, key) \
    ht__remove_internal((hashtable), &(__typeof__(key)){(key)})

#define ht_get(hashtable, key, bytes_comp)    \
    ht__get_internal((hashtable), &(__typeof__(key)){(key)}, bytes_comp)


#endif 