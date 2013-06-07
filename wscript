serverAPPNAME="simple-rpc"
VERSION="0.1"

import os
import shutil
from waflib import Logs

def options(opt):
    opt.load("compiler_cxx")
    opt.add_option("--debug", action="store_true", default=False, help="enable debug support")

def configure(conf):
    conf.load("compiler_cxx")
    if conf.options.debug:
        Logs.pprint("PINK", "debug support enabled")
        conf.env.append_value("CXXFLAGS", ["-ggdb", "-Wall"])
    else:
        conf.env.append_value("CXXFLAGS", ["-O3", "-fno-omit-frame-pointer", "-Wall"])

def build(bld):
    def _depend(target, source, action):
        if source != None and os.path.exists(source) == False:
            Logs.pprint('RED', "'%s' not found!" % source)
            exit(1)
        if os.path.exists(target) == False or os.stat(target).st_mtime < os.stat(source).st_mtime:
            Logs.pprint('PINK', action)
            os.system(action)

    _depend("rpc/rpcgen.py", "rpc/rpcgen.g", "pylib/yapps/main.py rpc/rpcgen.g ; chmod a+x rpc/rpcgen.py")
    _depend("test/demo_service.h", "test/demo_service.rpc", "rpc/rpcgen.py test/demo_service.rpc")

    bld.stlib(source=bld.path.ant_glob("rpc/*.cc"), target="simplerpc", includes="rpc", lib="pthread")
    bld.stlib(source="test/param_map.cc", includes=".", target = 'test', name = 'test')

    def _prog(source, target):
        bld.program(source=source, target=target, includes=".", use="simplerpc", lib="pthread")

    _prog("test/demo_client.cc", "demo_client")
    _prog("test/demo_server.cc test/demo_service.cc", "demo_server")

    # Unit tests and benchmark, inherented from MCP code
    bld.program(source="test/param_map_test.cc", target="param_map_test",
        includes=".", use=["simplerpc", "test"], lib="pthread")
    bld.program(source="test/callback_test.cc", target="callback_test",
        includes=".", use=["simplerpc", "test"], lib="pthread")
    bld.program(source="test/callback_benchmark.cc", target="callback_benchmark",
        includes=".", use=["simplerpc", "test"], lib="pthread")
