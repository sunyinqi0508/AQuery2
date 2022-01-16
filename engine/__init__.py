from engine.ast import Context, ast_node
import engine.ddl

def initialize():
    return Context()    

def generate(ast, cxt):
    for k in ast.keys():
        if k in ast_node.types.keys():
            root = ast_node.types[k](None, ast, cxt)


__all__ = ["generate"]
