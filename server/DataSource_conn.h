#ifndef __DATASOURCE_CONN_H__
#define __DATASOURCE_CONN_H__
struct Context;

#ifndef __AQQueryResult__
#define __AQQueryResult__ 1
struct AQQueryResult {
    void* res;
    unsigned ref;
};
#endif

#ifndef __AQBACKEND_TYPE__
#define __AQBACKEND_TYPE__ 1
enum Backend_Type {
	BACKEND_AQuery,
	BACKEND_MonetDB,
	BACKEND_MariaDB, 
	BACKEND_DuckDB,
	BACKEND_SQLite,
	BACKEND_TOTAL
};
#endif

struct DataSource {
    void* server = nullptr;
    Context* cxt = nullptr;
    bool status = false;
    char* query = nullptr;
    Backend_Type DataSourceType = BACKEND_AQuery;

    void* res = nullptr;
    void* ret_col = nullptr;
    long long cnt = 0;
    const char* last_error = nullptr;

    void* handle;

    DataSource() = default;
    explicit DataSource(Context* cxt) = delete;
    
    virtual void connect(Context* cxt) = 0;
    virtual void exec(const char* q) = 0;
    virtual void* getCol(int col_idx, int type) = 0;
    // virtual long long getFirstElement() = 0;
    virtual void close() = 0;
    virtual bool haserror() = 0;
    // virtual void print_results(const char* sep = " ", const char* end = "\n");
    virtual ~DataSource() {};
};
// TODO: replace with super class
//typedef DataSource* (*create_server_t)(Context* cxt);
typedef void* (*create_server_t)(Context* cxt);
void* CreateNULLServer(Context*);
#endif //__DATASOURCE_CONN_H__