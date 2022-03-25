from engine.ast import Context


class Types:
    name = 'Any'
    cname = 'void*'
    ctype_name = "types::NONE"
    def __init__(self, context:Context):
        self.cxt = context
    def cast_to(self, *_):
        return self
    def cast_from(self, *_):
        return self
    def __repr__(self) -> str:
        return self.cname

class String(Types):
    name = 'String'
    cname = 'const char*'
    ctype_name = "types::ASTR"
    def cast_from(self, ty, val, container = None):
        if type(ty) is Int:
            self.cxt.emit()

class Int(Types):
    name = "Int"
    cname = "int"
    ctype_name = "types::AINT"
class Float(Types):
    name = "Float"
    cname = "float"
    ctype_name = "types::AFLOAT"
    