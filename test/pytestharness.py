#!/usr/bin/env python

import unittest
from unittest import TestCase
import sys
import os
sys.path += os.path.abspath(os.path.join(os.path.split(__file__)[0], "../pylib")),
import simplerpc
from benchmark_service import *
from test_service import *

class TestMarshal(TestCase):
    def test_marshal(self):
        m = simplerpc.Marshal()
        assert len(m) == 0
        m.write_i32(45)
        assert len(m) == 4
        assert m.read_i32() == 45
        assert len(m) == 0
        m.write_i32(-45)
        assert len(m) == 4
        assert m.read_i32() == -45
        assert len(m) == 0
        m.write_i64(1987)
        assert len(m) == 8
        assert m.read_i64() == 1987
        assert len(m) == 0
        m.write_i64(-1987)
        assert len(m) == 8
        assert m.read_i64() == -1987
        assert len(m) == 0
        m.write_double(-1.987)
        assert len(m) == 8
        assert m.read_double() == -1.987
        assert len(m) == 0
        m.write_str("hello world!")
        print len(m)
        print m.read_str()
        print len(m)
        p = point3(x=3.0, y=4.0, z=5.0)
        m.write_obj(p, "point3")
        print len(m)
        print m.read_obj("point3")
        print len(m)

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
        p = point3(x=3.0, y=4.0, z=5.0)
        print p

    def test_service_gen(self):
        class BS(BenchmarkService):
            def fast_prime(self, n):
                print self, n
                print "called 1 %d" % n
                return n
            def fast_dot_prod(self, a, b):
                print "called 2"
                return 0
        s = simplerpc.Server()
        s.reg_svc(BS())

    def test_proxy_gen(self):
        pass


if __name__ == "__main__":
    unittest.main()
