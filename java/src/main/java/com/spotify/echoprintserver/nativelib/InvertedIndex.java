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
package com.spotify.echoprintserver.nativelib;

import com.sun.jna.Pointer;

import java.util.*;

/**
 * Inverted Index structure. Wraps the C library methods.
 */
public class InvertedIndex {

  private Pointer index;
  private String[] paths;

  /**
   * Constructor. Note that no significant operation is done at this time
   * (that's what the load() method does)
   *
   * @param blockFilePaths List of file paths; order is relevant.
   */
  public InvertedIndex(List<String> blockFilePaths) {
    paths = new String[blockFilePaths.size()];
    blockFilePaths.toArray(paths);
    index = null;
  }

  /**
   * Load the index into memory.
   *
   * @throws IndexLoadingException if the index cannot be loaded.
   */
  public void load() throws IndexLoadingException {
    index = EchoprintServerLib.INSTANCE.echoprint_inverted_index_load_from_paths(
            paths, paths.length);
    if (index == Pointer.NULL)
      throw new IndexLoadingException("could not load inverted index");
  }

  /**
   * Free the memory held by the underlying C library.
   * Forgetting to call this function will cause significant memory leaks.
   */
  public void release() {
    if (index != null)
      EchoprintServerLib.INSTANCE.echoprint_inverted_index_free(index);
  }

  /**
   * Get the number of songs indexed.
   */
  public int getNSongs() {
    return EchoprintServerLib.INSTANCE.echoprint_inverted_index_get_n_songs(index);
  }

  /**
   * Perform a query
   *
   * @param query              sequence of echoprint codes
   * @param nResults           number of results to be returned
   * @param comparisonFunction similarity function, to be chosen among {@link ComparisonFunctions}
   * @return
   */
  public List<QueryResult> query(List<Integer> query, int nResults, int comparisonFunction) {

    if (index == null)
      throw new NullPointerException("load() must be called before querying");

    int[] resultsIndices = new int[nResults];
    float[] resultsScores = new float[nResults];

    int _i = 0;
    int[] _query = new int[query.size()];
    for (Integer code : query)
      _query[_i++] = code;

    int nActualResults = EchoprintServerLib.INSTANCE.echoprint_inverted_index_query(
            _query.length, _query, index, nResults, resultsIndices, resultsScores, comparisonFunction);

    List<QueryResult> results = new ArrayList(nActualResults);
    for (int i = 0; i < nActualResults; i++)
      results.add(new QueryResult(resultsIndices[i], resultsScores[i]));

    return results;
  }

}
