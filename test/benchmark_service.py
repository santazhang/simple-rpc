# generated from 'benchmark_service.rpc'

from simplerpc import Marshal

point3 = Marshal.reg_type('point3', [('x', 'double'), ('y', 'double'), ('z', 'double')])

class BenchmarkService(object):
    FAST_PRIME = 0x35e19c6a
    FAST_DOT_PROD = 0x232cccc7
    FAST_ADD = 0x3c848192
    FAST_NOP = 0x6109833a
    PRIME = 0x20276e17
    DOT_PROD = 0x61da5cb1
    ADD = 0x273b15c7
    NOP = 0x3ba571e9

    __input_type_info__ = {
        'fast_prime': ['rpc::i32'],
        'fast_dot_prod': ['point3','point3'],
        'fast_add': ['rpc::i32','rpc::i32'],
        'fast_nop': ['std::string'],
        'prime': ['rpc::i32'],
        'dot_prod': ['point3','point3'],
        'add': ['rpc::i32','rpc::i32'],
        'nop': ['std::string'],
    }

    __output_type_info__ = {
        'fast_prime': ['rpc::i32'],
        'fast_dot_prod': ['double'],
        'fast_add': ['rpc::i32'],
        'fast_nop': [],
        'prime': ['rpc::i32'],
        'dot_prod': ['double'],
        'add': ['rpc::i32'],
        'nop': [],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(BenchmarkService.FAST_PRIME, self.__bind_helper__(self.fast_prime), ['rpc::i32'], ['rpc::i32'])
        server.__reg_func__(BenchmarkService.FAST_DOT_PROD, self.__bind_helper__(self.fast_dot_prod), ['point3','point3'], ['double'])
        server.__reg_func__(BenchmarkService.FAST_ADD, self.__bind_helper__(self.fast_add), ['rpc::i32','rpc::i32'], ['rpc::i32'])
        server.__reg_func__(BenchmarkService.FAST_NOP, self.__bind_helper__(self.fast_nop), ['std::string'], [])
        server.__reg_func__(BenchmarkService.PRIME, self.__bind_helper__(self.prime), ['rpc::i32'], ['rpc::i32'])
        server.__reg_func__(BenchmarkService.DOT_PROD, self.__bind_helper__(self.dot_prod), ['point3','point3'], ['double'])
        server.__reg_func__(BenchmarkService.ADD, self.__bind_helper__(self.add), ['rpc::i32','rpc::i32'], ['rpc::i32'])
        server.__reg_func__(BenchmarkService.NOP, self.__bind_helper__(self.nop), ['std::string'], [])

    def fast_prime(self, marshal):
        raise NotImplementedError('subclass BenchmarkService and implement your own fast_prime function')

    def fast_dot_prod(self, marshal):
        raise NotImplementedError('subclass BenchmarkService and implement your own fast_dot_prod function')

    def fast_add(self, marshal):
        raise NotImplementedError('subclass BenchmarkService and implement your own fast_add function')

    def fast_nop(self, marshal):
        raise NotImplementedError('subclass BenchmarkService and implement your own fast_nop function')

    def prime(self, marshal):
        raise NotImplementedError('subclass BenchmarkService and implement your own prime function')

    def dot_prod(self, marshal):
        raise NotImplementedError('subclass BenchmarkService and implement your own dot_prod function')

    def add(self, marshal):
        raise NotImplementedError('subclass BenchmarkService and implement your own add function')

    def nop(self, marshal):
        raise NotImplementedError('subclass BenchmarkService and implement your own nop function')

class BenchmarkProxy(object):
    def __init__(self, clnt):
        self.clnt = clnt

    def async_fast_prime(TODO):
        pass

    def async_fast_dot_prod(TODO):
        pass

    def async_fast_add(TODO):
        pass

    def async_fast_nop(TODO):
        pass

    def async_prime(TODO):
        pass

    def async_dot_prod(TODO):
        pass

    def async_add(TODO):
        pass

    def async_nop(TODO):
        pass

    def sync_fast_prime(TODO):
        pass

    def sync_fast_dot_prod(TODO):
        pass

    def sync_fast_add(TODO):
        pass

    def sync_fast_nop(TODO):
        pass

    def sync_prime(TODO):
        pass

    def sync_dot_prod(TODO):
        pass

    def sync_add(TODO):
        pass

    def sync_nop(TODO):
        pass

