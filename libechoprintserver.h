/*
 * Copyright (c) 2016 Spotify AB.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <stdio.h>
#include <stdint.h>

typedef enum
{
  JACCARD = 0,
  SET_INT = 1,
  SET_INT_NORM_LENGTH_FIRST = 2
} similarity_function;


/**
   A part of an inverted index. Each block is serialized to disk in a
   different file; the serialization format is just a contiguous
   memory dump of all the data in the struct in order.

   For each distinc code, a code block contains the sequence of
   indexes corresponding to songs in which the code appears. The
   `song_indices` field contains a contiguous dump of all codeblocks,
   the other fields provide ways to index into it.
 */
typedef struct _EchoprintInvertedIndexBlock
{
  uint32_t n_codes;         // number of distinct codes
  uint32_t n_songs;
  uint32_t *codes;          // stores code of each codeblock (n_codes)
  uint32_t *code_lengths;   // length of each codeblock (n_codes)
  uint32_t *song_lengths;   // number of codes per song (n_songs)
  uint16_t *song_indices;   // main data (SUM-OF code_lengths)
} EchoprintInvertedIndexBlock;

/**
   An inverted index is just an ordered sequence of inverted index
   blocks.
*/
typedef struct _EchoprintInvertedIndex
{
  uint32_t n_blocks;
  EchoprintInvertedIndexBlock *blocks;
} EchoprintInvertedIndex;

/**
   Load an inverted index in memory. The order of `paths` is
   significant.
 */
EchoprintInvertedIndex * echoprint_inverted_index_load_from_paths(
  char **paths,
  int n_files);

/**
   Frees an inverted index (and all its blocks).
 */
void echoprint_inverted_index_free(
  EchoprintInvertedIndex *index);

/**
   Perform a query on the whole index.  Results (indices and jaccard
   similarities) are stored in the `output` and `output_scores`
   parameters, which must hold `n_results` elements.  Returns the
   number of results actually returned (just in the unrealistic case
   that the index size is smaller than n_results).
 */
uint32_t echoprint_inverted_index_query(
  uint32_t query_length,
  uint32_t *query,
  EchoprintInvertedIndex *index,
  uint32_t n_results,
  uint32_t *output_indices,
  float *output_scores,
  similarity_function sim);

/**
   Get total number of songs in the index
 */
uint32_t echoprint_inverted_index_get_n_songs(
  EchoprintInvertedIndex *index);

/**
   Construct an inverted index block from data and write it out to
   disk. Return 0 if all ok, 1 otherwise.
 */
int echoprint_inverted_index_build_write_block(
  uint32_t **block_songs_codes,
  uint32_t *block_song_lengths,
  uint32_t n_songs,
  char *path_out,
  int code_sequences_already_sorted_distinct);
