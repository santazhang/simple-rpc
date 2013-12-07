import _pyrpc

class Server(object):
    def __init__(self):
        self.id = _pyrpc.init_server()

    def __del__(self):
        _pyrpc.fini_server(self.id)

