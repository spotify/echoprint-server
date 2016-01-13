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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;

import org.apache.commons.codec.binary.Base64;
import org.apache.commons.compress.compressors.deflate.DeflateCompressorInputStream;
import org.apache.commons.io.IOUtils;

public class Util {

  public static List<Integer> sortedDistinct(List<Integer> seq) {
    Set<Integer> codeSet = new TreeSet<Integer>();
    codeSet.addAll(seq);
    List<Integer> uniqueSortedCodeSet = new ArrayList<Integer>();
    uniqueSortedCodeSet.addAll(codeSet);
    Collections.sort(uniqueSortedCodeSet);
    return uniqueSortedCodeSet;
  }

  public static byte[] b64SafeDecode(String s) {
    return new Base64(true).decodeBase64(s);
  }

  /**
   * Decode an Echoprint string into its list of codes (temporal offsets are not returned).
   */
  public static List<Integer> decodeEchoprintString(String echoprintString)
          throws IOException {

    byte[] decoded = b64SafeDecode(echoprintString);

    DeflateCompressorInputStream is = new DeflateCompressorInputStream(
            new ByteArrayInputStream(decoded));

    ByteArrayOutputStream os = new ByteArrayOutputStream();
    IOUtils.copy(is, os);
    String decodedEchoprintString = os.toString();

    // skip the first half of the string (offsets)
    // take every 5 chars, parse int in base 16

    List<Integer> codes = new ArrayList<Integer>();

    int N = decodedEchoprintString.length();
    for (int n = N / 2; n < N; n += 5) {
      String s = decodedEchoprintString.substring(n, n + 5);
      Integer c = Integer.parseInt(s, 16);
      codes.add(c);
    }

    return codes;

  }
}
