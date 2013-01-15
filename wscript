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
    """run regression tests"""
    os.system("./test/regression.py")


def rpcgen(ctx):
    """generate rpc code from definition files"""
    def _exec(cmd):
        print cmd
        os.system(cmd)

    _exec("./yapps/main.py ./rpc/rpcgen.g")
    _exec("chmod a+x ./rpc/rpcgen.py")
    _exec("./rpc/rpcgen.py test/rpc_regression.def")


from waflib.Build import BuildContext


class debug(BuildContext):
    """build debug binaries"""
    cmd = "debug"
    variant = "debug"


class release(BuildContext):
    """build release binaries"""
    cmd = "release"
    variant = "release"
