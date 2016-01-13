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
package com.spotify.echoprintserver.nativelib.example;

import com.spotify.echoprintserver.nativelib.*;

import java.io.IOException;
import java.util.*;

public class UsageExample {

  public static void main(String[] args) throws IOException, IndexLoadingException {

    // load the indices
    List<String> invertedIndexPaths = new ArrayList<>();
    invertedIndexPaths.add("path/to/inverted_index");
    InvertedIndex invertedIndex = new InvertedIndex(invertedIndexPaths);
    invertedIndex.load();

    // load an echoprint string -> transform into list of codes
    String echoprintString = "an echoprint string goes in here";
    List<Integer> queryCodes = Util.decodeEchoprintString(echoprintString);

    // query the inverted index, for the top10 results
    List<QueryResult> invertedIndexResults = invertedIndex.query(queryCodes, 10, ComparisonFunctions.JACCARD);
    for (QueryResult qr : invertedIndexResults) {
      System.out.println(String.format("[%04d]: %.3f", qr.getIndex(), qr.getScore()));
    }

  }

}
