# generated from 'pyonly.rpc'

import os
from simplerpc import Marshal
from simplerpc import Future

class PythonOnlyService(object):
    PYONLY_RPC = 0x23bb5cd6

    __input_type_info__ = {
        'pyonly_rpc': ['std::string'],
    }

    __output_type_info__ = {
        'pyonly_rpc': ['std::string'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(PythonOnlyService.PYONLY_RPC, self.__bind_helper__(self.pyonly_rpc), ['std::string'], ['std::string'])

    def pyonly_rpc(__self__, enc_in):
        raise NotImplementedError('subclass PythonOnlyService and implement your own pyonly_rpc function')

class PythonOnlyProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    #def async_pyonly_rpc(__self__, enc_in, __done_callback__=None):
        #__fu_id__ = __self__.__clnt__.async_call(PythonOnlyService.PYONLY_RPC, [enc_in], PythonOnlyService.__input_type_info__['pyonly_rpc'], PythonOnlyService.__output_type_info__['pyonly_rpc'], __done_callback__)
        #if __fu_id__ != 0:
            #return Future(id=__fu_id__)

    def sync_pyonly_rpc(__self__, enc_in):
        __result__ = __self__.__clnt__.sync_call(PythonOnlyService.PYONLY_RPC, [enc_in], PythonOnlyService.__input_type_info__['pyonly_rpc'], PythonOnlyService.__output_type_info__['pyonly_rpc'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

