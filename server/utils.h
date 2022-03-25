#pragma once
#include <string>
#include <ctime>
#include <random>
using std::string;
string base62uuid(int l = 8) {
    constexpr static const char* base62alp = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static mt19937_64 engine(chrono::system_clock::now().time_since_epoch().count());
	static uniform_int_distribution<uint64_t> u(0x100000000, 0xfffffffff);
    uint64_t uuid = (u(engine)<<16ull) + (time(0)&0xffff);
    printf("%lx\n", uuid);
    string ret;
    while (uuid && l-- >= 0){
        ret = string("") + base62alp[uuid % 62] + ret;
        uuid /= 62;
    }
    return ret;
}