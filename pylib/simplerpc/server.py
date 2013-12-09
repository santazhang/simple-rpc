from simplerpc import _pyrpc
try:
    import cPickle as pickle
except ImportError:
    import pickle

def rpc_wrap(f):
    # TODO proper marshaling
    def wrap_f(enc_args):
        args = pickle.loads(enc_args)
        ret = f(*args)
        return pickle.dumps(ret)
    return wrap_f

class Server(object):
    def __init__(self):
        self.id = _pyrpc.init_server()

    def __del__(self):
        _pyrpc.fini_server(self.id)

    def reg_func(self, rpc_id, func):
        return _pyrpc.server_reg(self.id, rpc_id, rpc_wrap(func))

    def reg_svc(self, svc):
        print "THIS SHALL BE DONE!"

    def unreg(self, rpc_id):
        _pyrpc.server_unreg(self.id, rpc_id)

    def start(self, addr):
        return _pyrpc.server_start(self.id, addr)
