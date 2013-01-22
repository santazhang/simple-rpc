#!/usr/bin/env python

import os
import sys
import subprocess
import traceback
import collections
import signal
import time

import yaml
import yaml.constructor

try:
    # included in standard lib from Python 2.7
    from collections import OrderedDict
except ImportError:
    # try importing the backported drop-in replacement
    # it's available on PyPI
    from ordereddict import OrderedDict

# from http://gits.github.com/844388
class OrderedDictYAMLLoader(yaml.Loader):
    """
    A YAML loader that loads mappings into ordered dictionaries.
    """

    def __init__(self, *args, **kwargs):
        yaml.Loader.__init__(self, *args, **kwargs)

        self.add_constructor(u'tag:yaml.org,2002:map', type(self).construct_yaml_map)
        self.add_constructor(u'tag:yaml.org,2002:omap', type(self).construct_yaml_map)

    def construct_yaml_map(self, node):
        data = OrderedDict()
        yield data
        value = self.construct_mapping(node)
        data.update(value)

    def construct_mapping(self, node, deep=False):
        if isinstance(node, yaml.MappingNode):
            self.flatten_mapping(node)
        else:
            raise yaml.constructor.ConstructorError(None, None,
                'expected a mapping node, but found %s' % node.id, node.start_mark)

        mapping = OrderedDict()
        for key_node, value_node in node.value:
            key = self.construct_object(key_node, deep=deep)
            try:
                hash(key)
            except TypeError, exc:
                raise yaml.constructor.ConstructorError('while constructing a mapping',
                    node.start_mark, 'found unacceptable key (%s)' % exc, key_node.start_mark)
            value = self.construct_object(value_node, deep=deep)
            mapping[key] = value
        return mapping


def write_log(msg):
    # colors: 0;30-0;37, 1:30-1;37
    sys.stdout.write("\x1B[1;36m" + "*** regression.py: " + msg + "\x1B[0m\n")
    sys.stdout.flush()


def ohcount_lines(folders):
    ret = ""
    folders = map(lambda x: "../" + x, folders)
    cmd = "ohcount %s" % (" ".join(folders))
    p = os.popen(cmd)
    pout = p.read()
    p.close()
    assert pout != ""
    idx = pout.find("----")
    idx2 = pout.rfind("----")
    for line in pout[idx:idx2].split("\n")[1:-1]:
        sp = line.split()
        ret += "%s: %s\n" % (sp[0], sp[2])
    return ret.strip()


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


def should_skip(case_name):
    interested = sys.argv[1:]
    if len(interested) == 0:
        return False
    else:
        return not case_name in interested


def regression_group(name, conf):
    if conf == None:
        if should_skip(name):
            return
        checked_exec("../build/debug/%s" % name)
        checked_exec("../build/release/%s" % name)
        return
    for case in conf:
        if should_skip(case):
            write_log("skip case: %s > %s" % (name, case))
            continue
        write_log("test case: %s > %s" % (name, case))
        case_conf = conf[case]
        for test_variant in case_conf:
            if test_variant.startswith("valgrind-"):
                build_variant = test_variant[9:]
                bin_path = "../build/%s/%s %s" % (build_variant, name, case)
                cmd = "valgrind --leak-check=full %s" % bin_path
            else:
                build_variant = test_variant
                bin_path = "../build/%s/%s %s" % (build_variant, name, case)
                cmd = bin_path
            if type([]) == type(case_conf):
                checked_exec(cmd)
            else:
                run_conf = case_conf[test_variant]
                if run_conf.has_key("args") and run_conf["args"] != None:
                    cmd += " " + run_conf["args"]
                repeat = 1
                if run_conf.has_key("repeat") and run_conf["repeat"] != None:
                    repeat = run_conf["repeat"]
                for r in range(1, repeat + 1):
                    if repeat != 1:
                        write_log("run %d/%d" % (r, repeat))
                    checked_exec(cmd)


def cluster_chain_perf(conf):
    f = open("server_list.txt")
    server_list = f.read().split()
    f.close()

    cwd = os.path.abspath(os.getcwd())
    checked_exec("mkdir -p log report")

    server_nodes = []
    for server_addr in server_list:
        server_node = server_addr.split(":")[0]
        if server_node not in server_nodes:
            server_nodes += server_node,

    # need a clean starting point
    write_log("kill servers")
    for node in server_nodes:
        checked_exec("ssh -o PasswordAuthentication=no %s pkill -9 lynx_server" % node)

    write_log("start servers")
    for node in server_nodes:
        for server_addr in server_list:
            if not server_addr.startswith(node):
                continue
            cmd = "ssh -o PasswordAuthentication=no %s %s/../build/release/lynx_server --bind=%s --list=%s/server_list.txt --log=%s/log/%s.log --time-limit=1000" % (node, cwd, server_addr, cwd, cwd, server_addr)
            checked_exec(cmd)

    cmd = "../build/release/lynx_regression cluster-chain-perf-client server_list.txt"
    checked_exec(cmd)

    # cleanup
    write_log("kill servers")
    for node in server_nodes:
        checked_exec("ssh -o PasswordAuthentication=no %s pkill -9 lynx_server" % node)


def functional_group(name, conf):
    if should_skip(name):
        write_log("skip functional: %s" % name)
        return
    write_log("test functional: %s" % name)
    if name == "cluster-chain-perf":
        cluster_chain_perf(conf)
    else:
        print "TODO: implement %s" % name

#
# MAIN ENTRY
#
if __name__ == "__main__":

    # change cwd to script dir
    os.chdir(os.path.split(os.path.abspath(__file__))[0])

    f = open("regression.yml", "r")
    test_conf = yaml.load(f, OrderedDictYAMLLoader)
    f.close()

    # make sure we have the newest binary
    if os.system("../waf") != 0:
        write_log("build failed, cannot run regression test!")
        exit(-1)

    for group_name in test_conf:
        write_log("test group: %s" % group_name)
        group_conf = test_conf[group_name]
        if group_name == "<ohcount>":
            if len(sys.argv) == 1:
                # did not specify which test to run, so all tests will be run
                print ohcount_lines(group_conf)
            else:
                write_log("skip group: <ohcount>")
        elif group_name == "<functional>":
            for name in group_conf:
                conf = group_conf[name]
                functional_group(name, conf)
        else:
            regression_group(group_name, group_conf)

