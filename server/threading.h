#ifndef _AQ_THREADING_H
#define _AQ_THREADING_H

#include <stdint.h>

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

#include <thread>
#include <mutex>
class A_Semphore;

class TriggerHost { 
protected:
    void* triggers;  
    std::thread* handle;
    ThreadPool *tp;
    Context* cxt;
    std::mutex* adding_trigger;

    virtual void tick() = 0;
public:
    TriggerHost() = default;
    virtual ~TriggerHost() = default;
};

struct StoredProcedure;

struct IntervalBasedTrigger {
    uint32_t interval; // in milliseconds
    uint32_t time_remaining;
    StoredProcedure* sp; 
    void reset();
    bool tick(uint32_t t); 
};

class IntervalBasedTriggerHost : public TriggerHost {
public:
    explicit IntervalBasedTriggerHost(ThreadPool *tp);
    void add_trigger(StoredProcedure* stored_procedure, uint32_t interval);
private:
    unsigned long long now;
    void tick() override;
};

class CallbackBasedTriggerHost : public TriggerHost {
public:
    void add_trigger();
private:
    void tick() override;
};

#endif
