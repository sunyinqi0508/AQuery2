from reconstruct.ast import Context, ast_node

saved_cxt = None

def initialize(cxt = None, keep = False):
    global saved_cxt
    if cxt is None or not keep or type(cxt) is not Context:
        if saved_cxt is None or not keep:
            cxt = Context() 
            saved_cxt = cxt
        else:
            cxt = saved_cxt
    cxt.new()
    return cxt 

def generate(ast, cxt):
    for k in ast.keys():
        if k in ast_node.types.keys():
            ast_node.types[k](None, ast, cxt)

def exec(stmts, cxt = None, keep = False):
    if 'stmts' not in stmts:
        return
    cxt = initialize(cxt, keep)
    stmts_stmts = stmts['stmts']
    if type(stmts_stmts) is list:
        for s in stmts_stmts:
            generate(s, cxt)
    else:
        generate(stmts_stmts, cxt)
    for q in cxt.queries:
        if not q.startswith('O'):
            cxt.print(q.strip())
    return cxt
    
__all__ = ["initialize", "generate", "exec", "saved_cxt"]
