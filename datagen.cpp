#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>
using uniform = std::uniform_int_distribution<unsigned int>;
std::random_device rd;
std::mt19937_64 engine = std::mt19937_64(rd());
uniform ui, g_quantity = uniform(100, 10000);

int getInt(char*& buf) {
    while ((*buf < '0' || *buf > '9') && *buf != '.' && *buf) ++buf;
    int res = 0;
    while (*buf >= '0' && *buf <= '9')
        res = res * 10 + *buf++ - '0';
    return res;
}

float getFloat(char*& buf) {
    float res = static_cast<float>(getInt(buf));
    if (*buf == '.') {
        ++buf;
        float frac = 1.f;
        while (*buf >= '0' && *buf <= '9')
            res += (*buf++ - '0') * (frac /= 10.f);
    }
    return res;
}

void permutation(int *v, int n) {
    for (int i = 0; i < n; ++i)
    { // ensures each element have 1/N chance of being at place i
        int j = i + ui(engine) % static_cast<unsigned int>(n - i);
        int t = v[i];
        v[i] = v[j];
        v[j] = t;
    }
}

int gen_trade_data(int argc, char* argv[])
{
    using std::vector;
    float frac = .3;
    int N = 70000;
    int n_rows = 10000000;
	if (argc > 2) {
        frac = getFloat(argv[1]);
        N = getInt(argv[2]);
        n_rows = getInt(argv[3]);
	}
    else {
        printf("No parameter supplied. Use default frac=%f, N=%d? [Y/n]\n", frac, N);
        char buf[4096]; fgets(buf, 4095, stdin); 
        if((buf[0] != 'y' && buf[0] !='Y') && buf[0] != '\n') { 
            const auto &getParams = [&](){ puts("Type: frac N [ENTER]");
                for(int i = 0; i < 4096; ++i) if(buf[i] == '\n') {buf[i] = 0; break;}
                char* _buf = buf; frac = getFloat(_buf); N = getInt(_buf); n_rows=getInt(_buf);
            }; getParams();
            while ((frac <= 0 || frac >= 1) && N < 1&&n_rows<1) { fgets(buf, 4095, stdin); getParams(); }
        }
    }
    printf("\nfrac = %f, N = %d\n", frac, N);
    vector<int> v_lens;
    int curr_len = N;
    int s_len = 0;
    while (curr_len >= 1) {
        curr_len *= frac;
        v_lens.push_back(s_len);
        s_len += curr_len;
    }
    int* lens = v_lens.data();
    int llens = v_lens.size();
    for (int i = 0; i < llens; ++i)
        lens[i] = s_len - lens[i];
    lens[0] = s_len;
    int *p = new int[lens[0] + N];
    for (int i = 0; i < N; ++i) p[lens[0] + i] = i + 1;
    permutation(p + lens[0], N);
    for (int i = 1; i < llens; ++i)
        memmove(p + lens[i], p + lens[0], (lens[i - 1] - lens[i]) * sizeof(int));
    permutation(p, lens[0] + N);
    // for (int i = 0; i < lens[0] + N; ++i) printf("%d ", p[i]);
    FILE* fp = fopen("trade.csv", "w");
    int* last_price = new int[N];
    memset(last_price, -1, sizeof(int) * N);
    fprintf(fp, "stocksymbol, time, quantity, price\n");
    struct data{int ss, t, q, p;} *d = new data[n_rows];

    using std::min; using std::max;
    for (int i = 0; i < n_rows; ++i) {
        int ss = p[ui(engine) % (lens[0]+N)];
        int current_price = last_price[ss - 1];
        if (current_price < 0)
            current_price = ui(engine) % 451 + 50;
        else {
            int l = max(50, current_price - 5);
            int h = min(500, current_price + 5);
            unsigned int interval = max(0, current_price - l) + max(0, h - current_price);
            int new_price = l + ui(engine) % interval; 
            if (new_price >= current_price) //make sure every candidate have equal chance of being chosen
                ++new_price;
            current_price = new_price;
        }
        d[i]= {ss, i+1, (int)g_quantity(engine), current_price};
        fprintf(fp, "s%d, %d, %d, %d\n", d[i].ss, d[i].t, d[i].q, d[i].p);
        last_price[ss - 1] = current_price;
    }
    fclose(fp);
    return 0;
}
#include "./server/utils.h"
#include "./server/types.h"
#include <string>
types::date_t rand_date(){
    unsigned char d = ui(engine) % 28 + 1;
    unsigned char m = ui(engine) % 12 + 1;
    short y = ui(engine) % 40 + 1990;
    if (ui(engine) % 2) return types::date_t((unsigned char)10, (unsigned char)1, 2003);
    return types::date_t{d, m, y};
}
int gen_stock_data(int argc, char* argv[]){
    using std::string;
    using namespace types;
    int n_stocks = 5;
    int n_data = 1000;
    string* IDs = new string[n_stocks + 1];
    string* names = new string[n_stocks + 1];
    for(int i = 0; i < n_stocks; ++i){
        IDs[i] = base62uuid();
        names[i] = base62uuid();
    }
    IDs[n_stocks] = "S";
    names[n_stocks] = "x";
    FILE* fp = fopen("./data/stock.csv", "w");
    fprintf(fp, "ID, timestamp, tradeDate, price\n");
    char date_str_buf [types::date_t::string_length()];
    int* timestamps = new int[n_data];
    for(int i = 0; i < n_data; ++i) timestamps[i] = i+1;
    permutation(timestamps, n_data);
    for(int i = 0; i < n_data; ++i){
        auto date = rand_date().toString(date_str_buf + date_t::string_length());
        fprintf(fp, "%s,%d,%s,%d\n", IDs[ui(engine)%(n_stocks + 1)].c_str(), timestamps[i], date, ui(engine) % 1000);
    }
    fclose(fp);
    fp = fopen("./data/base.csv", "w");
    fprintf(fp, "ID, name\n");
    for(int i = 0; i < n_stocks + 1; ++ i){
        fprintf(fp, "%s,%s\n", IDs[i].c_str(), names[i].c_str());
    }
    fclose(fp);
}

int main(int argc, char* argv[]){
    return gen_stock_data(argc, argv);
}
