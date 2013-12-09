#!/usr/bin/env python

import unittest
from unittest import TestCase
import sys
import os
sys.path += os.path.abspath(os.path.join(os.path.split(__file__)[0], "../pylib")),
import simplerpc

class TestUtils(TestCase):
    def test_cpp_python_interop(self):
        s = simplerpc.Server()

        s.reg_func(1987, lambda x, y: x + y)
        s.start("0.0.0.0:8848")

        c = simplerpc.Client()
        c.connect("0.0.0.0:8848")
        for i in range(1000):
            assert c.sync_call(1987, 1, 2) == 3

class TestRpcGen(TestCase):
    def test_struct_gen(self):
        from benchmark_service import point3
        p = point3(x=3.0, y=4.0, z=5.0)
        print p

    def test_service_gen(self):
        from benchmark_service import BenchmarkService

    def test_proxy_gen(self):
        from benchmark_service import BenchmarkProxy


if __name__ == "__main__":
    unittest.main()
