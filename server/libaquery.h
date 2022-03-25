#ifndef _AQUERY_H
#define _AQUERY_H

#include "table.h"
#include <unordered_map>
struct Context{
    std::unordered_map<const char*, void*> tables;
    std::unordered_map<const char*, uColRef *> cols;
};
#endif