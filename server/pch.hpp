#pragma once

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
#else
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif
