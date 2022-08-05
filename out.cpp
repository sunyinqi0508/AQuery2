#include "./server/libaquery.h"
#include "./server/monetdb_conn.h"

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        auto server = static_cast<Server*>(cxt->alt_server);
    auto len_5sGusn = server->cnt;
auto sumc_5IN = ColRef<__int128_t>(len_5sGusn, server->getCol(0));
auto b_79y = ColRef<int>(len_5sGusn, server->getCol(1));
auto d_4yS = ColRef<int>(len_5sGusn, server->getCol(2));
auto out_kio0QJ = new TableInfo<__int128_t,int,int>("out_kio0QJ");
out_kio0QJ->get_col<0>().initfrom(sumc_5IN);
out_kio0QJ->get_col<1>().initfrom(b_79y);
out_kio0QJ->get_col<2>().initfrom(d_4yS);
print(*out_kio0QJ);
puts("done.");
return 0;
}