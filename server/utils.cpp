#include "utils.h"
#include <random>
#include <chrono>
using std::string;
string base62uuid(int l = 8) {
    using namespace std;
    constexpr static const char* base62alp = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static mt19937_64 engine(chrono::system_clock::now().time_since_epoch().count());
    static uniform_int_distribution<uint64_t> u(0x10000, 0xfffff);
    uint64_t uuid = (u(engine) << 32ull) + (chrono::system_clock::now().time_since_epoch().count() & 0xffffffff);
    printf("%llx\n", uuid);
    string ret;
    while (uuid && l-- >= 0) {
        ret = string("") + base62alp[uuid % 62] + ret;
        uuid /= 62;
    }
    return ret;
}