/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2022 MonetDB B.V.
 */
/* monetdb_config.h.in.  Generated from CMakeLists.txt  */

#ifndef MT_SEEN_MONETDB_CONFIG_H
#define MT_SEEN_MONETDB_CONFIG_H 1

#ifdef _MSC_VER

#if _MSC_VER < 1900
#error Versions below Visual Studio 2015 are no longer supported
#endif

/* Prevent pollution through excessive inclusion of include files by Windows.h. */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

/* Visual Studio 8 has deprecated lots of stuff: suppress warnings */
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#define _CRT_RAND_S				/* for Windows rand_s, before stdlib.h */
#define HAVE_RAND_S 1

#endif

#if !defined(_XOPEN_SOURCE) && defined(__CYGWIN__)
#define _XOPEN_SOURCE 700
#endif

#include <stdlib.h>
#if defined(_MSC_VER) && defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)
/* In this case, malloc and friends are redefined in crtdbg.h to debug
 * versions.  We need to include stdlib.h first or else we get
 * conflicting declarations. */
#include <crtdbg.h>
#endif

#define HAVE_SYS_TYPES_H 1
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

/* standard C-99 include files */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER

/* Windows include files */
#include <process.h>
#include <windows.h>
#include <ws2tcpip.h>

/* indicate to sqltypes.h that windows.h has already been included and
   that it doesn't have to define Windows constants */
#define ALREADY_HAVE_WINDOWS_TYPE 1

#define NATIVE_WIN32 1

#endif /* _MSC_VER */

#if !defined(WIN32) && (defined(__CYGWIN__)||defined(__MINGW32__))
#define WIN32 1
#endif

// Section: monetdb configure defines
/* #undef HAVE_DISPATCH_DISPATCH_H */
#define HAVE_DLFCN_H 1
#define HAVE_FCNTL_H 1
#define HAVE_IO_H 1
/* #undef HAVE_KVM_H */
#define HAVE_LIBGEN_H 1
/* #undef HAVE_LIBINTL_H */
/* #undef HAVE_MACH_MACH_INIT_H */
/* #undef HAVE_MACH_TASK_H */
/* #undef HAVE_MACH_O_DYLD_H */
#define HAVE_NETDB_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_POLL_H 1
/* #undef HAVE_PROCFS_H */
#define HAVE_PWD_H 1
#define HAVE_STRINGS_H 1
/* #undef HAVE_STROPTS_H */
#define HAVE_SYS_FILE_H 1
#define HAVE_SYS_IOCTL_H 1
/* #undef HAVE_SYS_SYSCTL_H */
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_RANDOM_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TIMES_H 1
#define HAVE_SYS_UIO_H 1
#define HAVE_SYS_UN_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_UNISTD_H 1
#define HAVE_WINSOCK_H 1
#define HAVE_SEMAPHORE_H 1
#define HAVE_GETOPT_H 1

#define HAVE_STDATOMIC_H 1

#define HAVE_DIRENT_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_SYS_STAT_H 1
#define HAVE_FDATASYNC 1
#define HAVE_ACCEPT4 1
#define HAVE_ASCTIME_R 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_CTIME_R 1
/* #undef HAVE_DISPATCH_SEMAPHORE_CREATE */
/* #undef HAVE_FALLOCATE */
#define HAVE_FCNTL 1
#define HAVE_FORK 1
#define HAVE_FSYNC 1
#define HAVE_FTIME 1
#define HAVE_GETENTROPY 1
/* #undef HAVE_GETEXECNAME */
#define HAVE_GETLOGIN 1
#define HAVE_GETOPT_LONG 1
#define HAVE_GETRLIMIT 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_GETUID 1
#define HAVE_GMTIME_R 1
#define HAVE_LOCALTIME_R 1
#define HAVE_STRERROR_R 1
#define HAVE_LOCKF 1
#define HAVE_MADVISE  1
/* #undef HAVE_MREMAP */
#define HAVE_NANOSLEEP  1
#define HAVE_NL_LANGINFO  1
/* #undef HAVE__NSGETEXECUTABLEPATH */
/* #undef HAVE_PIPE2 */
#define HAVE_POLL  1
#define HAVE_POPEN 1
#define HAVE_POSIX_FADVISE 1
#define HAVE_POSIX_FALLOCATE 1
#define HAVE_POSIX_MADVISE 1
#define HAVE_PUTENV 1
#define HAVE_SETSID 1
#define HAVE_SHUTDOWN 1
#define HAVE_SIGACTION 1
#define HAVE_STPCPY 1
#define HAVE_STRCASESTR 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRPTIME 1
#define HAVE_STRSIGNAL 1
#define HAVE_SYSCONF 1
/* #undef HAVE_TASK_INFO */
#define HAVE_TIMES 1
#define HAVE_UNAME 1
/* #undef HAVE_SEMTIMEDOP */
#define HAVE_PTHREAD_KILL 1
#define HAVE_PTHREAD_SIGMASK 1
#define HAVE_GETOPT 1

#define ICONV_CONST
#define FLEXIBLE_ARRAY_MEMBER
#define ENABLE_MAPI 1
#define HAVE_MAPI 1
// End Section: monetdb configure defines

// Section: monetdb macro variables
/* #undef HAVE_ICONV */
#define HAVE_PTHREAD_H 1
/* #undef HAVE_LIBPCRE */
/* #undef HAVE_LIBBZ2 */
/* #undef HAVE_CURL */
/* #undef HAVE_LIBLZMA */
/* #undef HAVE_LIBXML */
/* #undef HAVE_LIBZ */
/* #undef HAVE_LIBLZ4 */
/* #undef HAVE_PROJ */
/* #undef HAVE_SNAPPY */
/* #undef HAVE_FITS */
/* #undef HAVE_VALGRIND */
/* #undef HAVE_NETCDF */
/* #undef HAVE_READLINE */
/* #undef HAVE_LIBR */
#define RHOME "/registry"
/* #undef HAVE_GEOM */
/* #undef HAVE_SHP */
/* #undef HAVE_LIBPY3 */

// #define SOCKET_LIBRARIES
#define HAVE_GETADDRINFO 1
/* #undef HAVE_CUDF */

#define MAPI_PORT 50000
#define MAPI_PORT_STR "50000"

#ifdef _MSC_VER
#define DIR_SEP '\\'
#define PATH_SEP ';'
#define DIR_SEP_STR "\\"
#define SO_PREFIX ""
#else
#define DIR_SEP '/'
#define PATH_SEP ':'
#define DIR_SEP_STR "/"
#define SO_PREFIX "lib"
#endif
#define SO_EXT ".dll"

#define BINDIR "C:/Program Files (x86)/MonetDB/bin"
#define LIBDIR "C:/Program Files (x86)/MonetDB/lib"
#define LOCALSTATEDIR "C:/Program Files (x86)/MonetDB/var"

// End Section: monetdb macro variables

// Section: monetdb configure misc
#define MONETDB_RELEASE "unreleased"

#define MONETDB_VERSION "11.44.0"
#define MONETDB_VERSION_MAJOR 11
#define MONETDB_VERSION_MINOR 44
#define MONETDB_VERSION_PATCH 0

#define GDK_VERSION "25.1.0"
#define GDK_VERSION_MAJOR 25
#define GDK_VERSION_MINOR 1
#define GDK_VERSION_PATCH 0
#define MAPI_VERSION "14.0.2"
#define MAPI_VERSION_MAJOR 14
#define MAPI_VERSION_MINOR 0
#define MAPI_VERSION_PATCH 2
#define MONETDB5_VERSION "32.0.6"
#define MONETDB5_VERSION_MAJOR 32
#define MONETDB5_VERSION_MINOR 0
#define MONETDB5_VERSION_PATCH 6
#define MONETDBE_VERSION "3.0.2"
#define MONETDBE_VERSION_MAJOR 3
#define MONETDBE_VERSION_MINOR 0
#define MONETDBE_VERSION_PATCH 2
#define STREAM_VERSION "16.0.1"
#define STREAM_VERSION_MAJOR 16
#define STREAM_VERSION_MINOR 0
#define STREAM_VERSION_PATCH 1
#define SQL_VERSION "12.0.5"
#define SQL_VERSION_MAJOR 12
#define SQL_VERSION_MINOR 0
#define SQL_VERSION_PATCH 5

/* Host identifier */
#define HOST "amd64-pc-windows-gnu"

/* The used password hash algorithm */
#define MONETDB5_PASSWDHASH "SHA512"

/* The used password hash algorithm */
#define MONETDB5_PASSWDHASH_TOKEN SHA512

#ifndef _Noreturn
#ifdef __cplusplus
#define _Noreturn
#else
/* #undef _Noreturn */
#endif
#endif
/* Does your compiler support `inline' keyword? (C99 feature) */
#ifndef inline
#ifdef __cplusplus
#define inline
#else
/* #undef inline */
#endif
#endif
/* Does your compiler support `restrict' keyword? (C99 feature) */
#ifndef restrict
#ifdef __cplusplus
#define restrict
#else
/* #undef restrict */
#endif
#endif

// End Section: monetdb configure misc

// Section: monetdb configure sizes
#define SIZEOF_SIZE_T 8

/* The size of `void *', as computed by sizeof. */
#define SIZEOF_VOID_P 8

#define SIZEOF_CHAR 1
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_DOUBLE 8
#define SIZEOF_WCHAR_T 2
#define HAVE_LONG_LONG 1	/* for ODBC include files */

#ifdef _MSC_VER
#ifdef _WIN64
#define LENP_OR_POINTER_T SQLLEN *
#else
#define LENP_OR_POINTER_T SQLPOINTER
#endif
#else
/* #undef LENP_OR_POINTER_T */
#endif
/* #undef SIZEOF_SQLWCHAR */

/* #undef WORDS_BIGENDIAN */

/* Does your compiler support `ssize_t' type? (Posix type) */
#ifndef ssize_t
/* #undef ssize_t */
#endif

/* The size of `__int128', as computed by sizeof. */
#define SIZEOF___INT128 16

/* The size of `__int128_t', as computed by sizeof. */
#define SIZEOF___INT128_T 16

/* The size of `__uint128_t', as computed by sizeof. */
#define SIZEOF___UINT128_T 16

#define HAVE___INT128 1
#define HAVE___INT128_T 1
#define HAVE___UINT128_T 1
/* #undef HAVE_HGE */

#ifdef HAVE_HGE
#ifdef HAVE___INT128
typedef __int128 hge;
typedef unsigned __int128 uhge;
#define SIZEOF_HGE SIZEOF___INT128
#elif defined(HAVE___INT128_T) && defined(HAVE___UINT128_T)
typedef __int128_t hge;
typedef __uint128_t uhge;
#define SIZEOF_HGE SIZEOF___INT128_T
#endif
#endif

// End Section: monetdb configure sizes

/* Does your compiler support `__attribute__' extension? */
#if !defined(__GNUC__) && !defined(__clang__) && !defined(__attribute__)
#define __attribute__(a)
#endif

#if !defined(__cplusplus) || __cplusplus < 201103L
#ifndef static_assert
/* static_assert is a C11/C++11 feature, defined in assert.h which also exists
 * in many other compilers we ignore it if the compiler doesn't support it
 * However in C11 static_assert is a macro, while on C++11 is a keyword */
#define static_assert(expr, mesg)	((void) 0)
#endif
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>		/* strcasecmp */
#endif

#ifdef _MSC_VER

#define strdup(s)	_strdup(s)

#ifndef strcasecmp
#define strcasecmp(x,y) _stricmp(x,y)
#endif

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1
#ifndef strncasecmp
#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#endif

#include <sys/stat.h>
#ifdef lstat
#undef lstat
#endif
#define lstat _stat64
#ifdef stat
#undef stat
#endif
#define stat _stat64
#ifdef fstat
#undef fstat
#endif
#define fstat _fstat64

static inline char *
stpcpy(char *restrict dst, const char *restrict src)
{
	size_t i;
	for (i = 0; src[i]; i++)
		dst[i] = src[i];
	dst[i] = 0;
	return dst + i;
}

/* Define to 1 if the system has the type `socklen_t'. */
#define HAVE_SOCKLEN_T 1
/* type used by connect */
#define socklen_t int
#define strtok_r(t,d,c) strtok_s(t,d,c)

#define HAVE_GETOPT_LONG 1

/* there is something very similar to localtime_r on Windows: */
#include <time.h>
#define HAVE_LOCALTIME_R 1
static inline struct tm *
localtime_r(const time_t *restrict timep, struct tm *restrict result)
{
	return localtime_s(result, timep) == 0 ? result : NULL;
}
#define HAVE_GMTIME_R 1
static inline struct tm *
gmtime_r(const time_t *restrict timep, struct tm *restrict result)
{
	return gmtime_s(result, timep) == 0 ? result : NULL;
}

/* Define if you have ctime_r(time_t*,char *buf,size_t s) */
#define HAVE_CTIME_R 1
#define HAVE_CTIME_R3 1
/* there is something very similar to ctime_r on Windows: */
#define ctime_r(t,b,s)  (ctime_s(b,s,t) ? NULL : (b))

#endif	/* _MSC_VER */

/* #undef HAVE_SOCKLEN_T */
#ifndef _MSC_VER
#define SOCKET int
#define closesocket close
#endif

#ifndef _In_z_
#define _In_z_
#endif
#ifndef _Printf_format_string_
#define _Printf_format_string_
#endif

#ifdef _MSC_VER
#define _LIB_STARTUP_FUNC_(f,q) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" q #f "_")) \
        static void f(void)
#ifdef _WIN64
        #define LIB_STARTUP_FUNC(f) _LIB_STARTUP_FUNC_(f,"")
#else
        #define LIB_STARTUP_FUNC(f) _LIB_STARTUP_FUNC_(f,"_")
#endif
#else
#define LIB_STARTUP_FUNC(f) \
        static void f(void) __attribute__((__constructor__)); \
        static void f(void)
#endif

#endif /* MT_SEEN_MONETDB_CONFIG_H */
