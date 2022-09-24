// TODO: Think of a way of decoupling table.h part and 
//		monetdbe.h part to speed up compilation.

#ifndef __TABLE_EXT_MONETDB_HPP__
#define __TABLE_EXT_MONETDB_HPP__

#include "table.h"
#include "monetdb_conn.h"
#include "monetdbe.h"

inline constexpr monetdbe_types AQType_2_monetdbe[] = {
		monetdbe_int32_t, monetdbe_float, monetdbe_str, monetdbe_double, monetdbe_double, monetdbe_int64_t,
#ifdef HAVE_HGE
		monetdbe_int128_t,
#else 
		monetdbe_int64_t,
#endif
		monetdbe_int16_t, monetdbe_date, monetdbe_time, monetdbe_int8_t,
		monetdbe_int32_t, monetdbe_int64_t,
#ifdef HAVE_HGE
		monetdbe_int128_t,
#else 
		monetdbe_int64_t,
#endif
		monetdbe_int16_t, monetdbe_int8_t, monetdbe_bool, monetdbe_int128_t,
		monetdbe_timestamp, monetdbe_int64_t, monetdbe_int64_t
};

template<class ...Ts>
void TableInfo<Ts ...>::monetdb_append_table(void* srv, const char* alt_name) {
	if (!alt_name){
		alt_name = this->name;
	}

	monetdbe_column** monetdbe_cols = new monetdbe_column * [sizeof...(Ts)];
	
	uint32_t i = 0;
	constexpr auto n_vecs = count_vector_type((tuple_type*)(0));
	void* gc_vecs[1 + n_vecs];
	puts("getcols...");
	uint32_t cnt = 0;
	const auto get_col = [&monetdbe_cols, &i, *this, &gc_vecs, &cnt](auto v) {
		printf("%d %d\n", i, (ColRef<void>*)v - colrefs);
		monetdbe_cols[i++] = (monetdbe_column*)v->monetdb_get_col(gc_vecs, cnt);
	};
	(get_col((ColRef<Ts>*)(colrefs + i)), ...);
	puts("getcols done");
	for(int i = 0; i < sizeof...(Ts); ++i)
	{
		printf("no:%d name: %s count:%d data: %p type:%d \n", 
		i, monetdbe_cols[i]->name, monetdbe_cols[i]->count, monetdbe_cols[i]->data, monetdbe_cols[i]->type);
	}
	std::string create_table_str = "CREATE TABLE IF NOT EXISTS ";
	create_table_str += alt_name;
	create_table_str += " (";
	i = 0;
	const auto get_name_type = [&i, *this, &create_table_str](auto v) {
		create_table_str+= std::string(colrefs[i++].name) + ' ' + 
		std::string(types::SQL_Type[types::Types<std::remove_pointer_t<decltype(v)>>::getType()]) + ", ";
	};
	(get_name_type((Ts*)(0)), ...);
	auto last_comma = create_table_str.find_last_of(',');
	if (last_comma != static_cast<decltype(last_comma)>(-1)) {
		create_table_str[last_comma] = ')';
		Server* server = (Server*)srv;
		puts("create table...");
		puts(create_table_str.c_str());
		server->exec(create_table_str.c_str());
		if (!server->last_error) {
			auto err = monetdbe_append(*((monetdbe_database*)server->server), "sys", alt_name, monetdbe_cols, sizeof...(Ts));
			if (err)
				puts(err);
			return;
		}
	}
	// for(uint32_t i = 0; i < n_vecs; ++i) 
	// 		free(gc_vecs[i]);
	puts("Error! Empty table.");
}


template<class Type>
void* ColRef<Type>::monetdb_get_col(void** gc_vecs, uint32_t& cnt) {
	auto aq_type = AQType_2_monetdbe[types::Types<Type>::getType()];
	monetdbe_column* col = (monetdbe_column*)malloc(sizeof(monetdbe_column));

	col->type = aq_type;
	col->count = this->size;
	col->data = this->container;
	col->name = const_cast<char*>(this->name);
	// auto arr = (types::timestamp_t*) malloc (sizeof(types::timestamp_t)* this->size);
	// if constexpr (is_vector_type<Type>){
	// 	for(uint32_t i = 0; i < this->size; ++i){
	// 		memcpy(arr + i, this->container + i, sizeof(types::timestamp_t));
	// 	}
	// 	gc_vecs[cnt++] = arr;
	// }
	return col;
}

#endif
