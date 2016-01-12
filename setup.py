#!/usr/bin/env python
# encoding: utf-8
from distutils.core import setup, Extension

c_ext = Extension("echoprint_server_c",
                  ["libechoprintserver.c", "echoprint_server_python.c"])

setup(
    name='echoprint_server',
    version='0.0.1',
    ext_modules=[c_ext],
    packages=['echoprint_server']
)
