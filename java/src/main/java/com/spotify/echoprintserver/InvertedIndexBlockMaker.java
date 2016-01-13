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

import com.spotify.echoprintserver.nativelib.IndexCreationException;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.*;

/**
 * A pure-Java implementation of the inverted index creation routine.
 */
public class InvertedIndexBlockMaker {

  /**
   * Construct an index block from a list of code sequences.
   *
   * @return binary index representation, to be serialized as-is on disk.
   * @throws IOException An index block cannot contain more than 65535 songs, any attempt to
   *                     do that will result in an exception being thrown.
   */
  public static byte[] fromCodeSequences(List<Integer[]> codeSequences) throws IOException, IndexCreationException {

    if(codeSequences.size() > 65535)
      throw new IndexCreationException("an index block cannot contain more than 2^16 - 1 songs");

    List<Integer[]> sortedUniqueCodeSequences = new LinkedList<>();
    for (Integer[] codeSequence : codeSequences) {
      TreeSet<Integer> codeSet = new TreeSet<>(Arrays.asList(codeSequence));
      sortedUniqueCodeSequences.add(codeSet.toArray(new Integer[codeSet.size()]));
    }

    // inverted mapping code -> [songs]
    Map<Integer, List<Integer>> code2songs = new TreeMap<Integer, List<Integer>>();
    ListIterator<Integer[]> it = sortedUniqueCodeSequences.listIterator();
    while (it.hasNext()) {
      int songIndex = it.nextIndex();
      Integer[] songCodes = it.next();
      for (Integer code : songCodes) {
        if (!code2songs.containsKey(code))
          code2songs.put(code, new ArrayList<Integer>());
        code2songs.get(code).add(songIndex);
      }
    }

    List<Integer> codeSet = new ArrayList<Integer>(code2songs.keySet());
    Collections.sort(codeSet);

    Integer nCodes = codeSet.size();
    Integer nSongs = sortedUniqueCodeSequences.size();

    List<Integer> codeLengths = new ArrayList<Integer>(nCodes);
    for (Integer c : codeSet)
      codeLengths.add(code2songs.get(c).size());

    List<Integer> songLengths = new ArrayList<Integer>(nSongs);
    for (Integer[] codes : sortedUniqueCodeSequences)
      songLengths.add(codes.length);

    ByteArrayOutputStream out = new ByteArrayOutputStream();
    out.write(ByteFunctions.asUint32Array(nCodes));
    out.write(ByteFunctions.asUint32Array(nSongs));
    out.write(ByteFunctions.asUint32Array(codeSet));
    out.write(ByteFunctions.asUint32Array(codeLengths));
    out.write(ByteFunctions.asUint32Array(songLengths));
    for (Integer c : codeSet)
      out.write(ByteFunctions.asUint16Array(code2songs.get(c)));

    return out.toByteArray();
  }

}
