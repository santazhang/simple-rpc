#!/usr/bin/env python

import os
import sys
import subprocess
import traceback
import collections
import signal


def write_log(msg):
    # colors: 0;30-0;37, 1:30-1;37
    sys.stdout.write("\x1B[1;36m" + "*** regression.py: " + msg + "\x1B[0m\n")
    sys.stdout.flush()


def ohcount_lines():
    ret = ""
    p = os.popen("ohcount rpc/ test/")
    pout = p.read()
    p.close()
    assert pout != ""
    idx = pout.find("----")
    idx2 = pout.rfind("----")
    for line in pout[idx:idx2].split("\n")[1:-1]:
        sp = line.split()
        ret += "%s: %s\n" % (sp[0], sp[2])

    return ret


class Test:
    def __init__(self, **attr):
        default_values = {
            "repeat": 1,
            "mode": "release",
            "group": None,
            "before": lambda: None,
            "after": lambda: None,
            "func": lambda: None,
        }
        for k in default_values:
            if k not in attr:
                attr[k] = default_values[k]
        self.__dict__ = attr


def checked_exec(cmd):
    write_log("exec: %s" % cmd)
    try:
        subprocess.check_call(cmd, shell=True)
    except subprocess.CalledProcessError as e:
        if e.returncode >= 0:
            print "*** fail (retcode=%d)" % e.returncode
        else:
            signal_names = dict((k, v) for v, k in signal.__dict__.iteritems() if v.startswith('SIG') and not v.startswith('SIG_'))
            print "*** fail (%s)" % signal_names[-e.returncode]


def run_test(t):
    write_log("running %s (group=%s, mode=%s)" % (t.name, t.group, t.mode))
    for i in range(t.repeat):
        t.before()
        if t.mode == "python":
            try:
                print t.func()
            except:
                traceback.print_exc()
        elif t.mode == "valgrind":
            cmd = "valgrind --leak-check=full ./build/debug/%s %s" % (t.group, t.name)
            checked_exec(cmd)
            cmd = "valgrind --leak-check=full ./build/release/%s %s" % (t.group, t.name)
            checked_exec(cmd)
        else:
            cmd = "./build/%s/%s %s" % (t.mode, t.group, t.name)
            checked_exec(cmd)
        t.after()
    write_log("finished %s (group=%s, mode=%s)" % (t.name, t.group, t.mode))


class RegressionTester:

    def __init__(self):
        self.tests = []

    def reg(self, **attr):
        self.tests += Test(**attr),

    def run(self):
        for t in self.tests:
            run_test(t)


#
# MAIN ENTRY
#
if __name__ == "__main__":

    # change cwd to project root dir
    os.chdir(os.path.split(os.path.split(os.path.abspath(__file__))[0])[0])

    # make sure we have the newest binary
    if os.system("./waf") != 0:
        write_log("build failed, cannot run regression test!")
        exit(-1)

    tester = RegressionTester()

    tester.reg(name="marshal-test", mode="debug", group="rpc_regression")
    tester.reg(name="marshal-test", mode="valgrind", group="rpc_regression")
    tester.reg(name="marshal-perf", mode="release", group="rpc_regression", repeat=3)
    tester.reg(name="marshal-perf", mode="valgrind", group="rpc_regression")
    tester.reg(name="rpc-test", mode="debug", group="rpc_regression")
    tester.reg(name="rpc-test", mode="valgrind", group="rpc_regression")
    tester.reg(name="rpc-perf", mode="release", group="rpc_regression")
    tester.reg(name="rpc-perf", mode="valgrind", group="rpc_regression")
    tester.reg(name="ohcount", mode="python", func=ohcount_lines)

    tester.run()

