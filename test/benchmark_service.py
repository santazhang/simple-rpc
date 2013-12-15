# generated from 'benchmark_service.rpc'

import os
from simplerpc import Marshal
from simplerpc import Future

point3 = Marshal.reg_type('point3', [('x', 'double'), ('y', 'double'), ('z', 'double')])

class BenchmarkService(object):
    FAST_PRIME = 0x6af97d0b
    FAST_DOT_PROD = 0x63893c77
    FAST_ADD = 0x553d16ab
    FAST_NOP = 0x486ca867
    PRIME = 0x12d7e9ee
    DOT_PROD = 0x37224de3
    ADD = 0x3d06af47
    NOP = 0x4e82eb4d

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

    def fast_prime(__self__, n):
        raise NotImplementedError('subclass BenchmarkService and implement your own fast_prime function')

    def fast_dot_prod(__self__, p1, p2):
        raise NotImplementedError('subclass BenchmarkService and implement your own fast_dot_prod function')

    def fast_add(__self__, a, b):
        raise NotImplementedError('subclass BenchmarkService and implement your own fast_add function')

    def fast_nop(__self__, in0):
        raise NotImplementedError('subclass BenchmarkService and implement your own fast_nop function')

    def prime(__self__, n):
        raise NotImplementedError('subclass BenchmarkService and implement your own prime function')

    def dot_prod(__self__, p1, p2):
        raise NotImplementedError('subclass BenchmarkService and implement your own dot_prod function')

    def add(__self__, a, b):
        raise NotImplementedError('subclass BenchmarkService and implement your own add function')

    def nop(__self__, in0):
        raise NotImplementedError('subclass BenchmarkService and implement your own nop function')

class BenchmarkProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    #def async_fast_prime(__self__, n, __done_callback__=None):
        #__fu_id__ = __self__.__clnt__.async_call(BenchmarkService.FAST_PRIME, [n], BenchmarkService.__input_type_info__['fast_prime'], BenchmarkService.__output_type_info__['fast_prime'], __done_callback__)
        #if __fu_id__ != 0:
            #return Future(id=__fu_id__)

    #def async_fast_dot_prod(__self__, p1, p2, __done_callback__=None):
        #__fu_id__ = __self__.__clnt__.async_call(BenchmarkService.FAST_DOT_PROD, [p1, p2], BenchmarkService.__input_type_info__['fast_dot_prod'], BenchmarkService.__output_type_info__['fast_dot_prod'], __done_callback__)
        #if __fu_id__ != 0:
            #return Future(id=__fu_id__)

    #def async_fast_add(__self__, a, b, __done_callback__=None):
        #__fu_id__ = __self__.__clnt__.async_call(BenchmarkService.FAST_ADD, [a, b], BenchmarkService.__input_type_info__['fast_add'], BenchmarkService.__output_type_info__['fast_add'], __done_callback__)
        #if __fu_id__ != 0:
            #return Future(id=__fu_id__)

    #def async_fast_nop(__self__, in0, __done_callback__=None):
        #__fu_id__ = __self__.__clnt__.async_call(BenchmarkService.FAST_NOP, [in0], BenchmarkService.__input_type_info__['fast_nop'], BenchmarkService.__output_type_info__['fast_nop'], __done_callback__)
        #if __fu_id__ != 0:
            #return Future(id=__fu_id__)

    #def async_prime(__self__, n, __done_callback__=None):
        #__fu_id__ = __self__.__clnt__.async_call(BenchmarkService.PRIME, [n], BenchmarkService.__input_type_info__['prime'], BenchmarkService.__output_type_info__['prime'], __done_callback__)
        #if __fu_id__ != 0:
            #return Future(id=__fu_id__)

    #def async_dot_prod(__self__, p1, p2, __done_callback__=None):
        #__fu_id__ = __self__.__clnt__.async_call(BenchmarkService.DOT_PROD, [p1, p2], BenchmarkService.__input_type_info__['dot_prod'], BenchmarkService.__output_type_info__['dot_prod'], __done_callback__)
        #if __fu_id__ != 0:
            #return Future(id=__fu_id__)

    #def async_add(__self__, a, b, __done_callback__=None):
        #__fu_id__ = __self__.__clnt__.async_call(BenchmarkService.ADD, [a, b], BenchmarkService.__input_type_info__['add'], BenchmarkService.__output_type_info__['add'], __done_callback__)
        #if __fu_id__ != 0:
            #return Future(id=__fu_id__)

    #def async_nop(__self__, in0, __done_callback__=None):
        #__fu_id__ = __self__.__clnt__.async_call(BenchmarkService.NOP, [in0], BenchmarkService.__input_type_info__['nop'], BenchmarkService.__output_type_info__['nop'], __done_callback__)
        #if __fu_id__ != 0:
            #return Future(id=__fu_id__)

    def sync_fast_prime(__self__, n):
        __result__ = __self__.__clnt__.sync_call(BenchmarkService.FAST_PRIME, [n], BenchmarkService.__input_type_info__['fast_prime'], BenchmarkService.__output_type_info__['fast_prime'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_fast_dot_prod(__self__, p1, p2):
        __result__ = __self__.__clnt__.sync_call(BenchmarkService.FAST_DOT_PROD, [p1, p2], BenchmarkService.__input_type_info__['fast_dot_prod'], BenchmarkService.__output_type_info__['fast_dot_prod'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_fast_add(__self__, a, b):
        __result__ = __self__.__clnt__.sync_call(BenchmarkService.FAST_ADD, [a, b], BenchmarkService.__input_type_info__['fast_add'], BenchmarkService.__output_type_info__['fast_add'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_fast_nop(__self__, in0):
        __result__ = __self__.__clnt__.sync_call(BenchmarkService.FAST_NOP, [in0], BenchmarkService.__input_type_info__['fast_nop'], BenchmarkService.__output_type_info__['fast_nop'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_prime(__self__, n):
        __result__ = __self__.__clnt__.sync_call(BenchmarkService.PRIME, [n], BenchmarkService.__input_type_info__['prime'], BenchmarkService.__output_type_info__['prime'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_dot_prod(__self__, p1, p2):
        __result__ = __self__.__clnt__.sync_call(BenchmarkService.DOT_PROD, [p1, p2], BenchmarkService.__input_type_info__['dot_prod'], BenchmarkService.__output_type_info__['dot_prod'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_add(__self__, a, b):
        __result__ = __self__.__clnt__.sync_call(BenchmarkService.ADD, [a, b], BenchmarkService.__input_type_info__['add'], BenchmarkService.__output_type_info__['add'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_nop(__self__, in0):
        __result__ = __self__.__clnt__.sync_call(BenchmarkService.NOP, [in0], BenchmarkService.__input_type_info__['nop'], BenchmarkService.__output_type_info__['nop'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]
