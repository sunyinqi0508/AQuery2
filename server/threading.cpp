#include "threading.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>

using namespace std;
using namespace chrono_literals;

#define A_TP_HAVE_PAYLOAD(x) ((x) & 0b1)
#define A_TP_SET_PAYLOAD(x) ((x) |= 0b1)
#define A_TP_UNSET_PAYLOAD(x) ((x) &= 0xfe)
#define A_TP_IS_RUNNING(x) ((x) & 0b10)

void ThreadPool::daemon_proc(uint32_t id){
    auto tf = static_cast<atomic<uint8_t>*>(this->thread_flags);
    auto ticking = static_cast<atomic<bool>*>(this->ticking);

    for(; tf[id]; this_thread::sleep_for(*ticking? 0ns:100ms)) {
        if (A_TP_HAVE_PAYLOAD(tf[id])) {
            A_TP_SET_PAYLOAD(tf[id]);
            current_payload[id]();
            current_payload[id].empty();
            A_TP_UNSET_PAYLOAD(tf[id]);
        }
    }
}

void ThreadPool::tick(){
    auto pq_lock = static_cast<mutex*>(payload_queue_lock);
    auto pq = static_cast<deque<payload_t> *>(payload_queue);
    auto tf = static_cast<atomic<uint8_t>*>(this->thread_flags);
    auto ticking = static_cast<atomic<bool>*>(this->ticking);
    auto th = static_cast<thread*>(this->thread_handles);
    for(; !this->terminate; this_thread::sleep_for(50ms)){
        if(*ticking) {
            bool quit = false;
            for(; !quit; ){
                for(uint32_t i = 0; i < n_threads; ++i){
                    if(!A_TP_HAVE_PAYLOAD(tf[i])){
                        pq_lock->lock();
                        payload_t& p = pq->front();
                        current_payload[i] = p;
                        A_TP_SET_PAYLOAD(tf[i]);
                        pq->pop_front();
                        quit = !pq->size();
                        pq_lock->unlock();
                        if (quit) break;
                    }
                }
            }
            puts("done");
            *ticking = false;
        }
    }

    for (uint32_t i = 0; i < n_threads; ++i)
        tf[i] &= 0xfd;
    for (uint32_t i = 0; i < n_threads; ++i)
        th[i].join();

    delete[] th;
    delete[] tf;
    delete pq;
    delete pq_lock;
    delete ticking;
    auto cp = static_cast<payload_t*>(current_payload);
    delete[] cp;
}

ThreadPool::ThreadPool(uint32_t n_threads) 
    : n_threads(n_threads) {
    printf("Thread pool started with %u threads;", n_threads);
    fflush(stdout);
    this->terminate = false;
    payload_queue = new deque<payload_t>;
    auto th = new thread[n_threads];
    auto tf = new atomic<uint8_t>[n_threads];
    
    thread_handles = th;
    thread_flags = tf;
    ticking = static_cast<void*>(new atomic<bool>(false));
    
    for (uint32_t i = 0; i < n_threads; ++i){
        atomic_init(tf + i, 0b10);
        th[i] = thread(&ThreadPool::daemon_proc, this, i);
    }

    payload_queue_lock = new mutex();
    tick_handle = new thread(&ThreadPool::tick, this);
    current_payload = new payload_t[n_threads];
}


void ThreadPool::enqueue_task(const payload_t& payload){
    auto pq_lock = static_cast<mutex*>(payload_queue_lock);
    auto pq = static_cast<deque<payload_t> *>(payload_queue);
    auto tf = static_cast<atomic<uint8_t>*>(this->thread_flags);
    auto& ticking = *static_cast<atomic<bool>*>(this->ticking);

    if (!ticking){
        for (uint32_t i = 0; i < n_threads; ++i){
            if(!A_TP_HAVE_PAYLOAD(tf[i])){
                current_payload[i] = payload;
                A_TP_SET_PAYLOAD(tf[i]);
                return;
            }
        }
    }
    
    pq_lock->lock();
    pq->push_back(payload);
    ticking = true;
    pq_lock->unlock();        
}

ThreadPool::~ThreadPool() {
    this->terminate = true;
    auto tick = static_cast<thread*> (tick_handle);
    tick->join();
    delete tick;
    puts("Thread pool terminated.");
}

bool ThreadPool::busy(){
    if (!*(atomic<bool>*)ticking) {
        for (int i = 0; i < n_threads; ++i)
            if (A_TP_HAVE_PAYLOAD(((atomic<uint8_t>*)thread_flags)[i]))
                return true;
        return false;
    }
    return true;
}

