#ifdef _WIN32
#include <mysql.h>
#else
#include <mariadb/mysql.h>
#endif
struct Context;

struct Server{
    MYSQL *server = 0;
    Context *cxt = 0;
    bool status = 0;
    bool has_error = false;
    char* query = 0;
    int type = 0;
    
    void connect(Context* cxt, const char* host = "bill.local",
        const char* user = "root", const char* passwd = "0508", 
        const char* db_name = "db", const unsigned int port = 3306, 
        const char* unix_socket = 0, const unsigned long client_flag = 0
    );
    void exec(const char* q);
    void close();
    ~Server();
};