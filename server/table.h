#ifndef _TABLE_H
#define _TABLE_H

#include "types.h"
#include "vector_type.hpp"

template <class T>
class vector_type;

#ifdef _MSC_VER
namespace types {
	enum Type_t;
}
#endif

template<typename Ty>
class ColRef : public vector_type<Ty>
{
	const char* name;
	types::Type_t ty;
	ColRef(const char* name) : name(name) {}
};


class TableInfo {
	const char* name;
	ColRef<void>* colrefs;
	uint32_t n_cols;
};

#endif