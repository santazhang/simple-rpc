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
