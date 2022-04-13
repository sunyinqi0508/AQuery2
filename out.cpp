#include "./server/libaquery.h"
#include "./server/aggregations.h"

    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
    auto stocks = new TableInfo<int,int>("stocks", 2);
cxt->tables.insert({"stocks", stocks});
auto& stocks_timestamp = *(ColRef<int> *)(&stocks->colrefs[0]);
auto& stocks_price = *(ColRef<int> *)(&stocks->colrefs[1]);
stocks_timestamp.init();
stocks_price.init();
stocks_timestamp.emplace_back(1);
stocks_price.emplace_back(15);
stocks_timestamp.emplace_back(2);
stocks_price.emplace_back(19);
stocks_timestamp.emplace_back(3);
stocks_price.emplace_back(16);
stocks_timestamp.emplace_back(4);
stocks_price.emplace_back(17);
stocks_timestamp.emplace_back(5);
stocks_price.emplace_back(15);
stocks_timestamp.emplace_back(6);
stocks_price.emplace_back(13);
stocks_timestamp.emplace_back(7);
stocks_price.emplace_back(5);
stocks_timestamp.emplace_back(8);
stocks_price.emplace_back(8);
stocks_timestamp.emplace_back(9);
stocks_price.emplace_back(7);
stocks_timestamp.emplace_back(10);
stocks_price.emplace_back(13);
stocks_timestamp.emplace_back(11);
stocks_price.emplace_back(11);
stocks_timestamp.emplace_back(12);
stocks_price.emplace_back(14);
stocks_timestamp.emplace_back(13);
stocks_price.emplace_back(10);
stocks_timestamp.emplace_back(14);
stocks_price.emplace_back(5);
stocks_timestamp.emplace_back(15);
stocks_price.emplace_back(2);
stocks_timestamp.emplace_back(16);
stocks_price.emplace_back(5);
auto out_6gPn = new TableInfo<decays<decltype(max((stocks_price[0]-min(stocks_timestamp[0]))))>>("out_6gPn", 1);
cxt->tables.insert({"out_6gPn", out_6gPn});
auto& out_6gPn_maxstockspriceminstockstimestamp = *(ColRef<decays<decltype(max((stocks_price[0]-min(stocks_timestamp[0]))))>> *)(&out_6gPn->colrefs[0]);
out_6gPn_maxstockspriceminstockstimestamp.init();
out_6gPn_maxstockspriceminstockstimestamp = max((stocks_price-min(stocks_timestamp)));
print(*out_6gPn);

auto out_7a2d = new TableInfo<decays<decltype(max((stocks_price[0]-mins(stocks_price[0]))))>>("out_7a2d", 1);
cxt->tables.insert({"out_7a2d", out_7a2d});
auto& out_7a2d_maxstockspriceminsstocksprice = *(ColRef<decays<decltype(max((stocks_price[0]-mins(stocks_price[0]))))>> *)(&out_7a2d->colrefs[0]);
out_7a2d_maxstockspriceminsstocksprice.init();
out_7a2d_maxstockspriceminsstocksprice = max((stocks_price-mins(stocks_price)));
print(*out_7a2d);

return 0;
}