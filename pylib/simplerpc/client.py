import errno
from simplerpc import Marshal
from simplerpc import _pyrpc

class Client(object):

    pollmgr = None

    def __init__(self):
        if Client.pollmgr == None:
            Client.pollmgr = _pyrpc.init_poll_mgr()
        self.id = _pyrpc.init_client(Client.pollmgr)

    def __del__(self):
        _pyrpc.fini_client(self.id)

    def connect(self, addr):
        _pyrpc.client_connect(self.id, addr)

    def sync_call(self, rpc_id, req_values, req_types, rep_types):
        req_m = Marshal()
        for i in range(len(req_values)):
            req_m.write_obj(req_values[i], req_types[i])
        error_code, rep_marshal_id = _pyrpc.client_sync_call(self.id, rpc_id, req_m.id)
        results = []
        if rep_marshal_id != 0 and error_code == 0:
            rep_m = Marshal(id=rep_marshal_id)
            for ty in rep_types:
                results += rep_m.read_obj(ty),
        return error_code, results

    def async_call(self, rpc_id, req_values, req_types, rep_types, done_cb):
        req_m = Marshal()
        for i in range(len(req_values)):
            req_m.write_obj(req_values[i], req_types[i])

        if done_cb == None:
            fu_id = _pyrpc.client_async_call(self.id, rpc_id, req_m.id, None)
        else:
            
            def wrapped_cb(fu_id):
                # make sure done_cb has input ([error_code], [results])
                fu = Future(id=fu_id)
                results = []
                if fu.get_error_code() == 0:
                    rep_m = fu.get_reply()
                    for ty in rep_types:
                        results += rep_m.read_obj(ty),
                done_cb(fu.get_error_code(), results)

            fu_id = _pyrpc.client_async_call(self.id, rpc_id, req_m.id, wrapped_cb)
            if fu_id == 0: # ENOTCONN
                done_cb(errno.ENOTCONN, [])

        return fu_id
