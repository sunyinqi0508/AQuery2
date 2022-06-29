#include "libaquery.h"
#include <cstdio>
#include "monetdb_conn.h"

const char* monetdbe_type_str[] = {
	"monetdbe_bool", "monetdbe_int8_t", "monetdbe_int16_t", "monetdbe_int32_t", "monetdbe_int64_t",
#ifdef HAVE_HGE
	"monetdbe_int128_t",
#endif
	"monetdbe_size_t", "monetdbe_float", "monetdbe_double",
	"monetdbe_str", "monetdbe_blob",
	"monetdbe_date", "monetdbe_time", "monetdbe_timestamp",

	// should be last:
	"monetdbe_type_unknown"
} ;

Server::Server(Context* cxt){
    if (cxt){
        connect(cxt);
    }
}

void Server::connect(Context *cxt){
    if (cxt){
        cxt->alt_server = this;
        this->cxt = cxt;
    }
    else{
        puts("Error: Empty context.");
        return;
    }

    if (server){
        printf("Error: Server %llx already connected. Restart? (Y/n). \n", server);
        char c[50];
        std::cin.getline(c, 49);
        for(int i = 0; i < 50; ++i){
            if (!c[i] || c[i] == 'y' || c[i] == 'Y'){
                monetdbe_close(*server);
                free(*server);
                server = 0;
                break;
            }
            else if(c[i]&&!(c[i] == ' ' || c[i] == '\t'))
                break;
        }
    }

    server = (monetdbe_database*)malloc(sizeof(monetdbe_database));
    auto ret = monetdbe_open(server, nullptr, nullptr);
}

void Server::exec(const char* q){
    auto qresult = monetdbe_query(*server, const_cast<char*>(q), &res, &cnt);
    if (res != 0)
        this->cnt = res->nrows;
    if (qresult != nullptr){
        printf("Execution Failed. %s\n", qresult);
        last_error = qresult;
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
        res->ncols;
        auto err_msg = monetdbe_result_fetch(res, &ret_col, col_idx);
        if(err_msg == nullptr)
        {
            cnt = ret_col->count;
            printf("Dbg: Getting col %s, type: %s\n", 
                ret_col->name, monetdbe_type_str[ret_col->type]);
            return ret_col->data;
        }
        else{
            printf("Error fetching result: %s\n", err_msg);
        }
    }
    else{
        puts("Error: No result.");
    }
    return 0;
}

Server::~Server(){
    close();
}