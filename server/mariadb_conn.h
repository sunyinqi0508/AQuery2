#ifdef _WIN32
#include <mysql.h>
#else
#include <mariadb/mysql.h>
#endif
struct Context;

struct Server{
    MYSQL *server = nullptr;
    Context *cxt = nullptr;
    bool status = false;
    bool has_error = false;
    char* query = nullptr;
    int type = 0;
    
    void connect(Context* cxt, const char* host = "bill.local",
        const char* user = "root", const char* passwd = "0508", 
        const char* db_name = "db", const unsigned int port = 3306, 
        const char* unix_socket = nullptr, const unsigned long client_flag = 0
    );
    void exec(const char* q);
    void close();
    ~Server();
};