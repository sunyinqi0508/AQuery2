#include "mariadb_conn.h"
#include "libaquery.h"
#include <cstdio>

inline size_t my_strlen(const char* str){
    size_t ret = 0;
    if(str)
        while(*str) ++str;
    return ret;
}

void MariadbServer::connect(
    Context* cxt, const char* host, const char* user, const char* passwd, 
        const char* db_name, const unsigned int port, 
        const char* unix_socket, const unsigned long client_flag
){
    this->cxt = cxt;
    cxt->alt_server = this;

    server = 0;
    mysql_init(server);
    if (server == 0){
        printf("Error initializing client.\n");
        return;
    }
    
    auto ret = 
        mysql_real_connect(server, host, user, passwd, db_name, port, unix_socket, client_flag);
    
    if(ret){
        printf("Error connecting to server: %d, %s\n", ret, mysql_error(server));
        return;
    }

    this->status = true;
}

void MariadbServer::exec(const char*q){
    auto res = mysql_real_query(server, q, my_strlen(q));
    if(res) printf("Execution Error: %d, %s\n", res, mysql_error(server)); 
}

void MariadbServer::close(){
    if(this->status && this->server){
        mysql_close(server);
        server = 0;
        this->status = false;
    }
}

Server::~Server(){
    if(this->status && this->server){
        this->close();
        mysql_library_end();
    }
}