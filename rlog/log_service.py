# generated from 'log_service.rpc'

from simplerpc import Marshal

class RLogService(object):
    LOG = 0x28a5fff9
    AGGREGATE_QPS = 0x2adc4f82

    __input_type_info__ = {
        'log': ['rpc::i32','std::string','rpc::i64','std::string'],
        'aggregate_qps': ['std::string','rpc::i32'],
    }

    __output_type_info__ = {
        'log': [],
        'aggregate_qps': [],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(RLogService.LOG, self.__bind_helper__(self.log), ['rpc::i32','std::string','rpc::i64','std::string'], [])
        server.__reg_func__(RLogService.AGGREGATE_QPS, self.__bind_helper__(self.aggregate_qps), ['std::string','rpc::i32'], [])

    def log(self, marshal):
        raise NotImplementedError('subclass RLogService and implement your own log function')

    def aggregate_qps(self, marshal):
        raise NotImplementedError('subclass RLogService and implement your own aggregate_qps function')

class RLogProxy(object):
    def __init__(self, clnt):
        self.clnt = clnt

    def async_log(TODO):
        pass

    def async_aggregate_qps(TODO):
        pass

    def sync_log(TODO):
        pass

    def sync_aggregate_qps(TODO):
        pass

