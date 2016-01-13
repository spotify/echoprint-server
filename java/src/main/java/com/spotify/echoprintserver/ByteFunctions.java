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

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

/**
 * Internal convenience functions for operating on binary integer representations.
 */
public class ByteFunctions {

  public static byte[] asUint32Array(Integer n) {
    return ByteBuffer.allocate(4)
            .order(ByteOrder.LITTLE_ENDIAN).putInt(n).array();
  }

  public static byte[] asUint32Array(List<Integer> nn) {
    ByteBuffer arr = ByteBuffer.allocate(nn.size() * 4);
    for (Integer n : nn)
      arr.put(asUint32Array(n));
    return arr.array();
  }

  public static byte[] asUint16Array(Integer n) {
    return ByteBuffer.allocate(2)
            .order(ByteOrder.LITTLE_ENDIAN).putShort(n.shortValue()).array();
  }

  public static byte[] asUint16Array(List<Integer> nn) {
    ByteBuffer arr = ByteBuffer.allocate(nn.size() * 2);
    for (Integer n : nn)
      arr.put(asUint16Array(n));
    return arr.array();
  }

  public static byte[] asUint64Array(Long n) {
    return ByteBuffer.allocate(8)
            .order(ByteOrder.LITTLE_ENDIAN).putLong(n).array();
  }
}
