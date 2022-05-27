from reconstruct.ast import Context, ast_node

def initialize():
    return Context()    

def generate(ast, cxt):
    for k in ast.keys():
        if k in ast_node.types.keys():
            ast_node.types[k](None, ast, cxt)
            
__all__ = ["initialize", "generate"]
