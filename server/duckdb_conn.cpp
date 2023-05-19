#include "pch_msc.hpp"
#include "duckdb_conn.h"
#include "duckdb.hpp"
#include "libaquery.h"
#include "types.h"
#include <cstdio>
#include <string_view>
using namespace std;

void DuckdbServer::connect(Context* cxt) {
	duckdb_database* db_handle = 
		static_cast<duckdb_database*>(malloc(sizeof(duckdb_database)));
	this->handle = db_handle;
	bool status = duckdb_open(nullptr, db_handle);
	duckdb_connection* conn_handle = 
		static_cast<duckdb_connection*>(malloc(sizeof(duckdb_connection)));;
	status = status || duckdb_connect(*db_handle, conn_handle);
	this->server = conn_handle;
	if (status != 0) {
		puts("DuckdbServer: Error! Creating/Connecting to INMemory DB.");
	}
}

DuckdbServer::DuckdbServer(Context* cxt) {
	this->DataSourceType = BACKEND_DuckDB;
	this->cxt = cxt;
	connect(cxt);
}

void DuckdbServer::exec(const char* q) {
	//TODO: Add res to GC queue with ref count.
	auto res = static_cast<duckdb_result*>(malloc(sizeof(duckdb_result)));
	auto status = duckdb_query(*static_cast<duckdb_connection*>(this->server), q, res);
	if (status) {
		last_error = duckdb_result_error(res);
		this->res = nullptr;
		this->cnt = 0;
	}
	else {
		this->res = res;
		this->cnt = duckdb_row_count(res);
	}
}

void* DuckdbServer::getCol(int col_idx, int ty) {
	auto res = static_cast<duckdb_result*>(this->res);
	if (ty == types::Type_t::ASTR) {
		std::string_view* ret = 
			static_cast<string_view*>(malloc(sizeof(std::string_view) * cnt));
		for(uint32_t i = 0; i < cnt; ++i)
			ret[i] = {duckdb_value_varchar(res, col_idx, i)};
		return ret;
	}
	else
	{ 
		auto chk = duckdb_result_chunk_count(*res);
		const auto v_size = duckdb_vector_size();
		auto sz = types::AType_sizes[ty];
		char* ret = static_cast<char*>(malloc(sz * cnt));
		uint32_t j = 0;
		for (uint32_t i = 0; i < chk; ++i) {
				auto data = duckdb_vector_get_data(
					duckdb_data_chunk_get_vector(
						duckdb_result_get_chunk(*res, i), 
						col_idx)
				);
				const auto curr_ptr = i * v_size;
				const auto rem_size = int(cnt) - curr_ptr;
				memcpy(ret + i * v_size, data, 
					(rem_size < v_size ? rem_size : v_size) * sz);
		}
		return ret;
	}
}

void DuckdbServer::getDSTable(const char* name, void* tbl) { 
	// not implemented. 
	puts("NOT IMPLEMENTED ERROR: DuckdbServer::getDSTable");
}
bool DuckdbServer::haserror() {
	if (last_error) {
		puts(last_error);
		last_error = nullptr;
		return true;
	}
	return false;
}

void DuckdbServer::close() {
	duckdb_disconnect(static_cast<duckdb_connection*>(server));
	duckdb_close(static_cast<duckdb_database*>(handle));
}

DuckdbServer::~DuckdbServer() {
	this->close();
}
