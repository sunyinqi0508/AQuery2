#include "./server/aggregations.h"
#include "./udf.hpp"
#include "./server/hasher.h"
#include <unordered_map>
#include "./server/monetdb_conn.h"
#include "./server/libaquery.h"

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        auto server = static_cast<Server*>(cxt->alt_server);
    auto len_4fxytV = server->cnt;
auto b_3pr = ColRef<int>(len_4fxytV, server->getCol(0));
auto a_65L = ColRef<int>(len_4fxytV, server->getCol(1));
auto out_2UnEpP = new TableInfo<value_type<decays<decltype(covariances2_gettype(a_65L, b_3pr, 4))>>>("out_2UnEpP");
auto col_C9QF0Z = out_2UnEpP->get_col<0>();
for (uint32_t i41 = 0; i41 < t1K4f4I0.size; ++i41){
g1zdpLFa[forward_as_tuple(t1K4f4I0[i41])].emplace_back(i41);
}
for (const auto& i40 : g1zdpLFa){
auto &len_5NTOM6m = val_2423Z8E.size;
auto &key_6fZPUDS = i4O.first;
auto &val_2423Z8E = i4O.second;
col_C9QF0Z.emplace_back({len_5NTOM6m});

covariances2(a_65L[val_2423Z8E], b_3pr[val_2423Z8E], 4, len_5NTOM6m, col_C9QF0Z.back());

}
print(*out_2UnEpP);
return 0;
}