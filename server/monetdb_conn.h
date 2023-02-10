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
    void test(const char*);
    void print_results(const char* sep = " ", const char* end = "\n");
    friend void print_monetdb_results(void* _srv, const char* sep, const char* end, int limit);
    ~Server();
};

struct monetdbe_table_data{
    const char* table_name;
    const char* create_table_sql;
    void* cols;
};

size_t 
monetdbe_get_size(void* dbhdl, const char *table_name);

void* 
monetdbe_get_col(void* dbhdl, const char *table_name, uint32_t col_id);
#endif
