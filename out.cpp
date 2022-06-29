#include "./server/libaquery.h"
#include "./udf.hpp"
#include "./server/monetdb_conn.h"
#include "./server/aggregations.h"

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        auto server = static_cast<Server*>(cxt->alt_server);
    auto len_6WMRXO = server->cnt;
auto suma_6BP = ColRef<int64_t>(len_6WMRXO, server->getCol(0));
auto b_5Yb = ColRef<int>(len_6WMRXO, server->getCol(1));
auto c_2Vh = ColRef<int>(len_6WMRXO, server->getCol(2));
auto d_1Ma = ColRef<int>(len_6WMRXO, server->getCol(3));
auto out_2URo7p = new TableInfo<value_type<decays<decltype((paircorr(c_2Vh, b_5Yb) * d_1Ma))>>,int64_t,int>("out_2URo7p");
out_2URo7p->get_col<0>() = (paircorr(c_2Vh, b_5Yb) * d_1Ma);
out_2URo7p->get_col<1>().initfrom(suma_6BP);
out_2URo7p->get_col<2>().initfrom(b_5Yb);
print(*out_2URo7p);
return 0;
}