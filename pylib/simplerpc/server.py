from simplerpc import _pyrpc
try:
    import cPickle as pickle
except ImportError:
    import pickle

def pyonly_rpc_wrap(f):
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

    def reg_func(self, rpc_id, func, input_types=None, output_types=None):
        if input_types and output_types:
            print "TODO"
        else:
            return _pyrpc.server_reg(self.id, rpc_id, pyonly_rpc_wrap(func))

    def reg_svc(self, svc):
        svc.reg_to(self)

    def unreg(self, rpc_id):
        _pyrpc.server_unreg(self.id, rpc_id)

    def start(self, addr):
        return _pyrpc.server_start(self.id, addr)
