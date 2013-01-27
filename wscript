APPNAME="simple-rpc"
VERSION="0.1"


import os
from waflib.Build import BuildContext


def options(opt):
    opt.load("compiler_cxx")


def configure(conf):
    conf.load("compiler_cxx")
    orig_env = conf.env

    conf.setenv("debug", orig_env)
    conf.env.append_value("CXXFLAGS", ["-ggdb", "-Wall"])

    conf.setenv("release", orig_env)
    conf.env.append_value("CXXFLAGS", ["-O3", "-Wall"])


def build(bld):
    if bld.cmd == "build":
        if os.system("./waf --jobs %d release debug" % bld.jobs) != 0:
            exit(1)
    else:
        def _depend(target, source, action):
            if os.path.exists(target) == False or os.stat(target).st_mtime < os.stat(source).st_mtime:
                print action
                os.system(action)

        _depend("./rpc/rpcgen.py", "./rpc/rpcgen.g", "./yapps/main.py ./rpc/rpcgen.g ; chmod a+x ./rpc/rpcgen.py")
        _depend("./test/rpc_regression.h", "./test/rpc_regression.i", "./rpc/rpcgen.py ./test/rpc_regression.i")

        bld.stlib(source=bld.path.ant_glob("rpc/*.cc"), target="rpc", includes="rpc", lib="pthread")

        def _prog(source, target):
            bld.program(source=source, target=target, includes=".", use=["rpc"], lib=["pthread"])

        _prog("test/rpc_regression.cc", "rpc_regression")


def test(ctx):
    """run regression tests"""
    # support "./waf test <case-1> .. <case-N>"
    os.system("./test/regression.py %s" % (" ".join(Options.commands)))
    Options.commands = []


class debug(BuildContext):
    """build debug binaries"""
    cmd = "debug"
    variant = "debug"


class release(BuildContext):
    """build release binaries"""
    cmd = "release"
    variant = "release"
