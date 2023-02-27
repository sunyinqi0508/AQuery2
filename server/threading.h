#ifndef _AQ_THREADING_H
#define _AQ_THREADING_H

#include <stdint.h>
#include <thread>
#include <mutex>

typedef int(*payload_fn_t)(void*);
struct payload_t{
    payload_fn_t f;
    void* args;
    constexpr payload_t(payload_fn_t f, void* args) noexcept 
        : f(f), args(args) {}
    constexpr payload_t() noexcept
        : f(nullptr), args(nullptr) {};
    bool is_empty() const { return f && args; }
    void empty() { f = nullptr; args = nullptr; }
    void operator()() { f(args); }
};

class ThreadPool{

public:
    explicit ThreadPool(uint32_t n_threads = 0);
    void enqueue_task(const payload_t& payload);
    bool busy();
    virtual ~ThreadPool();

private:
    uint32_t n_threads;
    void* thread_handles;
    void* thread_flags;
    payload_t* current_payload;
    void* payload_queue;
    void* tick_handle;
    void* ticking;
    void* payload_queue_lock;
    bool terminate;
    void tick();
    void daemon_proc(uint32_t);

};


class A_Semphore;

class TriggerHost { 
public:
    void* triggers;  
    std::thread* handle;
    ThreadPool *tp;
    Context* cxt;
    std::mutex* trigger_queue_lock;

    virtual void tick() = 0;
    TriggerHost() = default;
    virtual ~TriggerHost() = default;
};

struct StoredProcedure;

struct Trigger{};

struct IntervalBasedTrigger : Trigger {
    uint32_t interval; // in milliseconds
    uint32_t time_remaining;
    StoredProcedure* sp; 
    void reset();
    bool tick(uint32_t t); 
};

class IntervalBasedTriggerHost : public TriggerHost {
public:
    explicit IntervalBasedTriggerHost(ThreadPool *tp, Context* cxt);
    void add_trigger(const char* name, StoredProcedure* stored_procedure, uint32_t interval);
    void remove_trigger(const char* name);
private:
    unsigned long long now;
    bool running;
    void tick() override;
};

struct CallbackBasedTrigger : Trigger {
    const char* trigger_name;
    const char* table_name;
    union {
        StoredProcedure* query;
        const char* query_name;
    };
    union {
        StoredProcedure* action;
        const char* action_name;
    };
    bool materialized;
};

class CallbackBasedTriggerHost : public TriggerHost {
public:
    explicit CallbackBasedTriggerHost(ThreadPool *tp, Context *cxt);
    void execute_trigger(StoredProcedure* query, StoredProcedure* action);
    void execute_trigger(const char* trigger_name);
    void add_trigger(const char* trigger_name, const char* table_name, 
        const char* query_name, const char* action_name);

private:
    void tick() override;
};

#endif
