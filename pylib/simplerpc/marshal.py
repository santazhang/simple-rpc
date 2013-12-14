import collections
from simplerpc import _pyrpc

class Marshal(object):

    structs = {} # typename -> (ctor, [(field_name, field_type)])

    @staticmethod
    def reg_type(type, fields):
        print "TODO"
        # fields -> (name, type)
        ctor = collections.namedtuple(type, [field[0] for field in fields])
        Marshal.structs[type] = ctor, fields
        return ctor

    def __init__(self):
        self.id = _pyrpc.init_marshal()

    def __del__(self):
        _pyrpc.fini_marshal(self.id)

    def __len__(self):
        return _pyrpc.marshal_size(self.id)

    def write_i32(self, i32):
        _pyrpc.marshal_write_i32(self.id, i32)

    def read_i32(self):
        return _pyrpc.marshal_read_i32(self.id)

    def write_i64(self, i64):
        _pyrpc.marshal_write_i64(self.id, i64)

    def read_i64(self):
        return _pyrpc.marshal_read_i64(self.id)

    def write_double(self, dbl):
        _pyrpc.marshal_write_double(self.id, dbl)

    def read_double(self):
        return _pyrpc.marshal_read_double(self.id)

    def write_str(self, s):
        _pyrpc.marshal_write_str(self.id, s)

    def read_str(self):
        return _pyrpc.marshal_read_str(self.id)

    def write_obj(self, o, obj_t):
        if obj_t in ["rpc::i32", "i32"]:
            return self.write_i32(o)
        elif obj_t in ["rpc::i64", "i64"]:
            return self.write_i64(o)
        elif obj_t == "double":
            return self.write_double(o)
        elif obj_t in ["std::string", "string"]:
            return self.write_str(o)
        else:
            ty = Marshal.structs[obj_t][1]
            for field in ty:
                self.write_obj(getattr(o, field[0]), field[1])

    def read_obj(self, obj_t):
        if obj_t in ["rpc::i32", "i32"]:
            return self.read_i32()
        elif obj_t in ["rpc::i64", "i64"]:
            return self.read_i64()
        elif obj_t == "double":
            return self.read_double()
        elif obj_t in ["std::string", "string"]:
            return self.read_str()
        else:
            ty = Marshal.structs[obj_t][1]
            field_values = []
            for field in ty:
                field_values += self.read_obj(field[1]),
            return Marshal.structs[obj_t][0](*field_values)
