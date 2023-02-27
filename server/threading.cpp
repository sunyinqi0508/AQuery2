#include "threading.h"
#include "libaquery.h"
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
            // A_TP_SET_PAYLOAD(tf[id]);
            current_payload[id]();
            //current_payload[id].empty();
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

    auto n_threads = (uint8_t) this->n_threads;
    for(; !this->terminate; this_thread::sleep_for(5ms)){
        if(*ticking) {
            size_t sz = pq->size();
            while(!pq->empty()){
                while(!pq->empty() && sz < (n_threads<<10)){
                    for(uint8_t i = 0; i < n_threads; ++i){
                        if(!A_TP_HAVE_PAYLOAD(tf[i])){
                            pq_lock->lock();
                            current_payload[i] = pq->front();
                            A_TP_SET_PAYLOAD(tf[i]);
                            pq->pop_front();
                            sz = pq->size();
                            pq_lock->unlock();
                            if (sz>=(n_threads<<10) || sz == 0) break;
                        }
                    }
                }
                pq_lock->lock();
                while(!pq->empty()){
                    for(uint8_t i = 0; i < n_threads; ++i){
                        if(!A_TP_HAVE_PAYLOAD(tf[i])){
                            current_payload[i] = pq->front();
                            A_TP_SET_PAYLOAD(tf[i]);
                            pq->pop_front();
                            if(pq->empty()) break;
                        }
                    }
                }
                pq_lock->unlock();
            }
            // puts("done");
            *ticking = false;
        }
    }

    for (uint8_t i = 0; i < n_threads; ++i)
        tf[i] &= 0xfd;
    for (uint8_t i = 0; i < n_threads; ++i)
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
    if (n_threads <= 0){
        n_threads = thread::hardware_concurrency();
        this->n_threads = n_threads;
    }
    printf("Thread pool started with %u threads;\n", n_threads);
    fflush(stdout);
    this->terminate = false;
    payload_queue = new deque<payload_t>;
    auto th = new thread[n_threads];
    auto tf = new atomic<uint8_t>[n_threads];
    
    thread_handles = th;
    thread_flags = tf;
    ticking = static_cast<void*>(new atomic<bool>(false));
    
    payload_queue_lock = new mutex();
    tick_handle = new thread(&ThreadPool::tick, this);
    current_payload = new payload_t[n_threads];

    for (uint32_t i = 0; i < n_threads; ++i){
        // atomic_init(tf + i, static_cast<unsigned char>(0b10));
        tf[i] = static_cast<unsigned char>(0b10);
        th[i] = thread(&ThreadPool::daemon_proc, this, i);
    }

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

IntervalBasedTriggerHost::IntervalBasedTriggerHost(ThreadPool* tp, Context* cxt){
    this->cxt = cxt;
    this->tp = tp;
    this->triggers = new aq_map<std::string, IntervalBasedTrigger>;
    trigger_queue_lock = new mutex();
    this->now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    this->running = true;
    this->handle = new thread(&IntervalBasedTriggerHost::tick, this);
}

void IntervalBasedTriggerHost::add_trigger(const char* name, StoredProcedure *p, uint32_t interval) {
    auto tr = IntervalBasedTrigger{.interval = interval, .time_remaining = 0, .sp = p};
    auto vt_triggers = static_cast<aq_map<std::string, IntervalBasedTrigger> *>(this->triggers);
    trigger_queue_lock->lock();
    vt_triggers->emplace(name, tr);
    //(*vt_triggers)[name] = tr;
    trigger_queue_lock->unlock();
}

void IntervalBasedTriggerHost::tick() {
    auto current_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto delta_t = static_cast<uint32_t>((current_time - now) / 1000000); // miliseconds precision
    now = current_time;
    auto vt_triggers = static_cast<aq_map<std::string, IntervalBasedTrigger> *>(this->triggers);
    fflush(stdout);
    while(this->running) {
        std::this_thread::sleep_for(50ms);
        current_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        delta_t = static_cast<uint32_t>((current_time - now) / 1000000); // miliseconds precision
        now = current_time;
        trigger_queue_lock->lock();
        for(auto& [_, t] : vt_triggers->values()) {
            if(t.tick(delta_t)) {
                payload_t payload;
                payload.f = execTriggerPayload;
                payload.args = static_cast<void*>(new StoredProcedurePayload {t.sp, cxt});

                tp->enqueue_task(payload);
            }
        }
        trigger_queue_lock->unlock();
    }
}

void IntervalBasedTriggerHost::remove_trigger(const char* name) {
    auto vt_triggers = static_cast<aq_map<std::string, IntervalBasedTrigger> *>(this->triggers);
    vt_triggers->erase(name);
}

void IntervalBasedTrigger::reset() {
    time_remaining = interval;
}

bool IntervalBasedTrigger::tick(uint32_t delta_t) {
    bool ret = false; 
    // printf("%d %d\n",delta_t, time_remaining);
    if (time_remaining <= delta_t) 
        ret = true; 
    if (auto curr_dt = delta_t % interval; time_remaining <= curr_dt) 
        time_remaining = interval  + time_remaining - curr_dt;
    else 
        time_remaining = time_remaining - curr_dt;
    return ret;
}

CallbackBasedTriggerHost::CallbackBasedTriggerHost(ThreadPool *tp, Context *cxt) {
    this->tp = tp;
    this->cxt = cxt;
    this->triggers = new aq_map<std::string, CallbackBasedTrigger>;
}

void CallbackBasedTriggerHost::execute_trigger(StoredProcedure* query, StoredProcedure* action) {
    payload_t payload;
    payload.f = execTriggerPayloadCond;
    payload.args = static_cast<void*>(
        new StoredProcedurePayloadCond {query, action, cxt}
    );

    this->tp->enqueue_task(payload);
}

void execute_trigger(const char* trigger_name) {
    auto vt_triggers = static_cast<aq_map<std::string, CallbackBasedTrigger> *>(this->triggers);
    auto ptr = vt_triggers->find(trigger_name);
    if (ptr != vt_triggers->end()) {
        auto& tr = ptr->second;
        if (!tr.materialized) {
            tr.query = new CallbackBasedTrigger(get_procedure(cxt, tr.query_name));
            tr.action = new CallbackBasedTrigger(get_procedure(cxt, tr.action_name));
            tr.materialized = true;
        }
        this->execute_trigger(AQ_DupObject(tr.query), AQ_DupObject(tr.action));
    }
}

void CallbackBasedTriggerHost::add_trigger(
    const char* trigger_name, 
    const char* table_name, 
    const char* query_name, 
    const char* action_name
) {
    auto vt_triggers = static_cast<aq_map<std::string, CallbackBasedTrigger> *>(this->triggers);
    auto tr = CallbackBasedTrigger {
        .trigger_name = trigger_name, 
        .table_name = table_name, 
        .query_name = query_name, 
        .action_name = action_name,
        .materialized = false
    };
    vt_triggers->emplace(trigger_name, tr);
}

void CallbackBasedTriggerHost::tick() {}
