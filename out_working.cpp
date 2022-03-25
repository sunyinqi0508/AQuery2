#include "./server/libaquery.h"
#include <cstdio>
#include <threads.h>
extern "C" int dllmain(Context *cxt)
{
    auto stocks = new TableInfo<int, int>("stocks", 2);
    cxt->tables.insert(std::make_pair("stocks", stocks));
    stocks->colrefs[0].ty = types::AINT;
    stocks->colrefs[1].ty = types::AINT;
    auto &stocks_0 = *(ColRef<int> *)(&stocks->colrefs[0]);
    auto &stocks_1 = *(ColRef<int> *)(&stocks->colrefs[1]);
    stocks_0.emplace_back(1);
    stocks_1.emplace_back(15);
    stocks_0.emplace_back(2);
    stocks_1.emplace_back(19);
    stocks_0.emplace_back(3);
    stocks_1.emplace_back(16);
    stocks_0.emplace_back(4);
    stocks_1.emplace_back(17);
    stocks_0.emplace_back(5);
    stocks_1.emplace_back(15);
    stocks_0.emplace_back(6);
    stocks_1.emplace_back(13);
    stocks_0.emplace_back(7);
    stocks_1.emplace_back(5);
    stocks_0.emplace_back(8);
    stocks_1.emplace_back(8);
    stocks_0.emplace_back(9);
    stocks_1.emplace_back(7);
    stocks_0.emplace_back(10);
    stocks_1.emplace_back(13);
    stocks_0.emplace_back(11);
    stocks_1.emplace_back(11);
    stocks_0.emplace_back(12);
    stocks_1.emplace_back(14);
    stocks_0.emplace_back(13);
    stocks_1.emplace_back(10);
    stocks_0.emplace_back(14);
    stocks_1.emplace_back(5);
    stocks_0.emplace_back(15);
    stocks_1.emplace_back(2);
    stocks_0.emplace_back(16);
    stocks_1.emplace_back(5);
    printf("%d\n", max(stocks_0 - min(stocks_1)));
    return 0;
}