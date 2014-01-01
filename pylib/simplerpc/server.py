import traceback

from simplerpc import _pyrpc
from simplerpc.marshal import Marshal

# f input: input_marshal (id only), f output: output_marshal (id only)
def marshal_wrap(f, input_types, output_types):

    def wrap_f(input_marshal_id):
        input_m = Marshal(id=input_marshal_id, should_release=False)

        input_values = []
        for input_ty in input_types:
            input_values += input_m.read_obj(input_ty),
        try:
            output = f(*input_values)
        except:
            traceback.print_exc()
            raise

        if len(output_types) == 0:
            # void rpc
            return 0 # mark as a NULL reply

        output_m = Marshal(should_release=False) # C++ code will release the marshal object
        if len(output_types) == 1:
            # single return value
            output_m.write_obj(output, output_types[0])
        else:
            # multiple return values
            for i in range(len(output_types)):
                output_m.write_obj(output[i], output_types[i])

        return output_m.id

    return wrap_f


class Server(object):
    def __init__(self):
        self.id = _pyrpc.init_server()

    def __del__(self):
        _pyrpc.fini_server(self.id)

    def __reg_func__(self, rpc_id, func, input_types, output_types):
        return _pyrpc.server_reg(self.id, rpc_id, marshal_wrap(func, input_types, output_types))

    def reg_svc(self, svc):
        svc.__reg_to__(self)

    def unreg(self, rpc_id):
        _pyrpc.server_unreg(self.id, rpc_id)

    def start(self, addr):
        return _pyrpc.server_start(self.id, addr)
