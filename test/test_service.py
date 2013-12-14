# generated from 'test_service.rpc'

from simplerpc import Marshal

empty_struct = Marshal.reg_type('empty_struct', [])

class empty_serviceService(object):

    __input_type_info__ = {
    }

    __output_type_info__ = {
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def reg_to(self, server):
        pass

class empty_serviceProxy(object):
    def __init__(self, clnt):
        self.clnt = clnt

