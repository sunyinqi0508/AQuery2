#include "monetdbe.h"

struct Context;

struct Server{
    monetdbe_database *server = 0;
    Context *cxt = 0;
    bool status = 0;
    char* query = 0;
    int type = 1;

    monetdbe_result* res = 0;
    monetdbe_column* ret_col = 0;
    monetdbe_cnt cnt = 0;

    void connect(Context* cxt);
    void exec(const char* q);
    void *getCol(int col_idx);
    void close();
    ~Server();
};