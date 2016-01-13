#!/usr/bin/env python
# encoding: utf-8
'''
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
'''
import unittest
import shutil
import random
import os
import tempfile
from itertools import islice
from echoprint_server import load_inverted_index, inverted_index_size, \
    decode_echoprint, create_inverted_index, query_inverted_index


class TestLoadIndex(unittest.TestCase):

    def test_load_index(self):
        # load pre-made index, check that it contains 100 songs
        index_block_paths = ['testdata/inverted_index.bin']
        index = load_inverted_index(index_block_paths)
        self.assertEquals(inverted_index_size(index), 100)


class TestDecoding(unittest.TestCase):

    def test_decode_echoprint(self):
        # check that echoprint string is decoded correctly
        codestring = open('testdata/echoprint_string.txt').read().strip()
        expected_codes = [int(c) for c in open(
            'testdata/echoprint_codes.txt').read().split(',')]
        self.assertEquals(decode_echoprint(codestring)[1], expected_codes)


def codes_gen():
    # read the codes for 100 songs
    CODES_DIR = 'testdata/echoprint-strings'
    code_files = [f for f in sorted(os.listdir(CODES_DIR))
                  if f.endswith('.echoprint')]
    for f in code_files:
        codestr = open(os.path.join(CODES_DIR, f)).read().strip()
        yield decode_echoprint(codestr)[1]


class TestIndexMaking(unittest.TestCase):

    def test_make_inverted_index(self):
        temp_dir = tempfile.mkdtemp()
        temp_index_path = os.path.join(temp_dir, 'index')
        all_codes = list(codes_gen())
        create_inverted_index(all_codes, temp_index_path)
        self.assertTrue(open(temp_index_path).read() ==
                        open('testdata/inverted_index.bin').read())
        shutil.rmtree(temp_dir)


class TestIndexQuerying(unittest.TestCase):

    def test_query_inverted_index(self):
        '''
        Query an inverted index with the same songs from which it was
        constructed, checking that the best result is always correct
        and that its jaccard similarity score is 1.
        '''
        inverted_index = load_inverted_index(
            ['testdata/inverted_index.bin'])  # pre-build indexing codes_gen()
        for i, codes in enumerate(codes_gen()):
            results = query_inverted_index(
                codes, inverted_index, 'jaccard')
            best_res = results[0]
            assert(best_res['index'] == i)            # best result = self
            assert(best_res['score'] == 1.)           # self-similarity = 1

    def test_random_index_making_querying(self):
        '''
        Using random data, create an inverted index of 1000 songs.  Query using
        (different) random data, and make sure the similarity score is
        the same between index and a reference python implementation.
        '''

        def jaccard_similarity(a, b):
            a = set(a)
            b = set(b)
            return float(len(a & b)) / float(len(a | b))

        def random_code_maker(n):
            # get n "random songs"
            for _ in xrange(n):
                song_len = random.randint(200, 600)
                song_codes = list(set(random.sample(xrange(10000), song_len)))
                random.shuffle(song_codes)
                yield song_codes

        temp_dir = tempfile.mkdtemp()
        index_songs = list(random_code_maker(1000))
        inv_index_path = os.path.join(temp_dir, 'inverted_index')
        create_inverted_index(index_songs, inv_index_path)

        inv_index = load_inverted_index([inv_index_path])

        for query_codes in random_code_maker(100):
            inv_results = query_inverted_index(
                query_codes, inv_index, 'jaccard')
            for res in inv_results:
                doc_index = res['index']
                inverted_similarity = res['score']
                expected_similarity = jaccard_similarity(
                    query_codes, index_songs[doc_index])
                self.assertAlmostEqual(expected_similarity,
                                       inverted_similarity, 5)

        shutil.rmtree(temp_dir)


class TestMemoryLeaks(unittest.TestCase):

    def test_memory_leaks(self):
        '''
        Makes sure that the C extension is not leaking memory.  Not
        really a test, just prints out the heap size periodically to
        make sure we're not forgetting to deallocate objects.  The
        test is skipped if guppy is not installed.
        '''
        try:
            from guppy import hpy
        except ImportError:
            # just skip the test
            return
        for it in range(100):
            if it % 10 == 0:
                h = hpy()
                print h.heap()
            inv_index = load_inverted_index(['testdata/inverted_index.bin'])
            for query_codes in islice(codes_gen(), 2):
                query_inverted_index(query_codes, inv_index, 'jaccard')


if __name__ == '__main__':
    unittest.main()
