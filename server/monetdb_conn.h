#ifndef __MONETDB_CONN_H__
#define __MONETDB_CONN_H__

struct Context;

struct Server{
    void *server = 0;
    Context *cxt = 0;
    bool status = 0;
    char* query = 0;
    int type = 1;

    void* res = 0;
    void* ret_col = 0;
    long long cnt = 0;
    char* last_error = 0;
    
    Server(Context* cxt = nullptr);
    void connect(Context* cxt);
    void exec(const char* q);
    void *getCol(int col_idx);
    void close();
    bool haserror();
    static bool havehge();
    ~Server();
};

#endif
