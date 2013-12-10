import collections

class Marshal(object):
    
    @staticmethod
    def reg_type(type, fields):
        print "TODO"
        # fields -> (name, type)
        return collections.namedtuple(type, [field[0] for field in fields])

