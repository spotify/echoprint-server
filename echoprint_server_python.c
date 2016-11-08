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
#include <Python.h>
#include "libechoprintserver.h"

/* Docstrings */
static char module_docstring[] =
  "This module provides an interface for decoding musicbrainz fingerprints.";
static char load_inverted_index_docstring[] =
  "Load the inverted index from a (ordered) list of file paths.";
static char query_inverted_index_docstring[] =
  "query inverted index"; // TODO complete docstring
static char inverted_index_size_docstring[] =
  "return the number of songs present in the index";
static char inverted_index_create_block_docstring[] =
  "create an index block";

/* Available functions */
static PyObject *echoprint_py_load_inverted_index(
  PyObject *self, PyObject *args);
static PyObject *echoprint_py_query_inverted_index(
  PyObject *self, PyObject *args);
static PyObject *echoprint_py_inverted_index_size(
  PyObject *self, PyObject *args);
static PyObject *echoprint_py_inverted_index_create_block(
  PyObject *self, PyObject *args);

/* Module specification */
static PyMethodDef module_methods[] = {
  {"load_inverted_index", echoprint_py_load_inverted_index,
   METH_VARARGS, load_inverted_index_docstring},
  {"inverted_index_size", echoprint_py_inverted_index_size,
   METH_VARARGS, inverted_index_size_docstring},
  {"query_inverted_index", echoprint_py_query_inverted_index,
   METH_VARARGS, query_inverted_index_docstring},
  {"_create_index_block", echoprint_py_inverted_index_create_block,
   METH_VARARGS, inverted_index_create_block_docstring},
  {NULL, NULL, 0, NULL}
};

/* Initialize the module */
PyMODINIT_FUNC initechoprint_server_c(void)
{
  PyObject *m = Py_InitModule3(
    "echoprint_server_c", module_methods, module_docstring);
  if (m == NULL)
    return;
}

// destructor
static void echoprint_py_free_inverted_index(PyObject *object)
{
  EchoprintInvertedIndex *index = (EchoprintInvertedIndex *)
    PyCapsule_GetPointer(object, NULL);
  echoprint_inverted_index_free(index);
}

// constructor
static PyObject *echoprint_py_load_inverted_index(
  PyObject *self, PyObject *args)
{
  PyObject *arg_index_file_list;
  EchoprintInvertedIndex *index;
  char **index_file_paths;
  int n, n_blocks;
  if(!PyArg_ParseTuple(args, "O", &arg_index_file_list))
    return NULL;
  if(!PyList_Check(arg_index_file_list))
  {
    PyErr_SetString(PyExc_TypeError, "parameter must be a list");
    return NULL;
  }
  n_blocks = PyList_Size(arg_index_file_list);
  index_file_paths = (char **) malloc(sizeof(char *) * n_blocks);
  for(n = 0; n < n_blocks; n++)
  {
    PyObject *py_path = PyList_GetItem(arg_index_file_list, n);
    if(!PyString_Check(py_path))
    {
      PyErr_SetString(PyExc_TypeError, "argument's items must be strings");
      return NULL;
    }
    index_file_paths[n] = PyString_AsString(
      PyList_GetItem(arg_index_file_list, n));
  }
  index = echoprint_inverted_index_load_from_paths(index_file_paths, n_blocks);
  free(index_file_paths);
  if(index == NULL)
  {
    PyErr_SetString(PyExc_Exception, "could not load the index");
    return NULL;
  }
  return PyCapsule_New(index, NULL, echoprint_py_free_inverted_index);
}

// size of the inverted index
static PyObject *echoprint_py_inverted_index_size(
  PyObject *self, PyObject *args)
{
  PyObject *arg_index;
  EchoprintInvertedIndex *index;
  if(!PyArg_ParseTuple(args, "O", &arg_index))
    return NULL;
  index = (EchoprintInvertedIndex *) PyCapsule_GetPointer(arg_index, NULL);
  if(!index)
  {
    PyErr_SetString(PyExc_Exception, "the argument is not a valid index");
    return NULL;
  }
  return PyInt_FromLong((long) echoprint_inverted_index_get_n_songs(index));
}

// query
static PyObject *echoprint_py_query_inverted_index(
  PyObject *self, PyObject *args)
{
  PyObject *arg_query, *arg_index, *arg_sim_fun;
  EchoprintInvertedIndex *index;
  uint32_t n;
  uint32_t query_length, n_results, N_MAX_RESULTS;
  uint32_t *query, *output_indices;
  float *output_scores;
  similarity_function sf;
  PyObject *results;

  if(!PyArg_ParseTuple(args, "OOS", &arg_query, &arg_index, &arg_sim_fun))
    return NULL;
  if(!PyList_Check(arg_query))
    return NULL;

  if(strcmp(PyString_AsString(arg_sim_fun), "jaccard") == 0)
    sf = JACCARD;
  else if(strcmp(PyString_AsString(arg_sim_fun), "set_int") == 0)
    sf = SET_INT;
  else if(strcmp(PyString_AsString(arg_sim_fun),
		 "set_int_norm_length_first") == 0)
    sf = SET_INT_NORM_LENGTH_FIRST;
  else
  {
    PyErr_SetString(PyExc_Exception, "similarity must be one of: \"jaccard\", \"set_int\", \"set_int_norm_length_first\"");
    return NULL;
  }

  index = (EchoprintInvertedIndex *) PyCapsule_GetPointer(arg_index, NULL);
  if(!index)
  {
    PyErr_SetString(PyExc_Exception, "the argument is not a valid index");
    return NULL;
  }

  query_length = PySequence_Length(arg_query);
  query = (uint32_t *) malloc(sizeof(uint32_t) * query_length);
  for(n = 0; n < query_length; n++)
  {
    PyObject *code_obj;
    long code;
    code_obj = PySequence_GetItem(arg_query, n);
    if(!PyInt_Check(code_obj))
    {
      PyErr_SetString(
	PyExc_TypeError, "all the codes in the query must be integers");
      Py_DECREF(code_obj);
      free(query);
      return NULL;
    }
    code = (uint32_t) PyInt_AsLong(code_obj);
    Py_DECREF(code_obj);
    query[n] = (int) code;
  }

  N_MAX_RESULTS = 10;
  output_indices = (uint32_t *) malloc(sizeof(uint32_t) * N_MAX_RESULTS);
  output_scores = (float *) malloc(sizeof(float) * N_MAX_RESULTS);
  n_results = echoprint_inverted_index_query(
    query_length, query, index,
    N_MAX_RESULTS, output_indices, output_scores, sf);

  results = PyList_New(n_results);
  for(n = 0; n < n_results; n++)
  {
    PyObject *r = PyDict_New();                                                                                                                                                                                                        
    PyStringObject* score_k = (PyStringObject*)PyString_FromString("score");
    PyFloatObject* score_v = (PyFloatObject*)PyFloat_FromDouble((float) output_scores[n]);
    PyDict_SetItem(r, (PyObject*)score_k, (PyObject*)score_v);
    Py_DECREF(score_k);
    Py_DECREF(score_v);
    PyStringObject* index_k = (PyStringObject*)PyString_FromString("index");
    PyIntObject* index_v = (PyIntObject*)PyInt_FromLong((long) output_indices[n]);
    PyDict_SetItem(r, (PyObject*)index_k, (PyObject*)index_v);
    Py_DECREF(index_k);
    Py_DECREF(index_v);
    PyList_SetItem(results, n, r); 
  }

  free(output_indices);
  free(output_scores);
  free(query);

  return results;
}


static PyObject *echoprint_py_inverted_index_create_block(
  PyObject *self, PyObject *args)
{
  // input is a list of lists, each item of the outer list being a
  // song (list of codes); second argument is the output path

  PyObject *arg_songs, *arg_output_path;
  int n, m, n_songs, error_parsing_input, error_writing_blocks;
  char *path_out;
  uint32_t **block_songs_codes;
  uint32_t *block_song_lengths;

  if(!PyArg_ParseTuple(args, "OS", &arg_songs, &arg_output_path))
    return NULL;

  error_parsing_input = 0;
  if(!PyList_Check(arg_songs))
  {
    PyErr_SetString(
      PyExc_TypeError, "first argument must be a list (of lists of codes)");
    return NULL;
  }

  path_out = PyString_AsString(arg_output_path);
  n_songs = PyList_Size(arg_songs);

  block_song_lengths = (uint32_t*) malloc(sizeof(uint32_t) * n_songs);
  block_songs_codes = (uint32_t **) malloc(sizeof(uint32_t *) * n_songs);

  for(n = 0; n < n_songs; n++)
    block_songs_codes[n] = 0;
  for(n = 0; n < n_songs; n++)
  {
    if(error_parsing_input)
      break;
    else
    {
      PyObject * py_song_seq = PySequence_GetItem(arg_songs, n);
      uint32_t song_length = PyList_Size(py_song_seq);
      block_songs_codes[n] = (uint32_t *) malloc(
	sizeof(uint32_t) * song_length);
      for(m = 0; m < song_length; m++)
      {
	PyObject *code = PyList_GetItem(py_song_seq, m);
	if(!PyInt_Check(code))
	{
	  PyErr_SetString(
	    PyExc_TypeError, "all codes in input songs must be integers");
	  error_parsing_input = 1;
	  break;
	}
	else
	  block_songs_codes[n][m] = (uint32_t) PyInt_AsLong(code);
      }
      block_song_lengths[n] = song_length;
      Py_DECREF(py_song_seq);
    }
  }

  error_writing_blocks = 0;
  if(!error_parsing_input)
  {
    if(echoprint_inverted_index_build_write_block(
	 block_songs_codes, block_song_lengths, n_songs, path_out, 0))
    {
      error_writing_blocks = 1;
      PyErr_SetString(PyExc_TypeError, "could not write the index block");
    }
  }

  for(n = 0; n < n_songs; n++)
    if(block_songs_codes[n] != 0)
      free(block_songs_codes[n]);
  free(block_songs_codes);
  free(block_song_lengths);

  if(error_parsing_input || error_writing_blocks)
    return NULL;

  return Py_None;

}
