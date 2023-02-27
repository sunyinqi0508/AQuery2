#include "pch_msc.hpp"

#include "io.h"
#include <limits>

#include <chrono>
#include <ctime>

#include "utils.h"
#include "libaquery.h"
#include <random>
#include "gc.h"

char* gbuf = nullptr;

void setgbuf(char* buf) {
	static char* b = nullptr;
	if (buf == nullptr)
		gbuf = b;
	else {
		gbuf = buf;
		b = buf;
	}
}

#ifdef __AQ__HAS__INT128__

template <>
void print<__int128_t>(const __int128_t& v, const char* delimiter){
	char s[41];
	s[40] = 0;
	std::cout<< get_int128str(v, s+40)<< delimiter;
}

template <>
void print<__uint128_t>(const __uint128_t&v, const char* delimiter){
	char s[41];
	s[40] = 0;
    std::cout<< get_uint128str(v, s+40) << delimiter;
}

std::ostream& operator<<(std::ostream& os, __int128 & v)
{
	print(v);
	return os;
}
std::ostream& operator<<(std::ostream& os, __uint128_t & v)
{
	print(v);
	return os;
}

#endif

template <>
void print<bool>(const bool&v, const char* delimiter){
	std::cout<< (v?"true":"false") << delimiter;
}


void skip(const char*& buf){ 
	while(*buf && (*buf >'9' || *buf < '0')) buf++; 
}

namespace types {

	date_t::date_t(const char* str) { fromString(str); }
	date_t& date_t::fromString(const char* str)  {
		if(str) {
			skip(str);
			year = getInt<short>(str);
			skip(str);
			month = getInt<unsigned char>(str);
			skip(str);
			day = getInt<unsigned char>(str);
		}
		else{
			day = month = year = 0;
		}
		return *this;
	}
	bool date_t::validate() const{
		return year <= 9999 && month <= 12 && day <= 31;
	}
	
	char* date_t::toString(char* buf) const {
		// if (!validate()) return "(invalid date)";
		*--buf = 0;
		buf = intToString(day, buf);
		*--buf = '-';
		buf = intToString(month, buf);
		*--buf = '-';
		buf = intToString(year, buf);
		return buf;
	}
	bool date_t::operator > (const date_t& other) const {
		return year > other.year || (year == other.year && (month > other.month || (month == other.month && day > other.day)));
	}

	bool date_t::operator < (const date_t& other) const {
		return year < other.year || (year == other.year && (month < other.month || (month == other.month && day < other.day)));
	}

	bool date_t::operator >= (const date_t& other) const {
		return year >= other.year || (year == other.year && (month >= other.month || (month == other.month && day >= other.day)));
	}

	bool date_t::operator <= (const date_t& other) const {
		return year <= other.year || (year == other.year && (month <= other.month || (month == other.month && day <= other.day)));
	}

	bool date_t::operator == (const date_t& other) const {
		return year == other.year && month == other.month && day == other.day;
	}

	bool date_t::operator != (const date_t& other) const {
		return !operator==(other);
	}

	time_t::time_t(const char* str) { fromString(str); }
	time_t& time_t::fromString(const char* str)  {
		if(str) {
			skip(str);
			hours = getInt<unsigned char>(str);
			skip(str);
			minutes = getInt<unsigned char>(str);
			skip(str);
			seconds = getInt<unsigned char>(str);
			skip(str);
			ms = getInt<unsigned int> (str);
		}
		else {
			hours = minutes = seconds = ms = 0;
		}
		return *this;
	}
	
	char* time_t::toString(char* buf) const {
		// if (!validate()) return "(invalid date)";
		*--buf = 0;
		buf = intToString(ms, buf);
		*--buf = ':';
		buf = intToString(seconds, buf);
		*--buf = ':';
		buf = intToString(minutes, buf);
		*--buf = ':';
		buf = intToString(hours, buf);
		return buf;
	}
	bool time_t::operator > (const time_t& other) const {
		return hours > other.hours || (hours == other.hours && (minutes > other.minutes || (minutes == other.minutes && (seconds > other.seconds || (seconds == other.seconds && ms > other.ms)))));
	}
	bool time_t::operator< (const time_t& other) const {
		return hours < other.hours || (hours == other.hours && (minutes < other.minutes || (minutes == other.minutes && (seconds < other.seconds || (seconds == other.seconds && ms < other.ms)))));
	} 
	bool time_t::operator>= (const time_t& other) const {
		return hours >= other.hours || (hours == other.hours && (minutes >= other.minutes || (minutes == other.minutes && (seconds >= other.seconds || (seconds == other.seconds && ms >= other.ms)))));
	}
	bool time_t::operator<= (const time_t& other) const{
		return hours <= other.hours || (hours == other.hours && (minutes <= other.minutes || (minutes == other.minutes && (seconds <= other.seconds || (seconds == other.seconds && ms <= other.ms)))));
	}
	bool time_t::operator==(const time_t& other) const {
		return hours == other.hours && minutes == other.minutes && seconds == other.seconds && ms == other.ms;
	}
	bool time_t::operator!= (const time_t& other) const {
		return !operator==(other);
	}
	bool time_t::validate() const{
		return hours < 24 && minutes < 60 && seconds < 60 && ms < 1000000;
	}

	timestamp_t::timestamp_t(const char* str) { fromString(str); }
	timestamp_t& timestamp_t::fromString(const char* str) {
		date.fromString(str);
		time.fromString(str);

		return *this;
	}
	bool timestamp_t::validate() const {
		return date.validate() && time.validate();
	}
	
	char* timestamp_t::toString(char* buf) const {
		buf = time.toString(buf);
		auto ret = date.toString(buf);
		*(buf-1) = ' ';
		return ret;
	}
	bool timestamp_t::operator > (const timestamp_t& other) const {
		return date > other.date || (date == other.date && time > other.time);
	}
	bool timestamp_t::operator < (const timestamp_t& other) const {
		return date < other.date || (date == other.date && time < other.time);
	}
	bool timestamp_t::operator >= (const timestamp_t& other) const {
		return date >= other.date || (date == other.date && time >= other.time);
	}
	bool timestamp_t::operator <= (const timestamp_t& other) const {
		return date <= other.date || (date == other.date && time <= other.time);
	}
	bool timestamp_t::operator == (const timestamp_t& other) const {
		return date == other.date && time == other.time;
	}
	bool timestamp_t::operator!= (const timestamp_t & other) const {
		return !operator==(other);
	}
}

template<class T>
void print_datetime(const T&v){
	char buf[T::string_length()];
	std::cout<<v.toString(buf + T::string_length());
}
std::ostream& operator<<(std::ostream& os, types::date_t & v)
{
	print_datetime(v);
	return os;
}
std::ostream& operator<<(std::ostream& os, types::time_t & v)
{
	print_datetime(v);
	return os;
}
std::ostream& operator<<(std::ostream& os, types::timestamp_t & v)
{
	print_datetime(v);
	return os;
}
std::ostream& operator<<(std::ostream& os, int8_t & v)
{
	os<<static_cast<int>(v);
	return os;
}
std::ostream& operator<<(std::ostream& os, uint8_t & v)
{
	os<<static_cast<unsigned>(v);
	return os;
}

std::string base62uuid(int l) {
    using namespace std;
    constexpr static const char* base62alp = 
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static mt19937_64 engine(
		std::chrono::system_clock::now().time_since_epoch().count());
    static uniform_int_distribution<uint64_t> u(0x10000, 0xfffff);
    uint64_t uuid = (u(engine) << 32ull) + 
		(std::chrono::system_clock::now().time_since_epoch().count() & 0xffffffff);
    
	string ret;
    while (uuid && l-- >= 0) {
        ret = string("") + base62alp[uuid % 62] + ret;
        uuid /= 62;
    }
    return ret;
}

template <>
inline const char* str(const bool& v) {
	return v ? "true" : "false";
}

class A {
	public:
	std::chrono::high_resolution_clock::time_point tp;
	A(){
		tp = std::chrono::high_resolution_clock::now();
		printf("A %llu created.\n", tp.time_since_epoch().count());
	}
	~A() {
		printf("A %llu died after %lldns.\n", tp.time_since_epoch().count(),
		 (std::chrono::high_resolution_clock::now() - tp).count());
	}
};

Context::Context() {
    current.memory_map = new std::unordered_map<void*, deallocator_t>;
#ifndef __AQ_USE_THREADEDGC__
	this->gc = new GC();
#endif
	GC::gc_handle->reg(new A(), 6553600, [](void* a){
		puts("deleting");
		delete ((A*)a);
	});
	init_session();
}

Context::~Context() {
    auto memmap = (std::unordered_map<void*, deallocator_t>*) this->current.memory_map;
    delete memmap;
}

void Context::init_session(){
    if (log_level == LOG_INFO){
        memset(&(this->current.stats), 0, sizeof(Session::Statistic));
    }
    auto memmap = (std::unordered_map<void*, deallocator_t>*) this->current.memory_map;
    memmap->clear();
}

void Context::end_session(){
    auto memmap = (std::unordered_map<void*, deallocator_t>*) this->current.memory_map;
    for (auto& mem : *memmap) {
        mem.second(mem.first);
    }
    memmap->clear();
}

void* Context::get_module_function(const char* fname){
    auto fmap = static_cast<std::unordered_map<std::string, void*>*>
        (this->module_function_maps);
    // printf("%p\n", fmap->find("mydiv")->second);
    //  for (const auto& [key, value] : *fmap){
    //      printf("%s %p\n", key.c_str(), value);
    //  }
    auto ret = fmap->find(fname);
    return ret == fmap->end() ? nullptr : ret->second;
}

// template<typename _Ty>
// inline void vector_type<_Ty>::out(uint32_t n, const char* sep) const
// {
// 	n = n > size ? size : n;
// 	std::cout << '(';
// 	{	
// 		uint32_t i = 0;
// 		for (; i < n - 1; ++i)
// 			std::cout << this->operator[](i) << sep;
// 		std::cout << this->operator[](i);
// 	}
// 	std::cout << ')';
// }

#include <utility>
#include <thread>
#ifndef __AQ_USE_THREADEDGC__

struct gcmemory_t{
	void* memory;
	void (*deallocator)(void*);
};

using memoryqueue_t = gcmemory_t*;
void GC::acquire_lock() {
	// auto this_tid = std::this_thread::get_id();
	// while(lock != this_tid)
	// {
	// 	while(lock != this_tid && lock != std::thread::id()) {
	// 		std::this_thread::sleep_for(std::chrono::milliseconds(0));
	// 	}
	// 	lock = this_tid;
	// }
}

void GC::release_lock(){
	// lock = std::thread::id();
}

void GC::gc()
{
	auto _q = static_cast<memoryqueue_t>(q);
	auto _q_back = static_cast<memoryqueue_t>(q_back);
	if (slot_pos == 0)
		return;
	auto t = _q;
	lock = true;
	while(alive_cnt != 0);
	q = _q_back;
	uint32_t _slot = slot_pos;
	slot_pos = 0;
	current_size = 0;
	lock = false;
	q_back = t;

	for(uint32_t i = 0; i < _slot; ++i){
		if (_q[i].memory != nullptr && _q[i].deallocator != nullptr)
			_q[i].deallocator(_q[i].memory);
	}
	memset(_q, 0, sizeof(gcmemory_t) * _slot);
	running = false;
}

void GC::daemon() {
	using namespace std::chrono;

	while (alive) {
		if (running) {
			if (uint64_t(current_size) > max_size || 
				forceclean_timer > forced_clean) 
			{
				gc();
				forceclean_timer = 0;
			}
			std::this_thread::sleep_for(microseconds(interval));
			forceclean_timer += interval;
		}
		else {
			std::this_thread::sleep_for(10ms);
			forceclean_timer += 10000;
		}
	}
}

void GC::start_deamon() {
	q = new gcmemory_t[max_slots << 1];
	q_back = new memoryqueue_t[max_slots << 1];
	lock = false;
	slot_pos = 0;
	current_size = 0;
	alive_cnt = 0;
	alive = true;
	handle = new std::thread(&GC::daemon, this);
}

void GC::terminate_daemon() {
	running = false;
	alive = false;
	decltype(auto) _handle = static_cast<std::thread*>(handle);
	delete[] static_cast<memoryqueue_t>(q);
	delete[] static_cast<memoryqueue_t>(q_back);
	using namespace std::chrono;
	std::this_thread::sleep_for(microseconds(1000 + std::max(static_cast<size_t>(10000), interval)));

	if (_handle->joinable()) {
		_handle->join();
	}
	delete _handle;
}

void GC::reg(void* v, uint32_t sz, void(*f)(void*)) { //~ 40ns expected v. free ~ 75ns
	if (v == nullptr || f == nullptr)
		return;
	if (sz < threshould){
		f(v);
		return;
	}
	else if (sz == 0xffffffff)
		sz = this->threshould;
	
	auto _q = static_cast<memoryqueue_t>(q);
	while(lock);
	++alive_cnt;
	current_size += sz;
	auto _slot = (slot_pos += 1);
	_q[_slot-1] = {v, f};
	--alive_cnt;
	running = true;
}

#endif

inline GC* GC::gc_handle = nullptr;
inline ScratchSpace* GC::scratch_space = nullptr;

void ScratchSpace::init(size_t initial_capacity) {
	ret = nullptr;
	scratchspace = static_cast<char*>(malloc(initial_capacity));
	ptr = cnt = 0;
	capacity = initial_capacity;
	this->initial_capacity = initial_capacity;
	temp_memory_fractions = new vector_type<void*>();
}

inline void* ScratchSpace::alloc(uint32_t sz){
    ptr = this->cnt;
	this->cnt += sz; // major cost
	if (this->cnt > capacity) {
		[[unlikely]] 
		capacity = this->cnt + (capacity >> 1);
		auto vec_tmpmem_fractions = static_cast<vector_type<char *>*>(temp_memory_fractions);
		vec_tmpmem_fractions->emplace_back(scratchspace);
		scratchspace = static_cast<char*>(malloc(capacity));
		ptr = 0;
	}
	return scratchspace + ptr;
}

inline void ScratchSpace::register_ret(void* ret){
	this->ret = ret;
}

inline void ScratchSpace::release(){
	ptr = cnt = 0;
	auto vec_tmpmem_fractions = 
		static_cast<vector_type<void*>*>(temp_memory_fractions);
	if (vec_tmpmem_fractions->size) {
    	[[unlikely]]
		for(auto& mem : *vec_tmpmem_fractions){
			//free(mem);
			GC::gc_handle->reg(mem);
		}
		vec_tmpmem_fractions->clear();
	}
}

inline void ScratchSpace::reset() {
	this->release();
	ret = nullptr;
	if (capacity != initial_capacity){
		capacity = initial_capacity;
		scratchspace = static_cast<char*>(realloc(scratchspace, capacity));
	}
}

void ScratchSpace::cleanup(){
	auto vec_tmpmem_fractions = 
		static_cast<vector_type<void*>*>(temp_memory_fractions);
	if (vec_tmpmem_fractions->size) {
		for(auto& mem : *vec_tmpmem_fractions){
			//free(mem); 
			GC::gc_handle->reg(mem);
		}
		vec_tmpmem_fractions->clear();
	}
	delete vec_tmpmem_fractions;
	free(this->scratchspace);
}


#include "dragonbox/dragonbox_to_chars.hpp" 


template<>
char*
aq_to_chars<float>(void* value, char* buffer) { 
    return jkj::dragonbox::to_chars_n(*static_cast<float*>(value), buffer);
}
template<>
char*
aq_to_chars<double>(void* value, char* buffer) { 
    return jkj::dragonbox::to_chars_n(*static_cast<double*>(value), buffer);
}

template<>
inline char*
aq_to_chars<bool>(void* value, char* buffer) {
	if (*static_cast<bool*>(value)){
		memcpy(buffer, "true", 4);
		return buffer + 4;
	}
	else{
		memcpy(buffer, "false", 5);
		return buffer + 5;
	}
}

template<>
char*
aq_to_chars<char*>(void* value, char* buffer) {
	const auto src = *static_cast<char**>(value);
	const auto len = strlen(src);
	memcpy(buffer, src, len);
	return buffer + len;
}

template<>
char*
aq_to_chars<types::date_t>(void* value, char* buffer) {
	const auto& src = *static_cast<types::date_t*>(value);
	buffer = to_text(buffer, src.year);
	*buffer++ = '-';
	buffer = to_text(buffer, src.month);
	*buffer++ = '-';
	buffer = to_text(buffer, src.day);
	return buffer;
}

template<>
char*
aq_to_chars<types::time_t>(void* value, char* buffer) {
	const auto& src = *static_cast<types::time_t*>(value);
	buffer = to_text(buffer, src.hours);
	*buffer++ = ':';
	buffer = to_text(buffer, src.minutes);
	*buffer++ = ':';
	buffer = to_text(buffer, src.seconds);
	*buffer++ = ':';
	buffer = to_text(buffer, src.ms);
	return buffer;
}

template<>
char*
aq_to_chars<types::timestamp_t>(void* value, char* buffer) {
	auto& src = *static_cast<types::timestamp_t*>(value);
	buffer = aq_to_chars<types::date_t>(static_cast<void*>(&src.date), buffer);
	*buffer++ = ' ';
	buffer = aq_to_chars<types::time_t>(static_cast<void*>(&src.time), buffer);
	return buffer;
}

template<> 
char* 
aq_to_chars<std::string_view>(void* value, char* buffer){
	const auto& src = *static_cast<std::string_view*>(value);
	memcpy(buffer, src.data(), src.size());
	return buffer + src.size();
}

// Defined in vector_type.h
template <>
vector_type<std::string_view>::vector_type(const char** container, uint32_t len, 
		typename std::enable_if_t<true>*) noexcept
{
	size = capacity = len;
	this->container = static_cast<std::string_view*>(
		malloc(sizeof(std::string_view) * len));
	for(uint32_t i = 0; i < len; ++i){
		this->container[i] = container[i];
	}
}

template<>
vector_type<std::string_view>::vector_type(const uint32_t size, void* data) : 
	size(size), capacity(0) {
	this->container = static_cast<std::string_view*>(
		malloc(sizeof(std::string_view) * size));
	for(uint32_t i = 0; i < size; ++i){
		this->container[i] = ((const char**)data)[i];
	}
	//std::cout<<size << container[1];
}

void activate_callback_based_trigger(Context* context, const char* cmd)
{
	const char* query_name = cmd + 2;
	const char* action_name = query_name;
	while (*action_name++);
	if(auto q = get_procedure(cxt, query_name), 
			a = get_procedure(cxt, action_name); 
			q.name == nullptr || a.name == nullptr
	)
		printf("Warning: Invalid query or action name: %s %s", 
			query_name, action_name);
	else {
		auto query = AQ_DupObject(&q);
		auto action = AQ_DupObject(&a);

		cxt->ct_host->execute_trigger(query, action);
	}
}
