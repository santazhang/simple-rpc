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

        def a_add_b(a, b):
            print "*** called"
            return a + b

        s.reg_func(1987, a_add_b)

        s.start("0.0.0.0:8848")

        c = simplerpc.Client()
        c.connect("0.0.0.0:8848")
        print c.sync_call(1987, 1, 2) # 1 + 2 -> 3

if __name__ == "__main__":
    unittest.main()
