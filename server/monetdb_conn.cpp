#include "pch_msc.hpp"

#include "libaquery.h"
#include <cstdio>
#include <string>
#include "monetdb_conn.h"
#include "monetdbe.h"
#include "table.h"
#undef ERROR
#undef static_assert

constexpr const char* monetdbe_type_str[] = {
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

inline constexpr static unsigned char monetdbe_type_szs[] = {
    sizeof(monetdbe_column_bool::null_value), sizeof(monetdbe_column_int8_t::null_value), 
    sizeof(monetdbe_column_int16_t::null_value), sizeof(monetdbe_column_int32_t::null_value), 
    sizeof(monetdbe_column_int64_t::null_value),
#ifdef HAVE_HGE
    sizeof(monetdbe_column_int128_t::null_value),
#endif
    sizeof(monetdbe_column_size_t::null_value), sizeof(monetdbe_column_float::null_value),
    sizeof(monetdbe_column_double::null_value),
    sizeof(monetdbe_column_str::null_value), sizeof(monetdbe_column_blob::null_value),
    sizeof(monetdbe_data_date), sizeof(monetdbe_data_time), sizeof(monetdbe_data_timestamp),

    // should be last:
    1
};

namespace types{
    constexpr const Type_t monetdbe_type_aqtypes[] = {
        ABOOL, AINT8, AINT16, AINT32, AINT64, 
#ifdef HAVE_HGE
        AINT128,
#endif
        AUINT64, AFLOAT, ADOUBLE, ASTR, 
        // blob?
        AINT64,
        ADATE, ATIME, ATIMESTAMP, ERROR

    };
}

Server::Server(Context* cxt){
    if (cxt){
        connect(cxt);
    } 
}

void Server::connect(Context *cxt){
    auto server = static_cast<monetdbe_database*>(this->server);
    if (cxt){
        cxt->alt_server = this;
        this->cxt = cxt;
    }
    else{
        puts("Error: Empty context.");
        return;
    }

    if (server){
        printf("Error: Server %p already connected. Restart? (Y/n). \n", server);
        char c[50];
        std::cin.getline(c, 49);
        for(int i = 0; i < 50; ++i){
            if (!c[i] || c[i] == 'y' || c[i] == 'Y'){
                monetdbe_close(*server);
                free(*server);
                this->server = 0;
                break;
            }
            else if(c[i]&&!(c[i] == ' ' || c[i] == '\t'))
                break;
        }
    }

    server = (monetdbe_database*)malloc(sizeof(monetdbe_database));
    auto ret = monetdbe_open(server, nullptr, nullptr);
    if (ret == 0){
        status = true;
        this->server = server;
    }
    else{
        if(server)
            free(server);
        this->server = nullptr;
        status = false;
        puts(ret == -1 ? "Allocation Error." : "Internal Database Error.");
    }
}

void Server::exec(const char* q){
    auto server = static_cast<monetdbe_database*>(this->server);
    auto _res = static_cast<monetdbe_result*>(this->res);
    monetdbe_cnt _cnt = 0;
    auto qresult = monetdbe_query(*server, const_cast<char*>(q), &_res, &_cnt);
    if (_res != 0){
        this->cnt = _res->nrows;
        this->res = _res;
    }
    if (qresult != nullptr){
        printf("Execution Failed. %s\n", qresult);
        last_error = qresult;
    }
}

bool Server::haserror(){
    if (last_error){
        last_error = nullptr;
        return true;
    }
    else{
        return false;
    }
}


void Server::print_results(const char* sep, const char* end){

    if (!haserror()){
        auto _res = static_cast<monetdbe_result*> (res);
        const auto& ncols = _res->ncols;
        monetdbe_column** cols = static_cast<monetdbe_column**>(malloc(sizeof(monetdbe_column*) * ncols));
        std::string* printf_string = new std::string[ncols];
        const char** col_data = static_cast<const char**> (malloc(sizeof(char*) * ncols));
        uint8_t* szs = static_cast<uint8_t*>(alloca(ncols));
        std::string header_string = "";
        const char* err_msg = nullptr;
        for(uint32_t i = 0; i < ncols; ++i){
            err_msg = monetdbe_result_fetch(_res, &cols[i], i);
            printf_string[i] = 
                std::string(types::printf_str[types::monetdbe_type_aqtypes[cols[i]->type]]) 
                + (i < ncols - 1 ? sep : "");
            puts(printf_string[i].c_str());
            puts(monetdbe_type_str[cols[i]->type]);
            col_data[i] = static_cast<char *>(cols[i]->data);
            szs [i] = monetdbe_type_szs[cols[i]->type];
            header_string = header_string + cols[i]->name + sep + '|' + sep;
        }
        const size_t l_sep = strlen(sep) + 1;
		if (header_string.size() - l_sep >= 0)
			header_string.resize(header_string.size() - l_sep);
        header_string += end + std::string(header_string.size(), '=') + end;
        fputs(header_string.c_str(), stdout);
        for(uint64_t i = 0; i < cnt; ++i){
            for(uint32_t j = 0; j < ncols; ++j){
                printf(printf_string[j].c_str(), *((void**)col_data[j]));
                col_data[j] += szs[j];
            }
            fputs(end, stdout);
        }
        free(cols);
        delete[] printf_string;
        free(col_data);
    }
}

void Server::close(){
    if(this->server){
        auto server = static_cast<monetdbe_database*>(this->server);
        monetdbe_close(*(server));
        free(server);
        this->server = nullptr;
    }
}

void* Server::getCol(int col_idx){
    if(res){
        auto _res = static_cast<monetdbe_result*>(this->res);
        auto err_msg = monetdbe_result_fetch(_res, 
            reinterpret_cast<monetdbe_column**>(&ret_col), col_idx);
        if(err_msg == nullptr)
        {
            auto _ret_col = static_cast<monetdbe_column*>(this->ret_col);
            cnt = _ret_col->count;
             printf("Dbg: Getting col %s, type: %s\n", 
                _ret_col->name, monetdbe_type_str[_ret_col->type]);
            return _ret_col->data;
        }
        else{
            printf("Error fetching result: %s\n", err_msg);
        }
    }
    else{
        puts("Error: No result.");
    }
    return nullptr;
}

Server::~Server(){
    close();
}

bool Server::havehge() {
#if defined(_MONETDBE_LIB_) and defined(HAVE_HGE)
    // puts("true");
    return HAVE_HGE;
#else
    // puts("false");
    return false;
#endif
}
