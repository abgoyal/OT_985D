


#ifndef __ACENV_H__
#define __ACENV_H__

/* Types for ACPI_MUTEX_TYPE */

#define ACPI_BINARY_SEMAPHORE       0
#define ACPI_OSL_MUTEX              1

/* Types for DEBUGGER_THREADING */

#define DEBUGGER_SINGLE_THREADED    0
#define DEBUGGER_MULTI_THREADED     1


#ifdef ACPI_LIBRARY
#define ACPI_USE_LOCAL_CACHE
#endif

#ifdef ACPI_ASL_COMPILER
#define ACPI_DEBUG_OUTPUT
#define ACPI_APPLICATION
#define ACPI_DISASSEMBLER
#define ACPI_CONSTANT_EVAL_ONLY
#define ACPI_LARGE_NAMESPACE_NODE
#define ACPI_DATA_TABLE_DISASSEMBLY
#endif

#ifdef ACPI_EXEC_APP
#undef DEBUGGER_THREADING
#define DEBUGGER_THREADING      DEBUGGER_SINGLE_THREADED
#define ACPI_FULL_DEBUG
#define ACPI_APPLICATION
#define ACPI_DEBUGGER
#define ACPI_MUTEX_DEBUG
#define ACPI_DBG_TRACK_ALLOCATIONS
#endif

#ifdef ACPI_APPLICATION
#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_LOCAL_CACHE
#endif

#ifdef ACPI_FULL_DEBUG
#define ACPI_DEBUGGER
#define ACPI_DEBUG_OUTPUT
#define ACPI_DISASSEMBLER
#endif


/*! [Begin] no source code translation */

#if defined(_LINUX) || defined(__linux__)
#include "aclinux.h"

#elif defined(_AED_EFI)
#include "acefi.h"

#elif defined(WIN32)
#include "acwin.h"

#elif defined(WIN64)
#include "acwin64.h"

#elif defined(MSDOS)		/* Must appear after WIN32 and WIN64 check */
#include "acdos16.h"

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "acfreebsd.h"

#elif defined(__NetBSD__)
#include "acnetbsd.h"

#elif defined(MODESTO)
#include "acmodesto.h"

#elif defined(NETWARE)
#include "acnetware.h"

#elif defined(__sun)
#include "acsolaris.h"

#else

/* All other environments */

#define ACPI_USE_STANDARD_HEADERS

#define COMPILER_DEPENDENT_INT64   long long
#define COMPILER_DEPENDENT_UINT64  unsigned long long

#endif

/*! [End] no source code translation !*/


#ifndef ACPI_MUTEX_TYPE
#define ACPI_MUTEX_TYPE             ACPI_BINARY_SEMAPHORE
#endif

#ifndef DEBUGGER_THREADING
#ifdef ACPI_APPLICATION
#define DEBUGGER_THREADING          DEBUGGER_SINGLE_THREADED

#else
#define DEBUGGER_THREADING          DEBUGGER_MULTI_THREADED
#endif
#endif				/* !DEBUGGER_THREADING */


#define ACPI_IS_ASCII(c)  ((c) < 0x80)

#ifdef ACPI_USE_SYSTEM_CLIBRARY
#ifdef ACPI_USE_STANDARD_HEADERS
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#endif				/* ACPI_USE_STANDARD_HEADERS */

#define ACPI_STRSTR(s1,s2)      strstr((s1), (s2))
#define ACPI_STRCHR(s1,c)       strchr((s1), (c))
#define ACPI_STRLEN(s)          (acpi_size) strlen((s))
#define ACPI_STRCPY(d,s)        (void) strcpy((d), (s))
#define ACPI_STRNCPY(d,s,n)     (void) strncpy((d), (s), (acpi_size)(n))
#define ACPI_STRNCMP(d,s,n)     strncmp((d), (s), (acpi_size)(n))
#define ACPI_STRCMP(d,s)        strcmp((d), (s))
#define ACPI_STRCAT(d,s)        (void) strcat((d), (s))
#define ACPI_STRNCAT(d,s,n)     strncat((d), (s), (acpi_size)(n))
#define ACPI_STRTOUL(d,s,n)     strtoul((d), (s), (acpi_size)(n))
#define ACPI_MEMCMP(s1,s2,n)    memcmp((const char *)(s1), (const char *)(s2), (acpi_size)(n))
#define ACPI_MEMCPY(d,s,n)      (void) memcpy((d), (s), (acpi_size)(n))
#define ACPI_MEMSET(d,s,n)      (void) memset((d), (s), (acpi_size)(n))

#define ACPI_TOUPPER(i)         toupper((int) (i))
#define ACPI_TOLOWER(i)         tolower((int) (i))
#define ACPI_IS_XDIGIT(i)       isxdigit((int) (i))
#define ACPI_IS_DIGIT(i)        isdigit((int) (i))
#define ACPI_IS_SPACE(i)        isspace((int) (i))
#define ACPI_IS_UPPER(i)        isupper((int) (i))
#define ACPI_IS_PRINT(i)        isprint((int) (i))
#define ACPI_IS_ALPHA(i)        isalpha((int) (i))

#else


 /*
  * Use local definitions of C library macros and functions
  * NOTE: The function implementations may not be as efficient
  * as an inline or assembly code implementation provided by a
  * native C library.
  */

#ifndef va_arg

#ifndef _VALIST
#define _VALIST
typedef char *va_list;
#endif				/* _VALIST */

#define  _AUPBND                (sizeof (acpi_native_int) - 1)
#define  _ADNBND                (sizeof (acpi_native_int) - 1)

#define _bnd(X, bnd)            (((sizeof (X)) + (bnd)) & (~(bnd)))
#define va_arg(ap, T)           (*(T *)(((ap) += (_bnd (T, _AUPBND))) - (_bnd (T,_ADNBND))))
#define va_end(ap)              (void) 0
#define va_start(ap, A)         (void) ((ap) = (((char *) &(A)) + (_bnd (A,_AUPBND))))

#endif				/* va_arg */

#define ACPI_STRSTR(s1,s2)      acpi_ut_strstr ((s1), (s2))
#define ACPI_STRCHR(s1,c)       acpi_ut_strchr ((s1), (c))
#define ACPI_STRLEN(s)          (acpi_size) acpi_ut_strlen ((s))
#define ACPI_STRCPY(d,s)        (void) acpi_ut_strcpy ((d), (s))
#define ACPI_STRNCPY(d,s,n)     (void) acpi_ut_strncpy ((d), (s), (acpi_size)(n))
#define ACPI_STRNCMP(d,s,n)     acpi_ut_strncmp ((d), (s), (acpi_size)(n))
#define ACPI_STRCMP(d,s)        acpi_ut_strcmp ((d), (s))
#define ACPI_STRCAT(d,s)        (void) acpi_ut_strcat ((d), (s))
#define ACPI_STRNCAT(d,s,n)     acpi_ut_strncat ((d), (s), (acpi_size)(n))
#define ACPI_STRTOUL(d,s,n)     acpi_ut_strtoul ((d), (s), (acpi_size)(n))
#define ACPI_MEMCMP(s1,s2,n)    acpi_ut_memcmp((const char *)(s1), (const char *)(s2), (acpi_size)(n))
#define ACPI_MEMCPY(d,s,n)      (void) acpi_ut_memcpy ((d), (s), (acpi_size)(n))
#define ACPI_MEMSET(d,v,n)      (void) acpi_ut_memset ((d), (v), (acpi_size)(n))
#define ACPI_TOUPPER(c)         acpi_ut_to_upper ((int) (c))
#define ACPI_TOLOWER(c)         acpi_ut_to_lower ((int) (c))

#endif				/* ACPI_USE_SYSTEM_CLIBRARY */



/* Unrecognized compiler, use defaults */

#ifndef ACPI_ASM_MACROS

#define ACPI_SYSTEM_XFACE
#define ACPI_EXTERNAL_XFACE
#define ACPI_INTERNAL_XFACE
#define ACPI_INTERNAL_VAR_XFACE

#define ACPI_ASM_MACROS
#define BREAKPOINT3
#define ACPI_DISABLE_IRQS()
#define ACPI_ENABLE_IRQS()
#define ACPI_ACQUIRE_GLOBAL_LOCK(Glptr, acq)
#define ACPI_RELEASE_GLOBAL_LOCK(Glptr, acq)

#endif				/* ACPI_ASM_MACROS */

#ifdef ACPI_APPLICATION

/* Don't want software interrupts within a ring3 application */

#undef BREAKPOINT3
#define BREAKPOINT3
#endif

#endif				/* __ACENV_H__ */
