#include "monetdb_conn.h"
#include "libaquery.h"
#include <cstdio>

void Server::connect(Context *cxt){
    if (!cxt){
        cxt->alt_server = this;
        this->cxt = cxt;
    }
    else{
        return;
    }

    if (server != 0){
        monetdbe_close(*server);
        free(*server);
        server = (monetdbe_database*)malloc(sizeof(monetdbe_database));
    }
    monetdbe_open(server, nullptr, nullptr);
}

void Server::exec(const char* q){
    auto qstatus = monetdbe_query(*server, const_cast<char*>(q), &res, &cnt);
    if (qstatus != nullptr){
        puts("Execution Failed.\n");
    }
}

void Server::close(){
    if(this->server){
        monetdbe_close(*(this->server));
        free(this->server);
        this->server = 0;
    }
}

void* Server::getCol(int col_idx){
    if(res){
        if(monetdbe_result_fetch(res, &ret_col, col_idx) != nullptr)
        {
            cnt = ret_col->count;
            return ret_col->data;
        }
    }
    return 0;
}

Server::~Server(){
    close();
}