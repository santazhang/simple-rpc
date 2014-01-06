from simplerpc import _pyrpc
from simplerpc.marshal import Marshal

class Future(object):
    def __init__(self, id):
        self.id = id

    def __del__(self):
        _pyrpc.fini_future(self.id)

    def wait(self):
        _pyrpc.future_wait(self.id)

    def get_error_code(self):
        return _pyrpc.future_get_error_code(self.id)

    def get_reply(self):
        return Marshal(id=_pyrpc.future_get_reply(self.id), should_release=False)

    def ready(self):
        return _pyrpc.future_ready(self.id) != 0

    def result(self):
        pass
