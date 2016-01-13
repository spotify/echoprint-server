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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "libechoprintserver.h"


int _cmpuint32(const void *a, const void *b)
{
  uint32_t x, y;
  x = *((uint32_t *) a);
  y = *((uint32_t *) b);
  return x - y;
}

void _sequence_to_set_inplace(uint32_t *seq, uint32_t *length)
{
  uint32_t i, j;
  if(*length >= 2)
  {
    qsort(seq, *length, sizeof(uint32_t), _cmpuint32);
    i = 0;
    j = 1;
    while(j < *length)
    {
      if(seq[i] == seq[j])
        j++;
      else
        seq[++i] = seq[j++];
    }
    *length = i + 1;
  }
}


// length of output array is index_block->n_songs
void echoprint_inverted_index_block_similarity(
  uint32_t query_length, uint32_t *query,
  EchoprintInvertedIndexBlock *index_block,
  float *output, similarity_function sim)
{

  int n, i, j, offset;

  for(n=0; n < index_block->n_songs; n++)
    output[n] = 0;

  i = 0;        // index of the codeblock
  j = 0;        // index of the query
  offset = 0;   // base pointer in index_block->song_indices
  while(j < query_length && i < index_block->n_codes)
  {
    uint32_t codeblock_length = index_block->code_lengths[i];
    uint32_t qc = query[j];
    uint32_t ic = index_block->codes[i];
    if(qc == ic)
    {
      for(n = 0; n < codeblock_length; n++)
      {
        uint16_t song_index = index_block->song_indices[offset + n];
        output[song_index]++;
      }
      i++;
      j++;
      offset += codeblock_length;
    }
    else
    {
      if(qc < ic)
        j++;
      else
      {
        i++;
        offset += codeblock_length;
      }
    }
  }

  for(n=0; n < index_block->n_songs; n++)
  {
    float den;
    float num = output[n];
    switch (sim)
    {
    case JACCARD:
      den = (float) (query_length + index_block->song_lengths[n] - num);
      break;
    case SET_INT:
      den = 1.0;
      break;
    case SET_INT_NORM_LENGTH_FIRST:
      den = query_length;
      break;
    }
    output[n] = num / den;
  }

}

// shift arrays' slices one position to the right, starting from i
void shift_outputs_right(
  int i, int len, uint32_t *indices, float *scores)
{
  int n;
  for(n = len-1; n > i; n--)
  {
    indices[n] = indices[n-1];
    scores[n] = scores[n-1];
  }
}

// index of first element in scores smaller than value
// (scores is sorted descending)
int first_index_smaller_than(
  float *scores, int len, float value)
{
  int i;
  for(i = len; i > 0; i--)
    if(scores[i-1] > value)
      break;
  return i;
}


// output_{indices, scores} have length n_results;
// return effective number returned (might be < n_results for very small index)
uint32_t echoprint_inverted_index_query(
  uint32_t query_length, uint32_t *query,
  EchoprintInvertedIndex *index,
  uint32_t n_results,
  uint32_t *output_indices,
  float *output_scores,
  similarity_function sim)
{
  int b, n, i;
  int max_block_n_songs, song_index_base;
  float *tmp_scores;

  max_block_n_songs = 0;
  for(b = 0; b < index->n_blocks; b++)
    max_block_n_songs = max_block_n_songs > index->blocks[b].n_songs ?
      max_block_n_songs : index->blocks[b].n_songs;

  tmp_scores = (float *) malloc(sizeof(float) * max_block_n_songs);

  for(n = 0; n < n_results; n++)
  {
    output_indices[n] = n;
    output_scores[n] = -1.;
  }

  _sequence_to_set_inplace(query, &query_length);

  song_index_base = 0;
  for(b = 0; b < index->n_blocks; b++)
  {
    echoprint_inverted_index_block_similarity(
      query_length, query, index->blocks + b, tmp_scores, sim);
    for(i = 0; i < index->blocks[b].n_songs; i++)
    {
      float ith_score = tmp_scores[i];
      int ith_pos = first_index_smaller_than(
        output_scores, n_results, ith_score);
      if(ith_pos < n_results)
      {
        shift_outputs_right(
          ith_pos, n_results, output_indices, output_scores);
        output_indices[ith_pos] = song_index_base + i;
        output_scores[ith_pos] = ith_score;
      }
    }
    song_index_base += index->blocks[b].n_songs;
  }

  int n_effective_results = 0;
  for(n = 0; n < n_results; n++)
    if(output_scores[n] >= 0.)
      n_effective_results++;

  free(tmp_scores);
  return n_effective_results;
}

void _load_echoprint_inverted_index_block(
  FILE *fp, EchoprintInvertedIndexBlock *block)
{
  int n;
  int n_tot_song_indices;
  fseek(fp, 0L, SEEK_SET);

  fread(&(block->n_codes), sizeof(uint32_t), 1, fp);
  fread(&(block->n_songs), sizeof(uint32_t), 1, fp);

  block->codes = (uint32_t *) malloc(sizeof(uint32_t) * block->n_codes);
  fread(block->codes, sizeof(uint32_t), block->n_codes, fp);
  block->code_lengths = (uint32_t *) malloc(sizeof(uint32_t) * block->n_codes);
  fread(block->code_lengths, sizeof(uint32_t), block->n_codes, fp);
  block->song_lengths = (uint32_t *) malloc(sizeof(uint32_t) * block->n_songs);
  fread(block->song_lengths, sizeof(uint32_t), block->n_songs, fp);

  n_tot_song_indices = 0;
  for(n = 0; n < block->n_codes; n++)
    n_tot_song_indices += block->code_lengths[n];
  block->song_indices =
    (uint16_t *) malloc(sizeof(uint16_t) * n_tot_song_indices);
  fread(block->song_indices, sizeof(uint16_t), n_tot_song_indices, fp);
}

// does not free block itself
void echoprint_inverted_index_free_block(
  EchoprintInvertedIndexBlock *block)
{
  free(block->codes);
  free(block->code_lengths);
  free(block->song_lengths);
  free(block->song_indices);
}

EchoprintInvertedIndex * load_echoprint_inverted_index(FILE **fps, int n_files)
{
  int n;
  EchoprintInvertedIndex * index =
    (EchoprintInvertedIndex *) malloc(sizeof(EchoprintInvertedIndex));
  index->n_blocks = n_files;
  index->blocks = (EchoprintInvertedIndexBlock *)
    malloc(sizeof(EchoprintInvertedIndexBlock) * index->n_blocks);
  for(n = 0; n < n_files; n++)
    _load_echoprint_inverted_index_block(fps[n], index->blocks + n);
  return index;
}

void echoprint_inverted_index_free(EchoprintInvertedIndex *index)
{
  int n;
  for(n = 0; n < index->n_blocks; n++)
    echoprint_inverted_index_free_block(index->blocks + n);
  free(index->blocks);
  free(index);
}

EchoprintInvertedIndex * echoprint_inverted_index_load_from_paths(
  char **paths, int n_files)
{
  int n;
  int all_files_opened = 1;
  FILE **fps = (FILE **) malloc(sizeof(FILE*) * n_files);
  for(n = 0; n < n_files; n++)
  {
    fps[n] = fopen(paths[n], "r");
    if(fps[n] == 0)
      all_files_opened = 0;
  }
  EchoprintInvertedIndex *epii = 0;
  if(all_files_opened)
    epii = load_echoprint_inverted_index(fps, n_files);
  for(n = 0; n < n_files; n++)
    if(fps[n] != 0)
      fclose(fps[n]);
  free(fps);
  return epii;
}


void echoprint_inverted_index_block_serialize(
  EchoprintInvertedIndexBlock *block,
  FILE *fp)
{
  int n, song_indices_length;
  song_indices_length = 0;
  for(n = 0; n < block->n_codes; n++)
    song_indices_length += block->code_lengths[n];
  fwrite(&(block->n_codes), sizeof(uint32_t), 1, fp);
  fwrite(&(block->n_songs), sizeof(uint32_t), 1, fp);
  fwrite(block->codes, sizeof(uint32_t), block->n_codes, fp);
  fwrite(block->code_lengths, sizeof(uint32_t), block->n_codes, fp);
  fwrite(block->song_lengths, sizeof(uint32_t), block->n_songs, fp);
  fwrite(block->song_indices, sizeof(uint16_t), song_indices_length, fp);
}


// `output` is malloc-ed; `sequences` and `sequence_lengths` are
// modified in-place
void _inplace_sort_and_merge(uint32_t **sequences,
                             uint32_t *sequence_lengths,
                             uint32_t n_sequences,
                             uint32_t **output,
                             uint32_t *output_length,
                             int code_sequences_already_sorted_distinct)
{
  uint32_t n, i, out_len;
  uint32_t *out;
  out_len = 0;
  for(n = 0; n < n_sequences; n++)
  {
    if(!code_sequences_already_sorted_distinct)
      _sequence_to_set_inplace(sequences[n], sequence_lengths + n);
    out_len += sequence_lengths[n];
  }
  out = (uint32_t *) malloc(sizeof(uint32_t) * out_len);
  i = 0;
  for(n = 0; n < n_sequences; n++)
  {
    uint32_t len_n = sequence_lengths[n];
    memcpy(out + i, sequences[n], sizeof(uint32_t) * len_n);
    i += len_n;
  }
  _sequence_to_set_inplace(out, &out_len);
  *output_length = out_len;
  *output = out;
}

void echoprint_inverted_index_block_from_song_codes(
  uint32_t **songs_codes,
  uint32_t *song_lengths,
  uint32_t n_songs,
  EchoprintInvertedIndexBlock *output_block,
  int code_sequences_already_sorted_distinct)
{
  uint32_t n_codes, c, i, code_lengths_sum;
  uint32_t *codes, *code_lengths, *code_offsets;
  uint16_t *song_indices;

  _inplace_sort_and_merge(songs_codes, song_lengths, n_songs,
                          &codes, &n_codes, code_sequences_already_sorted_distinct);

  code_lengths = (uint32_t *) malloc(sizeof(uint32_t) * n_codes);
  for(i = 0; i < n_codes; i++)
    code_lengths[i] = 0;
  for(i = 0; i < n_songs; i++)
  {
    int offset = 0;
    for(c = 0; c < song_lengths[i]; c++)
    {
      while(codes[offset] != songs_codes[i][c])
        offset++;
      code_lengths[offset]++;
    }
  }

  code_lengths_sum = 0;
  for(c = 0; c < n_codes; c++)
    code_lengths_sum += code_lengths[c];
  song_indices = (uint16_t *) malloc(
    sizeof(uint16_t) * code_lengths_sum);

  code_offsets = (uint32_t *) malloc(sizeof(uint32_t) * n_codes);
  code_offsets[0] = 0;
  for(c = 1; c < n_codes; c++)
    code_offsets[c] = code_offsets[c-1] + code_lengths[c-1];
  for(i = 0; i < n_songs; i++)
  {
    int offset = 0;
    for(c = 0; c < song_lengths[i]; c++)
    {
      uint32_t code = songs_codes[i][c];
      while(codes[offset] != code)
        offset++;
      song_indices[code_offsets[offset]] = i;
      code_offsets[offset]++;
    }
  }
  free(code_offsets);

  output_block->n_codes = n_codes;
  output_block->n_songs = n_songs;
  output_block->codes = codes;
  output_block->code_lengths = code_lengths;
  output_block->song_lengths = (uint32_t *) malloc(sizeof(uint32_t) * n_songs);
  memcpy(output_block->song_lengths, song_lengths, sizeof(uint32_t) * n_songs);
  output_block->song_indices = song_indices;
}


uint32_t echoprint_inverted_index_get_n_songs(
  EchoprintInvertedIndex *index)
{
  uint32_t n_total, b;
  n_total = 0;
  for(b = 0; b < index->n_blocks; b++)
    n_total += index->blocks[b].n_songs;
  return n_total;
}

int echoprint_inverted_index_build_write_block(
  uint32_t **block_songs_codes,
  uint32_t *block_song_lengths,
  uint32_t n_songs,
  char *path_out,
  int code_sequences_already_sorted_distinct)
{
  FILE *fout;
  EchoprintInvertedIndexBlock block;
  fout = fopen(path_out, "w");
  if(fout == 0)
    return 1;
  echoprint_inverted_index_block_from_song_codes(
    block_songs_codes, block_song_lengths, n_songs, &block,
    code_sequences_already_sorted_distinct);
  echoprint_inverted_index_block_serialize(&block, fout);
  fclose(fout);
  echoprint_inverted_index_free_block(&block);
  return 0;
}
