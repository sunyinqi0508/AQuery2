#ifndef __MONETDB_CONN_H__
#define __MONETDB_CONN_H__
#include "DataSource_conn.h"

struct MonetdbServer : DataSource {
    explicit MonetdbServer(Context* cxt); 
    void connect(Context* cxt) override;
    void exec(const char* q) override;
    void *getCol(int col_idx, int) override;
    void getDSTable(const char* name, void* tbl) override;
    long long getFirstElement();
    void close() override;
    bool haserror() override;
    static bool havehge();
    void print_results(const char* sep = " ", const char* end = "\n", 
        uint32_t limit = std::numeric_limits<uint32_t>::max()) override;
    friend void print_monetdb_results(void* _srv, const char* sep, const char* end, int limit);
    ~MonetdbServer() override;
};

struct monetdbe_table_data{
    const char* table_name;
    const char* create_table_sql;
    void* cols;
};

extern "C" size_t 
monetdbe_get_size(void* dbhdl, const char *table_name, void*);

extern "C" void* 
monetdbe_get_col(void* dbhdl, const char *table_name, uint32_t col_id);

extern "C" void
monetdbe_get_cols(void* dbhdl, const char* table_name, void*** cols, int i);

#endif
