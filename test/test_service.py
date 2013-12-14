# generated from 'test_service.rpc'

from simplerpc import Marshal

empty_struct = Marshal.reg_type('empty_struct', [])

complex_struct = Marshal.reg_type('complex_struct', [('d', 'std::map<std::string, std::vector<std::vector<std::string>>>'), ('s', 'std::set<std::string>')])

class empty_serviceService(object):

    __input_type_info__ = {
    }

    __output_type_info__ = {
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        pass

class empty_serviceProxy(object):
    def __init__(self, clnt):
        self.clnt = clnt

class math_serviceService(object):
    GCD = 0x6a2d6174

    __input_type_info__ = {
        'gcd': ['rpc::i64','rpc::i64'],
    }

    __output_type_info__ = {
        'gcd': ['rpc::i64'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(math_serviceService.GCD, self.__bind_helper__(self.gcd), ['rpc::i64','rpc::i64'], ['rpc::i64'])

    def gcd(self, marshal):
        raise NotImplementedError('subclass math_serviceService and implement your own gcd function')

class math_serviceProxy(object):
    def __init__(self, clnt):
        self.clnt = clnt

    def async_gcd(TODO):
        pass

    def sync_gcd(TODO):
        pass

