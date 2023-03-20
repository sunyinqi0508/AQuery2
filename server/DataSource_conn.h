#ifndef __DATASOURCE_CONN_H__
#define __DATASOURCE_CONN_H__
struct Context;

struct AQQueryResult {
    void* res;
    unsigned ref;
};
enum DataSourceType {
    Invalid, 
    MonetDB, 
    MariaDB, 
    DuckDB, 
    SQLite
};

struct DataSource {
    void* server = nullptr;
    Context* cxt = nullptr;
    bool status = false;
    char* query = nullptr;
    DataSourceType type = Invalid;

    void* res = nullptr;
    void* ret_col = nullptr;
    long long cnt = 0;
    const char* last_error = nullptr;

    void* handle;

    DataSource() = default;
    explicit DataSource(Context* cxt = nullptr) = delete;
    
    virtual void connect(Context* cxt) = 0;
    virtual void exec(const char* q) = 0;
    virtual void* getCol(int col_idx, int type) = 0;
    // virtual long long getFirstElement() = 0;
    virtual void close() = 0;
    virtual bool haserror() = 0;
    // virtual void print_results(const char* sep = " ", const char* end = "\n");
    virtual ~DataSource() = 0;
};
#endif //__DATASOURCE_CONN_H__