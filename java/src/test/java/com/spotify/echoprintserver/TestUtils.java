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

import com.spotify.echoprintserver.nativelib.Util;
import org.apache.commons.io.IOUtils;

import java.io.*;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;


public class TestUtils {

  private static Integer[] parseCsvLine(String line) {
    String[] parts = line.split(",");
    Integer[] codes = new Integer[parts.length];
    for (int i = 0; i < parts.length; i++)
      codes[i] = Integer.parseInt(parts[i]);
    return codes;
  }

  public static List<Integer[]> parseCsv(InputStream fileStream) throws IOException {
    BufferedReader reader = new BufferedReader(
            new InputStreamReader(fileStream));
    List<Integer[]> allCodes = new ArrayList<Integer[]>();
    String line;
    while ((line = reader.readLine()) != null)
      allCodes.add(parseCsvLine(line));
    return allCodes;
  }

  // read all the codes in testdata/echoprint-strings
  public List<Integer[]> test100EchoprintCodes() throws IOException {
    List<String> allFiles = new LinkedList<>();
    List<Integer[]> allCodes = new LinkedList<>();
    InputStream in = this.getClass().getResourceAsStream("/echoprint-strings");
    BufferedReader rdr = new BufferedReader(new InputStreamReader(in));
    String __fp;
    while ((__fp = rdr.readLine()) != null)
      if (__fp.endsWith(".echoprint"))
        allFiles.add(__fp);
    Collections.sort(allFiles);
    for (String _filePath : allFiles) {
      StringWriter sw = new StringWriter();
      String filePath = "/echoprint-strings/" + _filePath;
      IOUtils.copy(this.getClass().getResourceAsStream(filePath), sw);
      String echoprintString = sw.toString();
      List<Integer> _codes = Util.decodeEchoprintString(echoprintString);
      Integer[] codes = _codes.toArray(new Integer[_codes.size()]);
      allCodes.add(codes);
    }
    return allCodes;
  }

}
