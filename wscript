APPNAME="simple-rpc"
VERSION="0.1"


import os


def options(opt):
    opt.load("compiler_cxx")


def configure(conf):
    orig_env = conf.env
    conf.setenv("debug")
    conf.load("compiler_cxx")
    conf.env.CXXFLAGS = "-ggdb"

    conf.setenv("release", orig_env)
    conf.load("compiler_cxx")
    conf.env.CXXFLAGS = "-O3"


def build(bld):
    if bld.cmd == "build":
        if os.system("./waf release debug") != 0:
            exit(1)
    else:
        bld.stlib(source=bld.path.ant_glob("rpc/*.cc"), target="rpc", includes="rpc", lib="pthread")

        def _prog(source, target):
            bld.program(source=source, target=target, includes=".", use="rpc", lib="pthread")

        _prog("test/rpc_regression.cc", "rpc_regression")


def test(ctx):
    os.system("./test/regression.py")


from waflib.Build import BuildContext


class debug(BuildContext):
    cmd = "debug"
    variant = "debug"


class release(BuildContext):
    cmd = "release"
    variant = "release"
