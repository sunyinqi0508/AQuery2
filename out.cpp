#include "./udf.hpp"
#include "./server/monetdb_conn.h"
#include "./server/aggregations.h"
#include "./server/libaquery.h"

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        auto server = static_cast<Server*>(cxt->alt_server);
    auto len_6SzLPm = server->cnt;
auto sales_5fe = ColRef<int>(len_6SzLPm, server->getCol(0));
auto a_yJz = ColRef<int>(len_6SzLPm, server->getCol(1));
auto out_4UoFb5 = new TableInfo<value_type<decays<decltype((sd(a_yJz) + sales_5fe))>>>("out_4UoFb5");
out_4UoFb5->get_col<0>() = (sd(a_yJz) + sales_5fe);
print(*out_4UoFb5);
return 0;
}