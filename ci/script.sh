#!/usr/bin/env bash
# Copyright (c) 2015-2016 Spotify AB.
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

# Build the C code
mkdir build
cd build
cmake ..
make
cd ..

# Setup the python environment
virtualenv echoprintenv
source echoprintenv/bin/activate
pip install nose
pip install flake8
python setup.py install

# Lint the code
LINT_RESULT=`flake8 --config=.flake8 .`

# Run the tests
TEST_RESULT=`nosetests`

# Cleanup
deactivate
rm -rf echoprintenv
if [ "$LINT_RESULT" != "" ]
then
    exit 1
fi
if [ "$TEST_RESULT" != "0" ]
then
    exit $TEST_RESULT
fi
