#ifndef __DUCKDB_CONN_H__
#define __DUCKDB_CONN_H__
#include "DataSource_conn.h"

struct DuckdbServer : DataSource {
    explicit DuckdbServer(Context* cxt = nullptr);
    void connect(Context* cxt);
    void exec(const char* q);
    void* getCol(int col_idx, int type);
    long long getFirstElement();
    void close();
    bool haserror();
    void print_results(const char* sep = " ", const char* end = "\n");
    ~DuckdbServer();
};
#endif //__DUCKDB_CONN_H__