
// Copyright (c) 2022 James Edward Anhalt III - https://github.com/jeaiii/itoa
using u32 = decltype(0xffffffff);
using u64 = decltype(0xffffffffffffffff);

static_assert(u32(-1) > 0, "u32 must be unsigned");
static_assert(u32(0xffffffff) + u32(1) == u32(0), "u32 must be 32 bits");
static_assert(u64(-1) > 0, "u64 must be unsigned");
static_assert(u64(0xffffffffffffffff) + u32(1) == u32(0), "u64 must be 64 bits");

constexpr auto digits_00_99 =
    "00010203040506070809" "10111213141516171819" "20212223242526272829" "30313233343536373839"	"40414243444546474849"
    "50515253545556575859" "60616263646566676869" "70717273747576777879" "80818283848586878889"	"90919293949596979899";

struct pair { char t, o; };

#define JEAIII_W(I, U) *(pair*)&b[I] = *(pair*)&digits_00_99[(U) * 2]
#define JEAIII_A(I, N) t = (u64(1) << (32 + N / 5 * N * 53 / 16)) / u32(1e##N) + 1 + N / 6 - N / 8, t *= u, t >>= N / 5 * N * 53 / 16, t += N / 6 * 4, JEAIII_W(I, t >> 32)
#define JEAIII_S(I) b[I] = char(u64(10) * u32(t) >> 32) + '0'
#define JEAIII_D(I) t = u64(100) * u32(t), JEAIII_W(I, t >> 32)

#define JEAIII_C0(I) b[I] = char(u) + '0'
#define JEAIII_C1(I) JEAIII_W(I, u)
#define JEAIII_C2(I) JEAIII_A(I, 1), JEAIII_S(I + 2)
#define JEAIII_C3(I) JEAIII_A(I, 2), JEAIII_D(I + 2)
#define JEAIII_C4(I) JEAIII_A(I, 3), JEAIII_D(I + 2), JEAIII_S(I + 4)
#define JEAIII_C5(I) JEAIII_A(I, 4), JEAIII_D(I + 2), JEAIII_D(I + 4)
#define JEAIII_C6(I) JEAIII_A(I, 5), JEAIII_D(I + 2), JEAIII_D(I + 4), JEAIII_S(I + 6)
#define JEAIII_C7(I) JEAIII_A(I, 6), JEAIII_D(I + 2), JEAIII_D(I + 4), JEAIII_D(I + 6)
#define JEAIII_C8(I) JEAIII_A(I, 7), JEAIII_D(I + 2), JEAIII_D(I + 4), JEAIII_D(I + 6), JEAIII_S(I + 8)
#define JEAIII_C9(I) JEAIII_A(I, 8), JEAIII_D(I + 2), JEAIII_D(I + 4), JEAIII_D(I + 6), JEAIII_D(I + 8)

#define JEAIII_L(N, A, B) u < u32(1e##N) ? A : B
#define JEAIII_L09(F) JEAIII_L(2, JEAIII_L(1, F(0), F(1)), JEAIII_L(6, JEAIII_L(4, JEAIII_L(3, F(2), F(3)), JEAIII_L(5, F(4), F(5))), JEAIII_L(8, JEAIII_L(7, F(6), F(7)), JEAIII_L(9, F(8), F(9)))))
#define JEAIII_L03(F) JEAIII_L(2, JEAIII_L(1, F(0), F(1)), JEAIII_L(3, F(2), F(3)))

#define JEAIII_K(N) (JEAIII_C##N(0), b + N + 1)
#define JEAIII_KX(N) (JEAIII_C##N(0), u = x, JEAIII_C7(N + 1), b + N + 9)
#define JEAIII_KYX(N) (JEAIII_C##N(0), u = y, JEAIII_C7(N + 1), u = x, JEAIII_C7(N + 9), b + N + 17)

template<bool B, class T, class F> struct _cond { using type = F; };
template<class T, class F> struct _cond<true, T, F> { using type = T; };
template<bool B, class T, class F> using cond = typename _cond<B, T, F>::type;

template<class T> inline char* to_text_from_integer(char* b, T i)
{
    u64 t = u64(i);

    if (i < T(0))
        t = u64(0) - t, b[0] = '-', ++b;

    u32 u = cond<T(1) != T(2), cond<sizeof(T) != 1, cond<sizeof(T) != sizeof(short), u32, unsigned short>, unsigned char>, bool>(t);

    // if our input type fits in 32bits, or its value does, ctreat as 32bit (the line above ensures the compiler can still know the range limits of the input type)
    // and optimize out cases for small integer types (if only c++ had a builtin way to get the unsigned type from a signed type)
    if (sizeof(i) <= sizeof(u) || u == t)
        return JEAIII_L09(JEAIII_K);

    u32 x = t % 100000000u;
    u = u32(t /= 100000000u);

    // t / 10^8 (fits in 32 bit), t % 10^8 -> ~17.5 digits
    if (u == t)
        return JEAIII_L09(JEAIII_KX);

    // t / 10^16 (1-4 digits), t / 10^8 % 10^8, t % 10^8
    u32 y = t % 100000000u;
    u = u32(t / 100000000u);
    return JEAIII_L03(JEAIII_KYX);
}

inline char* to_text(char text[], signed char i) { return to_text_from_integer(text, i); }
inline char* to_text(char text[], unsigned char i) { return to_text_from_integer(text, i); }
inline char* to_text(char text[], short i) { return to_text_from_integer(text, i); }
inline char* to_text(char text[], unsigned short i) { return to_text_from_integer(text, i); }
inline char* to_text(char text[], int i) { return to_text_from_integer(text, i); }
inline char* to_text(char text[], unsigned int i) { return to_text_from_integer(text, i); }
inline char* to_text(char text[], long i) { return to_text_from_integer(text, i); }
inline char* to_text(char text[], unsigned long i) { return to_text_from_integer(text, i); }
inline char* to_text(char text[], long long i) { return to_text_from_integer(text, i); }
inline char* to_text(char text[], unsigned long long i) { return to_text_from_integer(text, i); }

// Copyright (c) 2022 Bill Sun
constexpr static __uint128_t _10_19 = 10000000000000000000ull, 
    _10_37 = _10_19*_10_19 / 10;

template<class T>
char* jeaiii_i128(char* buf, T v){
    if (v < 0){
        *(buf++) = '0';
        v = -v;
    }
    if (v > _10_37){
        uint8_t vv = uint8_t(v/_10_37);
        // vv <<= 1;
        // if (vv < 20)
        //     *buf ++ = digits_00_99[vv + 1];
        // else{
        //     *buf++ = digits_00_99[vv ];
        //     *buf++ = digits_00_99[vv + 1];
        // }  
    
        *(buf++) = vv%10 + '0';
        vv/=10;
        if (vv) {
            *buf = *(buf-1);
            *(buf++-1) = vv + '0';
        }
    }

    if (v > _10_19)
        buf = to_text(buf, uint64_t((v/_10_19) % _10_19));
    
    buf = to_text(buf, uint64_t(v % _10_19));
    return buf;
}