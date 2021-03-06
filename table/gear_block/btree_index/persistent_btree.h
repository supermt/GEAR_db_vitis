//
// Created by jinghuan on 6/15/21.
//

#pragma once

#include <rocksdb/slice.h>

#include <algorithm>
#include <functional>
#include <utility>
// Copyright (C) 2019 zn
//
// This file is part of btree.
//
// btree is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// btree is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with btree.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __BTREE_H__
#define __BTREE_H__

#include <stdint.h>
namespace ROCKSDB_NAMESPACE {

#ifdef __CHECKER__
#define FORCE __attribute__((force))
#else
#define FORCE
#endif

#ifdef __CHECKER__
#define BITWISE __attribute__((bitwise))
#else
#define BITWISE
#endif

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

typedef uint16_t BITWISE __be16; /* big endian, 16 bits */
typedef uint32_t BITWISE __be32; /* big endian, 32 bits */
typedef uint64_t BITWISE __be64; /* big endian, 64 bits */

#define SHA1_LENGTH 30

#define CACHE_SLOTS 23 /* prime */

struct btree_item {
  uint8_t sha1[SHA1_LENGTH];
  __be64 offset;
  __be64 child;
} __attribute__((packed));

#define TABLE_SIZE ((4096 - 1) / sizeof(struct btree_item))

struct btree_table {
  struct btree_item items[TABLE_SIZE];
  uint8_t size;
} __attribute__((packed));

struct btree_cache {
  off_t offset;
  struct btree_table *table;
};

struct blob_info {
  __be32 len;
};

struct btree_super {
  __be64 top;
  __be64 free_top;
  __be64 alloc;
} __attribute__((packed));

struct btree {
  off_t top;
  off_t free_top;
  off_t alloc;
  int fd;
  struct btree_cache cache[CACHE_SLOTS];
};
typedef struct btree BTree;
/*
 * Open an existing database file.
 */
int btree_open(struct btree *btree, const char *file);

/*
 * Create and initialize a new database file.
 */
int btree_creat(struct btree *btree, const char *file);

/*
 * Close a database file opened with btree_creat() or btree_open().
 */
void btree_close(struct btree *btree);

/*
 * Insert a new item with key 'sha1' with the contents in 'data' to the
 * database file.
 */
void btree_insert(struct btree *btree, const uint8_t *sha1, const void *data,
                  size_t len);

/*
 * Look up item with the given key 'sha1' in the database file. Length of the
 * item is stored in 'len'. Returns a pointer to the contents of the item.
 * The returned pointer should be released with free() after use.
 */
void *btree_get(struct btree *btree, const uint8_t *sha1, size_t *len);

/*
 * Remove item with the given key 'sha1' from the database file.
 */
int btree_delete(struct btree *btree, const uint8_t *sha1);

}  // namespace ROCKSDB_NAMESPACE

#endif  // __BTREE_H__
