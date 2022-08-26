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
    ThreadPool(uint32_t n_threads = 0);
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

class Trigger{
private:
    void* triggers;  //min-heap by t-rem
    virtual void tick() = 0;

public:
    Trigger(ThreadPool* tp);
};

class IntervalBasedTrigger : public Trigger{
public:
    struct timer{
        uint32_t interval; // in milliseconds
        uint32_t time_remaining;
    };
    void add_trigger();
private:
    void tick() override;
};

class CallbackBasedTrigger : public Trigger{
public:
    void add_trigger();
private:
    void tick() override;
};

#endif
