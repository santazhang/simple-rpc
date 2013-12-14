# generated from 'pyonly.rpc'

from simplerpc import Marshal

class PythonOnlyService(object):
    PYONLY_RPC = 0x3efe015a

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

    def pyonly_rpc(self, marshal):
        raise NotImplementedError('subclass PythonOnlyService and implement your own pyonly_rpc function')

class PythonOnlyProxy(object):
    def __init__(self, clnt):
        self.clnt = clnt

    def async_pyonly_rpc(TODO):
        pass

    def sync_pyonly_rpc(TODO):
        pass

