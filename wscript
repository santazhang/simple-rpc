APPNAME="simple-rpc"
VERSION="0.1"

import os
import shutil
from waflib import Logs

def options(opt):
    opt.load("compiler_cxx")

def configure(conf):
    conf.load("compiler_cxx")

def build(bld):
    def _depend(target, source, action):
        if os.path.exists(target) == False or os.stat(target).st_mtime < os.stat(source).st_mtime:
            Logs.pprint('PINK', action)
            os.system(action)

    _depend("./rpc/rpcgen.py", "./rpc/rpcgen.g", "./pylib/yapps/main.py ./rpc/rpcgen.g ; chmod a+x ./rpc/rpcgen.py")
    bld.stlib(source=bld.path.ant_glob("rpc/*.cc"), target="simplerpc", includes="rpc", lib="pthread")

    def _prog(source, target):
        bld.program(source=source, target=target, includes=".", use=["lynx", "rpc"], lib=["pthread"])

