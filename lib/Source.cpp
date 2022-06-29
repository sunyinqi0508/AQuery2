#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
#include "monetdbe.h"
int main() {
	monetdbe_database db;
	puts("alive");
	monetdbe_open(&db, NULL, NULL);
	printf("%lx", db);
	monetdbe_result *res;
	monetdbe_cnt cnt = 0;
	char* ret = 0;
	ret = monetdbe_query(db, const_cast<char*>(" CREATE TABLE test(a INT, b INT, c INT, d INT);\n"), &res, &cnt);
	if(ret) puts(ret);
	ret = monetdbe_query(db, const_cast<char*>("  COPY OFFSET 2 INTO test FROM 'd:/gg/AQuery++/test.csv' ON SERVER USING DELIMITERS ',';"), &res, &cnt);
	if(ret) puts(ret);
	ret = monetdbe_query(db, const_cast<char*>(" SELECT SUM(c), b, d FROM test GROUP BY a, b, d  ORDER BY d DESC, b ;"), &res, &cnt);
	if(ret) puts(ret);
	monetdbe_column* cols;
	monetdbe_result_fetch(res, &cols, 0);
	printf("%lx, %d\n", cols, cols->count);
	for(int i = 0; i < cols->count; ++i)
		printf("%d ", ((long long*)cols->data)[i]);
	monetdbe_close(db);
	return 0;
}
