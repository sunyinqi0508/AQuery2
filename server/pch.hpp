#ifndef __AQ_PCH_H__
#define __AQ_PCH_H__

#include "libaquery.h"
#include "aggregations.h"
#include <iostream>
#include <chrono>
#include "monetdb_conn.h"
#include "monetdbe.h"
#include "threading.h"
#include "../csv.h"
#ifdef _WIN32
#include "winhelper.h"
#undef max
#undef min
#else
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif
#endif