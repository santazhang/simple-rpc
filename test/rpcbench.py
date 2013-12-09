#!/usr/bin/env python

import sys
import os
import argparse
import time
sys.path += os.path.abspath(os.path.join(os.path.split(__file__)[0], "../pylib")),
import simplerpc

def main():
    argparser = argparse.ArgumentParser(prog=sys.argv[0])
    argparser.add_argument("-c")
    argparser.add_argument("-s")
    opt = argparser.parse_args(sys.argv[1:])
    if opt.c:
        c = simplerpc.Client()
        c.connect(opt.c)
        for i in range(1, 100001):
            c.sync_call(1987, 1, 2)
            if i % 1000 == 0:
                print "%d requests done" % i
    elif opt.s:
        s = simplerpc.Server()

        def a_add_b(a, b):
            return a + b

        s.reg_func(1987, a_add_b)

        s.start(opt.s)
        while True:
            time.sleep(1)

if __name__ == "__main__":
    main()
