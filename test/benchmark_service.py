# generated from 'benchmark_service.rpc'

import collections

point3 = collections.namedtuple('point3', ['x', 'y', 'z'])

class BenchmarkService(object):
    FAST_PRIME = 0x509ec60e
    FAST_DOT_PROD = 0x6c57cc74
    FAST_ADD = 0x1ae8267d
    FAST_NOP = 0x26a90eb2
    PRIME = 0x338abf4c
    DOT_PROD = 0x4b9286d6
    ADD = 0x219e9e1a
    NOP = 0x35813eae

    def reg_to(self, svc):
        pass

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

