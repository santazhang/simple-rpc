# generated from 'log_service.rpc'

import os
from simplerpc import Marshal

class RLogService(object):
    LOG = 0x2df25d3c
    AGGREGATE_QPS = 0x4b41d147

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

    def log(__self__, level, source, msg_id, message):
        raise NotImplementedError('subclass RLogService and implement your own log function')

    def aggregate_qps(__self__, metric_name, increment):
        raise NotImplementedError('subclass RLogService and implement your own aggregate_qps function')

class RLogProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_log(TODO):
        pass

    def async_aggregate_qps(TODO):
        pass

    def sync_log(__self__, level, source, msg_id, message):
        __result__ = __self__.__clnt__.sync_call(RLogService.LOG, [level, source, msg_id, message], RLogService.__input_type_info__['log'], RLogService.__output_type_info__['log'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_aggregate_qps(__self__, metric_name, increment):
        __result__ = __self__.__clnt__.sync_call(RLogService.AGGREGATE_QPS, [metric_name, increment], RLogService.__input_type_info__['aggregate_qps'], RLogService.__output_type_info__['aggregate_qps'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

