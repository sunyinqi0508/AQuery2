#ifndef __MONETDB_CONN_H__
#define __MONETDB_CONN_H__

struct Context;

struct Server{
    void *server = nullptr;
    Context *cxt = nullptr;
    bool status = false;
    char* query = nullptr;
    int type = 1;

    void* res = nullptr;
    void* ret_col = nullptr;
    long long cnt = 0;
    char* last_error = nullptr;
    
    explicit Server(Context* cxt = nullptr);
    void connect(Context* cxt);
    void exec(const char* q);
    void *getCol(int col_idx);
    long long getFirstElement();
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

extern "C" size_t 
monetdbe_get_size(void* dbhdl, const char *table_name);

extern "C" void* 
monetdbe_get_col(void* dbhdl, const char *table_name, uint32_t col_id);
#endif
