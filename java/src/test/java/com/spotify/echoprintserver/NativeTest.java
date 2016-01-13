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
package com.spotify.echoprintserver;

import com.spotify.echoprintserver.nativelib.ComparisonFunctions;
import com.spotify.echoprintserver.nativelib.IndexLoadingException;
import com.spotify.echoprintserver.nativelib.InvertedIndex;
import com.spotify.echoprintserver.nativelib.QueryResult;
import org.junit.Assert;
import org.junit.Test;

import java.io.IOException;
import java.util.*;


public class NativeTest {

  @Test
  /**
   * Query the index with one song that is in the index itself
   * (in the 11-th position). Make sure that the top result is
   * indeed correct and has score 1.
   */
  public void testInvertedIndexQuerying() throws IOException, IndexLoadingException {
    List<String> paths = new ArrayList<String>();
    paths.add(this.getClass().getResource("/inverted_index.bin").getPath());
    InvertedIndex index = new InvertedIndex(paths);
    index.load();
    Integer[] query = new TestUtils().test100EchoprintCodes().get(10);
    QueryResult bestResult = index.query(Arrays.asList(query), 10, ComparisonFunctions.JACCARD).get(0);
    Assert.assertEquals(10, bestResult.getIndex());
    Assert.assertEquals(1.f, bestResult.getScore(), 0.001);
    index.release();
  }

  /**
   * Make sure the correct exception is thrown when the index cannot be loaded.
   */
  @Test(expected = IndexLoadingException.class)
  public void testExceptions() throws IOException, IndexLoadingException {
    List<String> paths = new ArrayList<String>();
    paths.add("this-file-does-not-exist");
    InvertedIndex index = new InvertedIndex(paths);
    index.load();
  }

}
