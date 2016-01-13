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

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;


/**
 * JNA bridge to C library.
 */
public interface EchoprintServerLib extends Library {

  EchoprintServerLib INSTANCE = (EchoprintServerLib)
    Native.loadLibrary("echoprintserver", EchoprintServerLib.class);

  Pointer echoprint_inverted_index_load_from_paths(
    String[] paths, int n_files);

  void echoprint_inverted_index_free(
    Pointer index);

  int echoprint_inverted_index_query(
    int query_length, int[] query, Pointer index,
    int n_results, int[] output_indices, float[] output_scores,
    int comparisonFunction);

  int echoprint_inverted_index_get_n_songs(Pointer index);

}
