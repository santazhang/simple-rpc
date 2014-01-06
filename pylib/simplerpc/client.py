import errno
from threading import Thread
from threading import Lock
from simplerpc import _pyrpc
from simplerpc.marshal import Marshal
from simplerpc.future import Future

class Client(object):

    pollmgr = None

    def __init__(self):
        if Client.pollmgr == None:
            Client.pollmgr = _pyrpc.init_poll_mgr()
        self.id = _pyrpc.init_client(Client.pollmgr)

        # for async rpc
        self.closed = False
        self.cb_counter = 1 # 0 means no callback
        self.cb_func = {} # cb_id => cb_func
        self.cb_queue_id = _pyrpc.init_async_queue()
        self.async_m = Lock()
        self.cb_thread = Thread(target=self.cb_loop)
        self.cb_thread.start()

    def __del__(self):
        if not self.closed:
            _pyrpc.fini_client(self.id)

    def connect(self, addr):
        _pyrpc.client_connect(self.id, addr)

    def close(self):
        self.async_m.acquire()
        self.closed = True
        _pyrpc.async_queue_push(self.cb_queue_id, -1) # death pill
        self.async_m.release()
        self.cb_thread.join()
        _pyrpc.fini_async_queue(self.cb_queue_id)
        _pyrpc.fini_client(self.id)

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

    def cb_loop(self):
        while not self.closed:
            cb_id = _pyrpc.async_queue_pop(self.cb_queue_id)
            if cb_id == -1:
                continue
            fu_id = _pyrpc.async_queue_pop(self.cb_queue_id)
            self.async_m.acquire()
            func = self.cb_func.pop(cb_id)
            self.async_m.release()
            func(fu_id)

    def async_call(self, rpc_id, req_values, req_types, rep_types, done_cb):
        req_m = Marshal()
        for i in range(len(req_values)):
            req_m.write_obj(req_values[i], req_types[i])

        self.async_m.acquire()
        if done_cb:

            def wrapped_cb(fu_id):
                # make sure done_cb has input ([error_code], [results])
                fu = Future(id=fu_id)
                results = []
                if fu.get_error_code() == 0:
                    rep_m = fu.get_reply()
                    for ty in rep_types:
                        results += rep_m.read_obj(ty),
                done_cb(fu.get_error_code(), results)

            cb_id = self.cb_counter
            self.cb_counter += 1
            self.cb_func[cb_id] = wrapped_cb
        else:
            cb_id = 0 # no callback
        fu_id = _pyrpc.client_async_call(self.id, self.cb_queue_id, rpc_id, req_m.id, cb_id)
        self.async_m.release()

        return Future(fu_id)
