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
    decltype(auto) tfid = static_cast<atomic<uint8_t>*>(this->thread_flags)[id];
    auto ticking = static_cast<atomic<uint16_t>*>(this->ticking);
    auto& currentpayload = this->current_payload[id];
    auto pq_lock = static_cast<mutex*>(payload_queue_lock);
    auto pq = static_cast<deque<payload_t> *>(payload_queue);

    bool idle = true;
    uint32_t timer = 0;
    for(; tfid; ) {
        if (A_TP_HAVE_PAYLOAD(tfid)) {
            //A_TP_SET_PAYLOAD(tfid);
            currentpayload();
            // currentpayload.empty();
            A_TP_UNSET_PAYLOAD(tfid);
            if (idle) {
                idle = false;
                *ticking -= 1;
                timer = 1;
            }
        }
        
        else if (!pq->empty()) {
            pq_lock->lock();
            if (!pq->empty()) {
                pq->front()();
                pq->pop_front();
            }
            pq_lock->unlock();
            if (idle) {
                idle = false;
                *ticking -= 1;
                timer = 1;
            }
        }
        else if (!idle) {
            idle = true;
            *ticking += 1;
            timer = 1;
        }
        else if (*ticking == n_threads ) {
            if (timer > 1000000u)
                this_thread::sleep_for(chrono::nanoseconds(timer/100u));
            timer = timer > 4200000000u ? 4200000000u : timer*1.0000001 + 1;
        }

    }
}

void ThreadPool::tick(){
    auto pq_lock = static_cast<mutex*>(payload_queue_lock);
    auto pq = static_cast<deque<payload_t> *>(payload_queue);
    auto tf = static_cast<atomic<uint8_t>*>(this->thread_flags);
    auto ticking = static_cast<atomic<uint16_t>*>(this->ticking);
    auto th = static_cast<thread*>(this->thread_handles);
    //uint32_t threshold = (n_threads << 10u);
    for(; !this->terminate; this_thread::sleep_for(50ms)){
        // if(*ticking) {
        //     while(pq->size() > 0)
        //     {
        //         bool pqsize = false;
        //         for(; !pqsize; ){
        //             for(uint32_t i = 0; i < n_threads; ++i){
        //                 if(!A_TP_HAVE_PAYLOAD(tf[i])){
        //                     pq_lock->lock();
        //                     current_payload[i] = pq->front();
        //                     A_TP_SET_PAYLOAD(tf[i]);
        //                     pq->pop_front();
        //                     pqsize =! pq->size();
        //                     pq_lock->unlock();
        //                     if (pqsize) break;
        //                 }
        //             }
        //         }
        //     }
        //     // puts("done");
        //     *ticking = false;
        // }
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
        
    printf("Thread pool started with %u threads;\n", n_threads);
    fflush(stdout);
    this->terminate = false;
    payload_queue = new deque<payload_t>;
    auto th = new thread[n_threads];
    auto tf = new atomic<uint8_t>[n_threads];
    
    thread_handles = th;
    thread_flags = tf;
    ticking = static_cast<void*>(new atomic<uint16_t>(n_threads));

    payload_queue_lock = new mutex();
    tick_handle = new thread(&ThreadPool::tick, this);
    current_payload = new payload_t[n_threads];

    for (uint32_t i = 0; i < n_threads; ++i){
        atomic_init(tf + i, 0b10);
        th[i] = thread(&ThreadPool::daemon_proc, this, i);
    }

}


void ThreadPool::enqueue_task(const payload_t& payload){
    auto pq_lock = static_cast<mutex*>(payload_queue_lock);
    auto pq = static_cast<deque<payload_t> *>(payload_queue);
    auto tf = static_cast<atomic<uint8_t>*>(this->thread_flags);
    auto& ticking = *static_cast<atomic<uint16_t>*>(this->ticking);

    if (ticking > 0){
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
    return !(*(atomic<int16_t>*)ticking == n_threads);
}

