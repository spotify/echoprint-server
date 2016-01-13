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

import org.junit.Assert;
import org.junit.Test;

import java.io.IOException;
import java.io.StringWriter;
import java.util.List;
import java.util.Arrays;

import org.apache.commons.io.IOUtils;
import com.spotify.echoprintserver.nativelib.Util;

public class DecodeTest {

  @Test
  /** make sure that the echoprint string -> code sequence function works as expected */
  public void testEchoprintStringDecoding() throws IOException {
    StringWriter sw = new StringWriter();
    IOUtils.copy(this.getClass().getResourceAsStream("/echoprint_string.txt"), sw);
    String echoprintString = sw.toString();
    List<Integer> expectedCodes = Arrays.asList(
            TestUtils.parseCsv(
                    this.getClass().getResourceAsStream("/echoprint_codes.txt")).get(0));
    List<Integer> computedCodes = Util.decodeEchoprintString(echoprintString);
    for (int i = 0; i < expectedCodes.size(); i++)
      Assert.assertEquals(expectedCodes.get(i), computedCodes.get(i));

  }

}
