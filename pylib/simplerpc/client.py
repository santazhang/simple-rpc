import _pyrpc
try:
    import cPickle as pickle
except ImportError:
    import pickle

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

    def sync_call(self, rpc_id, *args):
        # TODO proper marshaling
        enc_args = pickle.dumps(args)
        error_code, enc_results = _pyrpc.client_sync_call(self.id, rpc_id, enc_args)
        return pickle.loads(enc_results)
