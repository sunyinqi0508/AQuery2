#include "pch_msc.hpp"

#include "libaquery.h"
#include <cstdio>
#include <string>
#include "monetdb_conn.h"
#include "monetdbe.h"

#include "table.h"
#include <thread>

#ifdef _WIN32
    #include "winhelper.h"
    #include "threading.h"
    #undef min
    #undef max
#else 
    #include <dlfcn.h>
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <atomic>
#endif // _WIN32

#ifdef ERROR
    #undef ERROR
#endif
#ifdef static_assert
    #undef static_assert
#endif

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
        for(int i = 0; i < 50; ++i) {
            if (!c[i] || c[i] == 'y' || c[i] == 'Y') {
                monetdbe_close(*server);
                free(*server);
                this->server = nullptr;
                break;
            }
            else if(c[i]&&!(c[i] == ' ' || c[i] == '\t'))
                break;
        }
    }

    server = (monetdbe_database*)malloc(sizeof(monetdbe_database));
    monetdbe_options ops;
    AQ_ZeroMemory(ops);
    ops.nr_threads = std::thread::hardware_concurrency();
    auto ret = monetdbe_open(server, nullptr, &ops);
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
    if (_res != nullptr){
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
        puts(last_error);
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
            if (err_msg) [[unlikely]]
                puts(err_msg);
        }
		if (const size_t l_sep = strlen(sep) + 1; header_string.size() >= l_sep)
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
        monetdbe_close(*server);
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

#define AQ_MONETDB_FETCH(X) case monetdbe_##X: \
    return (long long)((X *)(_ret_col->data))[0]; 
long long Server::getFirstElement() {
    if(!this->haserror() && res) {
        auto _res = static_cast<monetdbe_result*>(this->res);
        auto err_msg = monetdbe_result_fetch(_res, 
            reinterpret_cast<monetdbe_column**>(&ret_col), 0);
        if(err_msg == nullptr)
        { 
            auto _ret_col = static_cast<monetdbe_column*>(this->ret_col);
            cnt = _ret_col->count;
            if(cnt > 0) { 
                switch(_ret_col->type) {
                    AQ_MONETDB_FETCH(bool)
                    AQ_MONETDB_FETCH(int8_t)
                    AQ_MONETDB_FETCH(int16_t)
                    AQ_MONETDB_FETCH(int32_t)
                    AQ_MONETDB_FETCH(int64_t)
#ifdef HAVE_HGE
                case monetdbe_int128_t: 
                    return (long long)((__int128_t *)(_ret_col->data))[0]; 
#endif
                    AQ_MONETDB_FETCH(size_t)
                    AQ_MONETDB_FETCH(float)
                    AQ_MONETDB_FETCH(double)
                    case monetdbe_str:
                        return ((const char **)(_ret_col->data))[0][0] == '\0';
                    default:
                        printf("Error, non-primitive result: Getting col %s, type: %s\n", 
                    _ret_col->name, monetdbe_type_str[_ret_col->type]);
                        return 0;                    
                }
            }
        }
        else {
            printf("Error fetching result: %s\n", err_msg);
        }
    }
    else {
        puts("Error: No result.");
    }
    return 0;
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

using prt_fn_t = char* (*)(void*, char*);
constexpr prt_fn_t monetdbe_prtfns[] = {
	aq_to_chars<bool>, aq_to_chars<int8_t>, aq_to_chars<int16_t>, aq_to_chars<int32_t>, 
	aq_to_chars<int64_t>,
#if __SIZEOF_INT128__
	aq_to_chars<__int128_t>, 
#endif
	aq_to_chars<size_t>, aq_to_chars<float>, aq_to_chars<double>,
	aq_to_chars<char*>, aq_to_chars<std::nullptr_t>,
	aq_to_chars<types::date_t>, aq_to_chars<types::time_t>, aq_to_chars<types::timestamp_t>,

	// should be last:
	aq_to_chars<std::nullptr_t>
};


constexpr uint32_t output_buffer_size = 65536;
void print_monetdb_results(void* _srv, const char* sep = " ", const char* end = "\n", 
    uint32_t limit = std::numeric_limits<uint32_t>::max()) {
    auto srv = static_cast<Server *>(_srv);
    if (!srv->haserror() && srv->cnt && limit) {
        char buffer[output_buffer_size];
        auto _res = static_cast<monetdbe_result*> (srv->res);
        const auto ncols = _res->ncols;
        monetdbe_column** cols = static_cast<monetdbe_column**>(malloc(sizeof(monetdbe_column*) * ncols));
        prt_fn_t *prtfns = (prt_fn_t*) alloca(sizeof(prt_fn_t) * ncols);
        char** col_data = static_cast<char**> (alloca(sizeof(char*) * ncols));
        uint8_t* szs = static_cast<uint8_t*>(alloca(ncols));
        std::string header_string = "";
        const char* err_msg = nullptr;
        const size_t l_sep = strlen(sep);
        const size_t l_end = strlen(end);
        char* _buffer = buffer;
        const auto cnt = srv->cnt < limit? srv->cnt : limit;

        for(uint32_t i = 0; i < ncols; ++i){
            err_msg = monetdbe_result_fetch(_res, &cols[i], i);
            if(err_msg) { goto cleanup; }
            col_data[i] = static_cast<char *>(cols[i]->data);
            prtfns[i] = monetdbe_prtfns[cols[i]->type];
            szs [i] = monetdbe_type_szs[cols[i]->type];
            header_string = header_string + cols[i]->name + sep + '|' + sep;
        }

        if(l_sep > 512 || l_end > 512) {
            puts("Error: separator or end string too long");
            goto cleanup;
        }
		if (header_string.size() >= l_sep + 1)
			header_string.resize(header_string.size() - l_sep - 1);
        header_string += end + std::string(header_string.size(), '=') + end;
        fputs(header_string.c_str(), stdout);
        for(uint64_t i = 0; i < cnt; ++i){
            for(uint32_t j = 0; j < ncols; ++j){
                //copy the field to buf
                _buffer = prtfns[j](col_data[j], _buffer);
                if (j != ncols - 1){
                    memcpy(_buffer, sep, l_sep);
                    _buffer += l_sep;
                }
                col_data[j] += szs[j];
            }
            memcpy(_buffer, end, l_end);
            _buffer += l_end;
            if(output_buffer_size - (_buffer - buffer) <= 1024){
                fwrite(buffer, 1, _buffer - buffer, stdout);
                _buffer = buffer;
            }
        }
        memcpy(_buffer, end, l_end);
        _buffer += l_end;
        if (_buffer != buffer)
            fwrite(buffer, 1, _buffer - buffer, stdout);
cleanup:        
        free(cols);
    }
}


int ExecuteStoredProcedureEx(const StoredProcedure *p, Context* cxt){
    auto server = static_cast<Server*>(cxt->alt_server);
    int ret = 0;
    bool return_from_procedure = false;
    void* handle = nullptr;
    uint32_t procedure_module_cursor = 0;
    for(uint32_t i = 0; i < p->cnt; ++i) {
        switch(p->queries[i][0]){
            puts(p->queries[i]);
            case 'Q': {
                server->exec(p->queries[i] + 1);
                ret = int(server->getFirstElement());
            } 
            break;
            case 'P': {
                auto c = code_snippet(dlsym(handle, p->queries[i] + 1));
                ret = c(cxt);
            }
            break;
            case 'N': {
                if(procedure_module_cursor < p->postproc_modules)
                    handle = p->__rt_loaded_modules[procedure_module_cursor++];
            }
            break;
            case 'T' : {
                if (p->queries[i][1] == 'N') {
                    cxt->ct_host->execute_trigger(p->queries[i] + 2);
                }
            }
            break;
            case 'O': {
                uint32_t limit;
                memcpy(&limit, p->queries[i] + 1, sizeof(uint32_t));
                if (limit == 0)
                    continue;
                print_monetdb_results(server, " ", "\n", limit); 
            }
            break;
            default:
                printf("Warning Q%u: unrecognized command %c.\n", 
                    i, p->queries[i][0]);
        }
    }
    
    return ret;
}

int execTriggerPayload(void* args) {
    auto spp = (StoredProcedurePayload*)(args);
    puts("exec trigger");
    auto ret = ExecuteStoredProcedureEx(spp->p, spp->cxt);
    delete spp;
    return ret;
}

int execTriggerPayloadCond(void* args) {
    int ret = 0;
    auto spp = (StoredProcedurePayloadCond*)(args); 
    if(ExecuteStoredProcedureEx(spp->condition, spp->cxt) != 0) 
        ret = ExecuteStoredProcedureEx(spp->action, spp->cxt);
    free(spp->condition);
    free(spp->action);
    delete spp;
    return ret;
}
