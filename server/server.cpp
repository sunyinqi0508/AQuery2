#include "pch_msc.hpp"

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "libaquery.h"
#include "monetdb_conn.h"
#pragma region misc
#ifdef THREADING
#include "threading.h"
#endif

#ifdef _WIN32
#include "winhelper.h"
#else 
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <atomic>

// fast numeric to string conversion
#include "jeaiii_to_text.h"
#include "dragonbox/dragonbox_to_chars.h"

struct SharedMemory
{
    std::atomic<bool> a;
    int hFileMap;
    void* pData;
    explicit SharedMemory(const char* fname) {
        hFileMap = open(fname, O_RDWR, 0);
        if (hFileMap != -1)
            pData = mmap(nullptr, 8, PROT_READ | PROT_WRITE, MAP_SHARED, hFileMap, 0);
        else 
            pData = nullptr;
    }
    void FreeMemoryMap() const {
        // automatically unmapped in posix
    }
};

#ifndef __USE_STD_SEMAPHORE__
#ifdef __APPLE__
#include <dispatch/dispatch.h>
class A_Semaphore {
private:
	dispatch_semaphore_t native_handle;
public:
	A_Semaphore(bool v = false) {
		native_handle = dispatch_semaphore_create(v);
	}
	void acquire() {
        // puts("acquire");
		dispatch_semaphore_wait(native_handle, DISPATCH_TIME_FOREVER);
	}
	void release() {
        // puts("release");
		dispatch_semaphore_signal(native_handle);
	}
	~A_Semaphore() {
	}
};
#else
#include <semaphore.h>
class A_Semaphore {
private:
	sem_t native_handle;
public:
	A_Semaphore(bool v = false) {
		sem_init(&native_handle, v, 1);
	}
	void acquire() {
		sem_wait(&native_handle);
	}
	void release() {
		sem_post(&native_handle);
	}
	~A_Semaphore() {
		sem_destroy(&native_handle);
	}
};
#endif
#endif
#endif

#ifdef __USE_STD_SEMAPHORE__
#define __AQUERY_ITC_USE_SEMPH__
#include <semaphore>
class A_Semaphore {
private:
    std::binary_semaphore native_handle;
public:
    A_Semaphore(bool v = false) {
        native_handle = std::binary_semaphore(v);
    }
    void acquire() {
        native_handle.acquire();
    }
    void release() {
        native_handle.release();
    }
    ~A_Semaphore() { }
};
#endif

#ifdef __AQUERY_ITC_USE_SEMPH__
A_Semaphore prompt{ true }, engine{ false };
#define PROMPT_ACQUIRE() prompt.acquire()
#define PROMPT_RELEASE() prompt.release()
#define ENGINE_ACQUIRE() engine.acquire()
#define ENGINE_RELEASE() engine.release()
#else
#define PROMPT_ACQUIRE() 
#define PROMPT_RELEASE() std::this_thread::sleep_for(std::chrono::nanoseconds(0))
#define ENGINE_ACQUIRE() 
#define ENGINE_RELEASE() 
#endif

typedef void (*module_init_fn)(Context*);

int n_recv = 0;
char** n_recvd = nullptr;

__AQEXPORT__(void) wait_engine(){
    PROMPT_ACQUIRE();
}

__AQEXPORT__(void) wake_engine(){
    ENGINE_RELEASE();
}

extern "C" void __DLLEXPORT__ receive_args(int argc, char**argv){
    n_recv = argc;
    n_recvd = argv;
}

enum BinaryInfo_t {
	MSVC, MSYS, GCC, CLANG, AppleClang
};

extern "C" int __DLLEXPORT__ binary_info() {
#if defined(_MSC_VER) && !defined (__llvm__)
	return MSVC;
#elif defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)
	return MSYS;
#elif defined(__clang__)
	return CLANG;
#elif defined(__GNUC__)
    return GCC;
#endif
}

__AQEXPORT__(bool) have_hge(){
#if defined(__MONETDB_CONN_H__)
    return Server::havehge();
#else
    return false;
#endif
}

using prt_fn_t = char* (*)(void*, char*);

// This function contains heap allocations, free after use
template<class String_T>
char* to_lpstr(const String_T& str){
    auto ret = static_cast<char*>(malloc(str.size() + 1));
    memcpy(ret, str.c_str(), str.size());
    ret[str.size()] = '\0';
    return ret;
}
char* copy_lpstr(const char* str){
    auto len = strlen(str);
    auto ret = static_cast<char*>(malloc(len + 1));
    memcpy(ret, str, len + 1);
    return ret;
}

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

#include "monetdbe.h"
#undef max
#undef min
inline constexpr static unsigned char monetdbe_type_szs[] = {
    sizeof(monetdbe_column_bool::null_value), sizeof(monetdbe_column_int8_t::null_value), 
    sizeof(monetdbe_column_int16_t::null_value), sizeof(monetdbe_column_int32_t::null_value), 
    sizeof(monetdbe_column_int64_t::null_value),
#ifdef __SIZEOF_INT128__
    sizeof(monetdbe_column_int128_t::null_value),
#endif
    sizeof(monetdbe_column_size_t::null_value), sizeof(monetdbe_column_float::null_value),
    sizeof(monetdbe_column_double::null_value),
    sizeof(monetdbe_column_str::null_value), sizeof(monetdbe_column_blob::null_value),
    sizeof(monetdbe_data_date), sizeof(monetdbe_data_time), sizeof(monetdbe_data_timestamp),

    // should be last:
    1
};
constexpr uint32_t output_buffer_size = 65536;
void print_monetdb_results(Server* srv, const char* sep = " ", const char* end = "\n", 
    uint32_t limit = std::numeric_limits<uint32_t>::max()) {
    if (!srv->haserror() && srv->cnt && limit){
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
		if (header_string.size() - l_sep - 1>= 0)
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


void initialize_module(const char* module_name, void* module_handle, Context* cxt){
    auto _init_module = reinterpret_cast<module_init_fn>(dlsym(module_handle, "init_session"));
    if (_init_module) {
        _init_module(cxt);
    }
    else {
        printf("Warning: module %s have no session support.\n", module_name);
    }
}
#pragma endregion
int dll_main(int argc, char** argv, Context* cxt){
    aq_timer timer;
    Config *cfg = reinterpret_cast<Config *>(argv[0]);
    std::unordered_map<std::string, void*> user_module_map;
    std::string pwd = std::filesystem::current_path().c_str();
    auto sep = std::filesystem::path::preferred_separator;
    pwd += sep;
    std::string procedure_root = pwd + "procedures" + sep;
    std::string procedure_name = "";
    StoredProcedure current_procedure;
    vector_type<char *> recorded_queries;
    vector_type<void *> recorded_libraries;
    bool procedure_recording = false, 
         procedure_replaying = false;
    uint32_t procedure_module_cursor = 0;

    if (cxt->module_function_maps == nullptr)
        cxt->module_function_maps = new std::unordered_map<std::string, void*>();
    auto module_fn_map = 
        static_cast<std::unordered_map<std::string, void*>*>(cxt->module_function_maps);
    
    auto buf_szs = cfg->buffer_sizes;
    void** buffers = (void**) malloc (sizeof(void*) * cfg->n_buffers);
    for (int i = 0; i < cfg->n_buffers; i++) 
        buffers[i] = static_cast<void *>(argv[i + 1]);
    cxt->buffers = buffers;
    cxt->cfg = cfg;
    cxt->n_buffers = cfg->n_buffers;
    cxt->sz_bufs = buf_szs;
    if (cfg->backend_type == BACKEND_MonetDB && cxt->alt_server == nullptr)
    {
        auto alt_server = new Server(cxt);
        alt_server->exec("SELECT '**** WELCOME TO AQUERY++! ****';");
        puts(*(const char**)(alt_server->getCol(0)));
        cxt->alt_server = alt_server;
    }
    while(cfg->running){
        ENGINE_ACQUIRE();
        if (cfg->new_query) {
            cfg->stats.postproc_time = 0;
            cfg->stats.monet_time = 0;
start:

            void *handle = nullptr;
            void *user_module_handle = nullptr;
            if (cfg->backend_type == BACKEND_MonetDB){
                if (cxt->alt_server == nullptr)
                    cxt->alt_server = new Server(cxt);
                Server* server = reinterpret_cast<Server*>(cxt->alt_server);
                if(n_recv > 0){
                    if (cfg->backend_type == BACKEND_AQuery || cfg->has_dll) {
                        const char* proc_name = "./dll.so";
                        std::string dll_path;
                        if (procedure_recording) {
                            dll_path = procedure_root + 
                                procedure_name + std::to_string(recorded_libraries.size) + ".so";
                            
                            try{
                                if (std::filesystem::exists(dll_path))
                                    std::filesystem::remove(dll_path);
                                std::filesystem::copy_file(proc_name, dll_path);
                            } catch(std::filesystem::filesystem_error& e){
                                puts(e.what());
                                dll_path = proc_name;
                            }
                            proc_name = dll_path.c_str();
                            if(recorded_libraries.size)
                                recorded_queries.emplace_back(copy_lpstr("N"));
                        }
                        handle = dlopen(proc_name, RTLD_NOW);
                        if (procedure_recording) {
                            recorded_libraries.emplace_back(handle);
                        }
                    }
                    for (const auto& module : user_module_map){
                        initialize_module(module.first.c_str(), module.second, cxt);
                    }
                    cxt->init_session();
                    for(int i = 0; i < n_recv; ++i)
                    {
                        printf("%s, %d\n", n_recvd[i], n_recvd[i][0] == 'Q');
                        switch(n_recvd[i][0]){
                        case 'Q': // SQL query for monetdbe
                            {
                                if(procedure_recording){
                                    recorded_queries.emplace_back(copy_lpstr(n_recvd[i]));
                                }
                                timer.reset();
                                server->exec(n_recvd[i] + 1);
                                cfg->stats.monet_time += timer.elapsed();
                                printf("Exec Q%d: %s", i, n_recvd[i]);
                            }
                            break;
                        case 'P': // Postprocessing procedure 
                            if(handle && !server->haserror()) {
                                if (procedure_recording) {
                                    recorded_queries.emplace_back(copy_lpstr(n_recvd[i]));
                                }
                                code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, n_recvd[i]+1));
                                timer.reset();
                                c(cxt);
                                cfg->stats.postproc_time += timer.elapsed();
                            }
                            break;
                        case 'M': // Load Module
                            {
                                auto mname = n_recvd[i] + 1;
                                user_module_handle = dlopen(mname, RTLD_NOW);
                                //getlasterror
                                
                                if (!user_module_handle)
#ifndef _WIN32
                                    puts(dlerror());
#else
                                    printf("Fatal Error: Module %s failed to load with error code %d.\n", mname, dlerror());
#endif
                                user_module_map[mname] = user_module_handle;
                                initialize_module(mname, user_module_handle, cxt);
                            }
                            break;
                        case 'F': // Register Function in Module
                            {
                                auto fname = n_recvd[i] + 1;
                                //printf("F:: %s: %p, %p\n", fname, user_module_handle, dlsym(user_module_handle, fname));
                                module_fn_map->insert_or_assign(fname, dlsym(user_module_handle, fname));
                                //printf("F::: %p\n", module_fn_map->find("mydiv") != module_fn_map->end() ? module_fn_map->find("mydiv")->second : nullptr);
                            }
                            break;
                        case 'O':
                            {
                                if(!server->haserror()){
                                    if (procedure_recording){
                                        char* buf = (char*) malloc (sizeof(char) * 6);
                                        memcpy(buf, n_recvd[i], 5);
                                        buf[5] = '\0';
                                        recorded_queries.emplace_back(buf);
                                    }
                                    uint32_t limit;
                                    memcpy(&limit, n_recvd[i] + 1, sizeof(uint32_t));
                                    if (limit == 0)
                                        continue;
                                    timer.reset();
                                    print_monetdb_results(server, " ", "\n", limit);        
                                    cfg->stats.postproc_time += timer.elapsed();
                                }
                            }
                            break;
                        case 'U': // Unload Module
                            {
                                auto mname = n_recvd[i] + 1;
                                auto it = user_module_map.find(mname);
                                if (user_module_handle == it->second)
                                    user_module_handle = nullptr;
                                dlclose(it->second);
                                user_module_map.erase(it);
                            }
                            break;
                        case 'N':
                            {
                                if(procedure_module_cursor < current_procedure.postproc_modules)
                                    handle = current_procedure.__rt_loaded_modules[procedure_module_cursor++];
                            }
                            break;
                        case 'R': //recorded procedure
                            {
                                auto proc_name = n_recvd[i] + 2;
                                proc_name = *proc_name?proc_name : proc_name + 1;
                                puts(proc_name);
                                const auto& load_modules = [&](StoredProcedure &p) {
                                    if (!p.__rt_loaded_modules){
                                        p.__rt_loaded_modules = static_cast<void**>(
                                            malloc(sizeof(void*) * p.postproc_modules));
                                        for(uint32_t j = 0; j < p.postproc_modules; ++j){
                                            auto pj = dlopen(p.name, RTLD_NOW);
                                            if (pj == nullptr){
                                                printf("Error: failed to load module %s\n", p.name);
                                                return true;
                                            }
                                            p.__rt_loaded_modules[j] = pj;
                                        }
                                    }
                                    return false;
                                };
                                const auto& save_proc_tofile = [&](const StoredProcedure& p)  {
                                    auto config_name = procedure_root + procedure_name + ".aqp";
                                    auto fp = fopen(config_name.c_str(), "wb");
                                    if (fp == nullptr){
                                        printf("Error: failed to open file %s\n", config_name.c_str());
                                        return true;
                                    }
                                    fwrite(&p.cnt, sizeof(p.cnt), 1, fp);
                                    fwrite(&p.postproc_modules, sizeof(p.postproc_modules), 1, fp);
                                    for(uint32_t j = 0; j < p.cnt; ++j){
                                        auto current_query = p.queries[j];
                                        auto len_query = strlen(current_query);
                                        fwrite(current_query, len_query + 1, 1, fp);
                                    }
                                    fclose(fp);
                                    return false;
                                };
                                const auto& load_proc_fromfile = [&](StoredProcedure& p)  {
                                    auto config_name = procedure_root + p.name + ".aqp";
                                    auto fp = fopen(config_name.c_str(), "rb");
                                    if(fp == nullptr){
                                        puts("ERROR: Procedure not found on disk.");
                                        return false;
                                    }
                                    fread(&p.cnt, sizeof(p.cnt), 1, fp);
                                    fread(&p.postproc_modules, sizeof(p.postproc_modules), 1, fp);
                                    auto offset_now = ftell(fp);
                                    fseek(fp, 0, SEEK_END);
                                    auto queries_size = ftell(fp) - offset_now;
                                    fseek(fp, offset_now, SEEK_SET);

                                    p.queries = static_cast<char**>(malloc(sizeof(char*) * p.cnt));
                                    p.queries[0] = static_cast<char*>(malloc(sizeof(char) * queries_size));
                                    fread(&p.queries[0], queries_size, 1, fp);

                                    for(uint32_t j = 1; j < p.cnt; ++j){
                                        p.queries[j] = p.queries[j-1];
                                        while(*p.queries[j] != '\0')
                                            ++p.queries[j];
                                    }
                                    fclose(fp);
                                    return load_modules(p);
                                };
                                switch(n_recvd[i][1]){
                                    case '\0':
                                        current_procedure.name = copy_lpstr(proc_name);
                                        current_procedure.cnt = 0;
                                        current_procedure.queries = nullptr;
                                        current_procedure.postproc_modules = 0;
                                        current_procedure.__rt_loaded_modules = nullptr;
                                        procedure_recording = true;
                                        procedure_name = proc_name;
                                    break;
                                    case 'T':
                                        current_procedure.queries = recorded_queries.container;
                                        current_procedure.cnt = recorded_queries.size;
                                        current_procedure.name = copy_lpstr(proc_name);
                                        current_procedure.postproc_modules = recorded_libraries.size;
                                        current_procedure.__rt_loaded_modules = recorded_libraries.container;
                                        recorded_queries.size = recorded_queries.capacity = 0;
                                        recorded_queries.container = nullptr;
                                        recorded_libraries.size = recorded_libraries.capacity = 0;
                                        recorded_libraries.container = nullptr;
                                        procedure_recording = false;
                                        save_proc_tofile(current_procedure);
                                        cxt->stored_proc.insert_or_assign(procedure_name, current_procedure);
                                        procedure_name = "";
                                    break;
                                    case 'E': // execute procedure
                                    {
                                        auto _proc = cxt->stored_proc.find(proc_name);
                                        if (_proc == cxt->stored_proc.end()){
                                            printf("Procedure %s not found. Trying load from disk.\n", proc_name);
                                            if (load_proc_fromfile(current_procedure)){
                                                cxt->stored_proc.insert_or_assign(proc_name, current_procedure);
                                            }
                                        }
                                        else{
                                            current_procedure = _proc->second;
                                            n_recv = current_procedure.cnt;
                                            n_recvd = current_procedure.queries;
                                            load_modules(current_procedure);
                                            goto start; // yes, I know, refactor later!!
                                        }
                                    }
                                    break;
                                    case 'D': // delete procedure
                                    break;
                                    case 'S': //save procedure
                                    break;
                                    case 'L': //load procedure
                                    break;
                                    case 'd': // display all procedures
                                    for(const auto& p : cxt->stored_proc){
                                        printf("Procedure: %s, %d queries, %d modules:\n", p.first.c_str(), 
                                            p.second.cnt, p.second.postproc_modules);
                                        for(uint32_t j = 0; j < p.second.cnt; ++j){
                                            printf("\tQuery %d: %s\n", j, p.second.queries[j]);
                                        }
                                        puts("");
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    if(handle && procedure_replaying) {
                        dlclose(handle);
                        handle = nullptr;
                    }
                    printf("%lld, %lld", cfg->stats.monet_time, cfg->stats.postproc_time);
                    cxt->end_session();
                    n_recv = 0;
                }
                if(server->last_error == nullptr){
                    // TODO: Add feedback to prompt.
                }   
                else{
                    server->last_error = nullptr;
                    //goto finalize;
                } 
            }
            
            // puts(cfg->has_dll ? "true" : "false");
            if (cfg->backend_type == BACKEND_AQuery) {
                handle = dlopen("./dll.so", RTLD_NOW);
                code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dllmain"));
                c(cxt);
            }
            if (handle) dlclose(handle);
            cfg->new_query = 0;
        }
        //puts(cfg->running? "true": "false");
//finalize:
        PROMPT_RELEASE();
    }
    
    return 0;
}

int launcher(int argc, char** argv){
#ifdef _WIN32
    constexpr char sep = '\\';
#else
    constexpr char sep = '/';
#endif
    std::string str = " ";
    std::string pwd = "";
    if (argc > 0)
        pwd = argv[0];
    
    auto pos = pwd.find_last_of(sep);
    if (pos == std::string::npos)
        pos = 0;
    pwd = pwd.substr(0, pos);
    for (int i = 1; i < argc; i++){
        str += argv[i];
        str += " ";
    }
    str = std::string("cd ") + pwd + std::string("&& python3 ./prompt.py ") + str;
    return system(str.c_str());
}
#if !( defined(_MSC_VER) && defined(_DEBUG) )
extern "C" int __DLLEXPORT__ main(int argc, char** argv) {
#ifdef __AQ_BUILD_LAUNCHER__
   return launcher(argc, argv);
#endif
   // puts("running");
   Context* cxt = new Context();
   cxt->aquery_root_path = to_lpstr(std::filesystem::current_path().string());
   // cxt->log("%d %s\n", argc, argv[1]);

#ifdef THREADING
    auto tp = new ThreadPool();
    cxt->thread_pool = tp;
#endif
    
#ifdef __AQ_THREADED_GC__
    cxt->gc_thread = new std::thread(gc_thread, cxt);
#endif    
   const char* shmname;
   if (argc < 0)
        return dll_main(argc, argv, cxt);
   else
       shmname = argv[1];
   SharedMemory shm = SharedMemory(shmname);
   if (!shm.pData)
       return 1;
   bool &running = static_cast<bool*>(shm.pData)[0], 
       &ready = static_cast<bool*>(shm.pData)[1];
   using namespace std::chrono_literals;
   cxt->log("running: %s\n", running? "true":"false");
   cxt->log("ready: %s\n", ready? "true":"false");
   while (running) {
       std::this_thread::sleep_for(1ms);
       if(ready){
           cxt->log("running: %s\n", running? "true":"false");
           cxt->log("ready: %s\n", ready? "true":"false");
           void* handle = dlopen("./dll.so", RTLD_NOW);
           cxt->log("handle: %p\n", handle);
           if (handle) {
               cxt->log("inner\n");
               code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dllmain"));
               cxt->log("routine: %p\n", c);
               if (c) {
                   cxt->log("inner\n");
                   cxt->err("return: %d\n", c(cxt));
               }
           }
           ready = false;
       }
   }
   shm.FreeMemoryMap();
   return 0;
}

#endif // MSCDBG