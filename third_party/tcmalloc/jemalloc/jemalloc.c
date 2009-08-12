/* -*- Mode: C; tab-width: 8; c-basic-offset: 8 -*- */
/* vim:set softtabstop=8 shiftwidth=8: */
/*-
 * Copyright (C) 2006-2008 Jason Evans <jasone@FreeBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 * This allocator implementation is designed to provide scalable performance
 * for multi-threaded programs on multi-processor systems.  The following
 * features are included for this purpose:
 *
 *   + Multiple arenas are used if there are multiple CPUs, which reduces lock
 *     contention and cache sloshing.
 *
 *   + Cache line sharing between arenas is avoided for internal data
 *     structures.
 *
 *   + Memory is managed in chunks and runs (chunks can be split into runs),
 *     rather than as individual pages.  This provides a constant-time
 *     mechanism for associating allocations with particular arenas.
 *
 * Allocation requests are rounded up to the nearest size class, and no record
 * of the original request size is maintained.  Allocations are broken into
 * categories according to size class.  Assuming runtime defaults, 4 kB pages
 * and a 16 byte quantum on a 32-bit system, the size classes in each category
 * are as follows:
 *
 *   |=====================================|
 *   | Category | Subcategory    |    Size |
 *   |=====================================|
 *   | Small    | Tiny           |       2 |
 *   |          |                |       4 |
 *   |          |                |       8 |
 *   |          |----------------+---------|
 *   |          | Quantum-spaced |      16 |
 *   |          |                |      32 |
 *   |          |                |      48 |
 *   |          |                |     ... |
 *   |          |                |     480 |
 *   |          |                |     496 |
 *   |          |                |     512 |
 *   |          |----------------+---------|
 *   |          | Sub-page       |    1 kB |
 *   |          |                |    2 kB |
 *   |=====================================|
 *   | Large                     |    4 kB |
 *   |                           |    8 kB |
 *   |                           |   12 kB |
 *   |                           |     ... |
 *   |                           | 1012 kB |
 *   |                           | 1016 kB |
 *   |                           | 1020 kB |
 *   |=====================================|
 *   | Huge                      |    1 MB |
 *   |                           |    2 MB |
 *   |                           |    3 MB |
 *   |                           |     ... |
 *   |=====================================|
 *
 * A different mechanism is used for each category:
 *
 *   Small : Each size class is segregated into its own set of runs.  Each run
 *           maintains a bitmap of which regions are free/allocated.
 *
 *   Large : Each allocation is backed by a dedicated run.  Metadata are stored
 *           in the associated arena chunk header maps.
 *
 *   Huge : Each allocation is backed by a dedicated contiguous set of chunks.
 *          Metadata are stored in a separate red-black tree.
 *
 *******************************************************************************
 */

/*
 * NOTE(mbelshe): Added these defines to fit within chromium build system.
 */
#define MOZ_MEMORY_WINDOWS
#define MOZ_MEMORY
#define DONT_OVERRIDE_LIBC

/*
 * MALLOC_PRODUCTION disables assertions and statistics gathering.  It also
 * defaults the A and J runtime options to off.  These settings are appropriate
 * for production systems.
 */
#ifndef MOZ_MEMORY_DEBUG
#  define	MALLOC_PRODUCTION
#endif

/*
 * Use only one arena by default.  Mozilla does not currently make extensive
 * use of concurrent allocation, so the increased fragmentation associated with
 * multiple arenas is not warranted.
 */
#define	MOZ_MEMORY_NARENAS_DEFAULT_ONE

/*
 * MALLOC_STATS enables statistics calculation, and is required for
 * jemalloc_stats().
 */
#define MALLOC_STATS

#ifndef MALLOC_PRODUCTION
   /*
    * MALLOC_DEBUG enables assertions and other sanity checks, and disables
    * inline functions.
    */
#  define MALLOC_DEBUG

   /* Memory filling (junk/zero). */
#  define MALLOC_FILL

   /* Allocation tracing. */
#  ifndef MOZ_MEMORY_WINDOWS
#    define MALLOC_UTRACE
#  endif

   /* Support optional abort() on OOM. */
#  define MALLOC_XMALLOC

   /* Support SYSV semantics. */
#  define MALLOC_SYSV
#endif

/*
 * MALLOC_VALIDATE causes malloc_usable_size() to perform some pointer
 * validation.  There are many possible errors that validation does not even
 * attempt to detect.
 */
#define MALLOC_VALIDATE

/* Embed no-op macros that support memory allocation tracking via valgrind. */
#ifdef MOZ_VALGRIND
#  define MALLOC_VALGRIND
#endif
#ifdef MALLOC_VALGRIND
#  include <valgrind/valgrind.h>
#else
#  define VALGRIND_MALLOCLIKE_BLOCK(addr, sizeB, rzB, is_zeroed)
#  define VALGRIND_FREELIKE_BLOCK(addr, rzB)
#endif

/*
 * MALLOC_BALANCE enables monitoring of arena lock contention and dynamically
 * re-balances arena load if exponentially averaged contention exceeds a
 * certain threshold.
 */
/* #define	MALLOC_BALANCE */

#if (!defined(MOZ_MEMORY_WINDOWS) && !defined(MOZ_MEMORY_DARWIN))
   /*
    * MALLOC_PAGEFILE causes all mmap()ed memory to be backed by temporary
    * files, so that if a chunk is mapped, it is guaranteed to be swappable.
    * This avoids asynchronous OOM failures that are due to VM over-commit.
    *
    * XXX OS X over-commits, so we should probably use mmap() instead of
    * vm_allocate(), so that MALLOC_PAGEFILE works.
    */
#define MALLOC_PAGEFILE
#endif

#ifdef MALLOC_PAGEFILE
/* Write size when initializing a page file. */
#  define MALLOC_PAGEFILE_WRITE_SIZE 512
#endif

#ifdef MOZ_MEMORY_LINUX
#define	_GNU_SOURCE /* For mremap(2). */
#define	issetugid() 0
#if 0 /* Enable in order to test decommit code on Linux. */
#  define MALLOC_DECOMMIT
#endif
#endif

#ifndef MOZ_MEMORY_WINCE
#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#endif
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef MOZ_MEMORY_WINDOWS
#ifndef MOZ_MEMORY_WINCE
//#include <cruntime.h>
//#include <internal.h>
#include <io.h>
#else
#include <cmnintrin.h>
#include <crtdefs.h>
#define SIZE_MAX UINT_MAX
#endif
#include <windows.h>

#pragma warning( disable: 4267 4996 4146 )

#define	false FALSE
#define	true TRUE
#define	inline __inline
#define	SIZE_T_MAX SIZE_MAX
#define	STDERR_FILENO 2
#define	PATH_MAX MAX_PATH
#define	vsnprintf _vsnprintf

#ifndef NO_TLS
static unsigned long tlsIndex = 0xffffffff;
#endif 

#define	__thread
#ifdef MOZ_MEMORY_WINCE
#define	_pthread_self() GetCurrentThreadId()
#else
#define	_pthread_self() __threadid()
#endif
#define	issetugid() 0

#ifndef MOZ_MEMORY_WINCE
/* use MSVC intrinsics */
#pragma intrinsic(_BitScanForward)
static __forceinline int
ffs(int x)
{
	unsigned long i;

	if (_BitScanForward(&i, x) != 0)
		return (i + 1);

	return (0);
}

/* Implement getenv without using malloc */
static char mozillaMallocOptionsBuf[64];

#define	getenv xgetenv
static char *
getenv(const char *name)
{

	if (GetEnvironmentVariableA(name, (LPSTR)&mozillaMallocOptionsBuf,
		    sizeof(mozillaMallocOptionsBuf)) > 0)
		return (mozillaMallocOptionsBuf);

	return (NULL);
}

#else /* WIN CE */

#define ENOMEM          12
#define EINVAL          22

static __forceinline int
ffs(int x)
{

	return 32 - _CountLeadingZeros((-x) & x);
}
#endif

typedef unsigned char uint8_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long long uintmax_t;
typedef long ssize_t;

#define	MALLOC_DECOMMIT
#endif

#ifndef MOZ_MEMORY_WINDOWS
#ifndef MOZ_MEMORY_SOLARIS
#include <sys/cdefs.h>
#endif
#ifndef __DECONST
#  define __DECONST(type, var)	((type)(uintptr_t)(const void *)(var))
#endif
#ifndef MOZ_MEMORY
__FBSDID("$FreeBSD: head/lib/libc/stdlib/malloc.c 180599 2008-07-18 19:35:44Z jasone $");
#include "libc_private.h"
#ifdef MALLOC_DEBUG
#  define _LOCK_DEBUG
#endif
#include "spinlock.h"
#include "namespace.h"
#endif
#include <sys/mman.h>
#ifndef MADV_FREE
#  define MADV_FREE	MADV_DONTNEED
#endif
#ifndef MAP_NOSYNC
#  define MAP_NOSYNC	0
#endif
#include <sys/param.h>
#ifndef MOZ_MEMORY
#include <sys/stddef.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#ifndef MOZ_MEMORY_SOLARIS
#include <sys/sysctl.h>
#endif
#include <sys/uio.h>
#ifndef MOZ_MEMORY
#include <sys/ktrace.h> /* Must come after several other sys/ includes. */

#include <machine/atomic.h>
#include <machine/cpufunc.h>
#include <machine/vmparam.h>
#endif

#include <errno.h>
#include <limits.h>
#ifndef SIZE_T_MAX
#  define SIZE_T_MAX	SIZE_MAX
#endif
#include <pthread.h>
#ifdef MOZ_MEMORY_DARWIN
#define _pthread_self pthread_self
#define _pthread_mutex_init pthread_mutex_init
#define _pthread_mutex_trylock pthread_mutex_trylock
#define _pthread_mutex_lock pthread_mutex_lock
#define _pthread_mutex_unlock pthread_mutex_unlock
#endif
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifndef MOZ_MEMORY_DARWIN
#include <strings.h>
#endif
#include <unistd.h>

#ifdef MOZ_MEMORY_DARWIN
#include <libkern/OSAtomic.h>
#include <mach/mach_error.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <malloc/malloc.h>
#endif

#ifndef MOZ_MEMORY
#include "un-namespace.h"
#endif

#endif

#include "jemalloc.h"

#undef bool
#define bool jemalloc_bool

#ifdef MOZ_MEMORY_DARWIN
static const bool __isthreaded = true;
#endif

#if defined(MOZ_MEMORY_SOLARIS) && defined(MAP_ALIGN) && !defined(JEMALLOC_NEVER_USES_MAP_ALIGN)
#define JEMALLOC_USES_MAP_ALIGN	 /* Required on Solaris 10. Might improve performance elsewhere. */
#endif

#if defined(MOZ_MEMORY_WINCE) && !defined(MOZ_MEMORY_WINCE6)
#define JEMALLOC_USES_MAP_ALIGN	 /* Required for Windows CE < 6 */
#endif

#define __DECONST(type, var) ((type)(uintptr_t)(const void *)(var))

#include "qr.h"
#include "ql.h"
#ifdef MOZ_MEMORY_WINDOWS
   /* MSVC++ does not support C99 variable-length arrays. */
#  define RB_NO_C99_VARARRAYS
#endif
#include "rb.h"

#ifdef MALLOC_DEBUG
   /* Disable inlining to make debugging easier. */
#ifdef inline
#undef inline
#endif

#  define inline
#endif

/* Size of stack-allocated buffer passed to strerror_r(). */
#define	STRERROR_BUF		64

/* Minimum alignment of allocations is 2^QUANTUM_2POW_MIN bytes. */
#  define QUANTUM_2POW_MIN      4
#ifdef MOZ_MEMORY_SIZEOF_PTR_2POW
#  define SIZEOF_PTR_2POW		MOZ_MEMORY_SIZEOF_PTR_2POW
#else
#  define SIZEOF_PTR_2POW       2
#endif
#define PIC
#ifndef MOZ_MEMORY_DARWIN
static const bool __isthreaded = true;
#else
#  define NO_TLS
#endif
#if 0
#ifdef __i386__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	2
#  define CPU_SPINWAIT		__asm__ volatile("pause")
#endif
#ifdef __ia64__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	3
#endif
#ifdef __alpha__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	3
#  define NO_TLS
#endif
#ifdef __sparc64__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	3
#  define NO_TLS
#endif
#ifdef __amd64__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	3
#  define CPU_SPINWAIT		__asm__ volatile("pause")
#endif
#ifdef __arm__
#  define QUANTUM_2POW_MIN	3
#  define SIZEOF_PTR_2POW	2
#  define NO_TLS
#endif
#ifdef __mips__
#  define QUANTUM_2POW_MIN	3
#  define SIZEOF_PTR_2POW	2
#  define NO_TLS
#endif
#ifdef __powerpc__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	2
#endif
#endif

#define	SIZEOF_PTR		(1U << SIZEOF_PTR_2POW)

/* sizeof(int) == (1U << SIZEOF_INT_2POW). */
#ifndef SIZEOF_INT_2POW
#  define SIZEOF_INT_2POW	2
#endif

/* We can't use TLS in non-PIC programs, since TLS relies on loader magic. */
#if (!defined(PIC) && !defined(NO_TLS))
#  define NO_TLS
#endif

#ifdef NO_TLS
   /* MALLOC_BALANCE requires TLS. */
#  ifdef MALLOC_BALANCE
#    undef MALLOC_BALANCE
#  endif
#endif

/*
 * Size and alignment of memory chunks that are allocated by the OS's virtual
 * memory system.
 */
#if defined(MOZ_MEMORY_WINCE) && !defined(MOZ_MEMORY_WINCE6)
#define	CHUNK_2POW_DEFAULT	21
#else
#define	CHUNK_2POW_DEFAULT	20
#endif
/* Maximum number of dirty pages per arena. */
#define	DIRTY_MAX_DEFAULT	(1U << 10)

/* Default reserve chunks. */
#define	RESERVE_MIN_2POW_DEFAULT	1
/*
 * Default range (in chunks) between reserve_min and reserve_max, in addition
 * to the mandatory one chunk per arena.
 */
#ifdef MALLOC_PAGEFILE
#  define RESERVE_RANGE_2POW_DEFAULT	5
#else
#  define RESERVE_RANGE_2POW_DEFAULT	0
#endif

/*
 * Maximum size of L1 cache line.  This is used to avoid cache line aliasing,
 * so over-estimates are okay (up to a point), but under-estimates will
 * negatively affect performance.
 */
#define	CACHELINE_2POW		6
#define	CACHELINE		((size_t)(1U << CACHELINE_2POW))

/* Smallest size class to support. */
#define	TINY_MIN_2POW		1

/*
 * Maximum size class that is a multiple of the quantum, but not (necessarily)
 * a power of 2.  Above this size, allocations are rounded up to the nearest
 * power of 2.
 */
#define	SMALL_MAX_2POW_DEFAULT	9
#define	SMALL_MAX_DEFAULT	(1U << SMALL_MAX_2POW_DEFAULT)

/*
 * RUN_MAX_OVRHD indicates maximum desired run header overhead.  Runs are sized
 * as small as possible such that this setting is still honored, without
 * violating other constraints.  The goal is to make runs as small as possible
 * without exceeding a per run external fragmentation threshold.
 *
 * We use binary fixed point math for overhead computations, where the binary
 * point is implicitly RUN_BFP bits to the left.
 *
 * Note that it is possible to set RUN_MAX_OVRHD low enough that it cannot be
 * honored for some/all object sizes, since there is one bit of header overhead
 * per object (plus a constant).  This constraint is relaxed (ignored) for runs
 * that are so small that the per-region overhead is greater than:
 *
 *   (RUN_MAX_OVRHD / (reg_size << (3+RUN_BFP))
 */
#define	RUN_BFP			12
/*                                    \/   Implicit binary fixed point. */
#define	RUN_MAX_OVRHD		0x0000003dU
#define	RUN_MAX_OVRHD_RELAX	0x00001800U

/* Put a cap on small object run size.  This overrides RUN_MAX_OVRHD. */
#define	RUN_MAX_SMALL_2POW	15
#define	RUN_MAX_SMALL		(1U << RUN_MAX_SMALL_2POW)

/*
 * Hyper-threaded CPUs may need a special instruction inside spin loops in
 * order to yield to another virtual CPU.  If no such instruction is defined
 * above, make CPU_SPINWAIT a no-op.
 */
#ifndef CPU_SPINWAIT
#  define CPU_SPINWAIT
#endif

/*
 * Adaptive spinning must eventually switch to blocking, in order to avoid the
 * potential for priority inversion deadlock.  Backing off past a certain point
 * can actually waste time.
 */
#define	SPIN_LIMIT_2POW		11

/*
 * Conversion from spinning to blocking is expensive; we use (1U <<
 * BLOCK_COST_2POW) to estimate how many more times costly blocking is than
 * worst-case spinning.
 */
#define	BLOCK_COST_2POW		4

#ifdef MALLOC_BALANCE
   /*
    * We use an exponential moving average to track recent lock contention,
    * where the size of the history window is N, and alpha=2/(N+1).
    *
    * Due to integer math rounding, very small values here can cause
    * substantial degradation in accuracy, thus making the moving average decay
    * faster than it would with precise calculation.
    */
#  define BALANCE_ALPHA_INV_2POW	9

   /*
    * Threshold value for the exponential moving contention average at which to
    * re-assign a thread.
    */
#  define BALANCE_THRESHOLD_DEFAULT	(1U << (SPIN_LIMIT_2POW-4))
#endif

/******************************************************************************/

/*
 * Mutexes based on spinlocks.  We can't use normal pthread spinlocks in all
 * places, because they require malloc()ed memory, which causes bootstrapping
 * issues in some cases.
 */
#if defined(MOZ_MEMORY_WINDOWS)
#define malloc_mutex_t CRITICAL_SECTION
#define malloc_spinlock_t CRITICAL_SECTION
#elif defined(MOZ_MEMORY_DARWIN)
typedef struct {
	OSSpinLock	lock;
} malloc_mutex_t;
typedef struct {
	OSSpinLock	lock;
} malloc_spinlock_t;
#elif defined(MOZ_MEMORY)
typedef pthread_mutex_t malloc_mutex_t;
typedef pthread_mutex_t malloc_spinlock_t;
#else
/* XXX these should #ifdef these for freebsd (and linux?) only */
typedef struct {
	spinlock_t	lock;
} malloc_mutex_t;
typedef malloc_spinlock_t malloc_mutex_t;
#endif

/* Set to true once the allocator has been initialized. */
static bool malloc_initialized = false;

#if defined(MOZ_MEMORY_WINDOWS)
/* No init lock for Windows. */
#elif defined(MOZ_MEMORY_DARWIN)
static malloc_mutex_t init_lock = {OS_SPINLOCK_INIT};
#elif defined(MOZ_MEMORY_LINUX)
static malloc_mutex_t init_lock = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
#elif defined(MOZ_MEMORY)
static malloc_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
#else
static malloc_mutex_t init_lock = {_SPINLOCK_INITIALIZER};
#endif

/******************************************************************************/
/*
 * Statistics data structures.
 */

#ifdef MALLOC_STATS

typedef struct malloc_bin_stats_s malloc_bin_stats_t;
struct malloc_bin_stats_s {
	/*
	 * Number of allocation requests that corresponded to the size of this
	 * bin.
	 */
	uint64_t	nrequests;

	/* Total number of runs created for this bin's size class. */
	uint64_t	nruns;

	/*
	 * Total number of runs reused by extracting them from the runs tree for
	 * this bin's size class.
	 */
	uint64_t	reruns;

	/* High-water mark for this bin. */
	unsigned long	highruns;

	/* Current number of runs in this bin. */
	unsigned long	curruns;
};

typedef struct arena_stats_s arena_stats_t;
struct arena_stats_s {
	/* Number of bytes currently mapped. */
	size_t		mapped;

	/*
	 * Total number of purge sweeps, total number of madvise calls made,
	 * and total pages purged in order to keep dirty unused memory under
	 * control.
	 */
	uint64_t	npurge;
	uint64_t	nmadvise;
	uint64_t	purged;
#ifdef MALLOC_DECOMMIT
	/*
	 * Total number of decommit/commit operations, and total number of
	 * pages decommitted.
	 */
	uint64_t	ndecommit;
	uint64_t	ncommit;
	uint64_t	decommitted;
#endif

	/* Per-size-category statistics. */
	size_t		allocated_small;
	uint64_t	nmalloc_small;
	uint64_t	ndalloc_small;

	size_t		allocated_large;
	uint64_t	nmalloc_large;
	uint64_t	ndalloc_large;

#ifdef MALLOC_BALANCE
	/* Number of times this arena reassigned a thread due to contention. */
	uint64_t	nbalance;
#endif
};

typedef struct chunk_stats_s chunk_stats_t;
struct chunk_stats_s {
	/* Number of chunks that were allocated. */
	uint64_t	nchunks;

	/* High-water mark for number of chunks allocated. */
	unsigned long	highchunks;

	/*
	 * Current number of chunks allocated.  This value isn't maintained for
	 * any other purpose, so keep track of it in order to be able to set
	 * highchunks.
	 */
	unsigned long	curchunks;
};

#endif /* #ifdef MALLOC_STATS */

/******************************************************************************/
/*
 * Extent data structures.
 */

/* Tree of extents. */
typedef struct extent_node_s extent_node_t;
struct extent_node_s {
	/* Linkage for the size/address-ordered tree. */
	rb_node(extent_node_t) link_szad;

	/* Linkage for the address-ordered tree. */
	rb_node(extent_node_t) link_ad;

	/* Pointer to the extent that this tree node is responsible for. */
	void	*addr;

	/* Total region size. */
	size_t	size;
};
typedef rb_tree(extent_node_t) extent_tree_t;

/******************************************************************************/
/*
 * Radix tree data structures.
 */

#ifdef MALLOC_VALIDATE
   /*
    * Size of each radix tree node (must be a power of 2).  This impacts tree
    * depth.
    */
#  if (SIZEOF_PTR == 4)
#    define MALLOC_RTREE_NODESIZE (1U << 14)
#  else
#    define MALLOC_RTREE_NODESIZE CACHELINE
#  endif

typedef struct malloc_rtree_s malloc_rtree_t;
struct malloc_rtree_s {
	malloc_spinlock_t	lock;
	void			**root;
	unsigned		height;
	unsigned		level2bits[1]; /* Dynamically sized. */
};
#endif

/******************************************************************************/
/*
 * Reserve data structures.
 */

/* Callback registration. */
typedef struct reserve_reg_s reserve_reg_t;
struct reserve_reg_s {
	/* Linkage for list of all registered callbacks. */
	ql_elm(reserve_reg_t)	link;

	/* Callback function pointer. */
	reserve_cb_t		*cb;

	/* Opaque application data pointer. */
	void			*ctx;

	/*
	 * Sequence number of condition notification most recently sent to this
	 * callback.
	 */
	uint64_t		seq;
};

/******************************************************************************/
/*
 * Arena data structures.
 */

typedef struct arena_s arena_t;
typedef struct arena_bin_s arena_bin_t;

/* Each element of the chunk map corresponds to one page within the chunk. */
typedef struct arena_chunk_map_s arena_chunk_map_t;
struct arena_chunk_map_s {
	/*
	 * Linkage for run trees.  There are two disjoint uses:
	 *
	 * 1) arena_t's runs_avail tree.
	 * 2) arena_run_t conceptually uses this linkage for in-use non-full
	 *    runs, rather than directly embedding linkage.
	 */
	rb_node(arena_chunk_map_t)	link;

	/*
	 * Run address (or size) and various flags are stored together.  The bit
	 * layout looks like (assuming 32-bit system):
	 *
	 *   ???????? ???????? ????---- --ckdzla
	 *
	 * ? : Unallocated: Run address for first/last pages, unset for internal
	 *                  pages.
	 *     Small: Run address.
	 *     Large: Run size for first page, unset for trailing pages.
	 * - : Unused.
	 * c : decommitted?
	 * k : key?
	 * d : dirty?
	 * z : zeroed?
	 * l : large?
	 * a : allocated?
	 *
	 * Following are example bit patterns for the three types of runs.
	 *
	 * r : run address
	 * s : run size
	 * x : don't care
	 * - : 0
	 * [cdzla] : bit set
	 *
	 *   Unallocated:
	 *     ssssssss ssssssss ssss---- --c-----
	 *     xxxxxxxx xxxxxxxx xxxx---- ----d---
	 *     ssssssss ssssssss ssss---- -----z--
	 *
	 *   Small:
	 *     rrrrrrrr rrrrrrrr rrrr---- -------a
	 *     rrrrrrrr rrrrrrrr rrrr---- -------a
	 *     rrrrrrrr rrrrrrrr rrrr---- -------a
	 *
	 *   Large:
	 *     ssssssss ssssssss ssss---- ------la
	 *     -------- -------- -------- ------la
	 *     -------- -------- -------- ------la
	 */
	size_t				bits;
#ifdef MALLOC_DECOMMIT
#define	CHUNK_MAP_DECOMMITTED	((size_t)0x20U)
#endif
#define	CHUNK_MAP_KEY		((size_t)0x10U)
#define	CHUNK_MAP_DIRTY		((size_t)0x08U)
#define	CHUNK_MAP_ZEROED	((size_t)0x04U)
#define	CHUNK_MAP_LARGE		((size_t)0x02U)
#define	CHUNK_MAP_ALLOCATED	((size_t)0x01U)
};
typedef rb_tree(arena_chunk_map_t) arena_avail_tree_t;
typedef rb_tree(arena_chunk_map_t) arena_run_tree_t;

/* Arena chunk header. */
typedef struct arena_chunk_s arena_chunk_t;
struct arena_chunk_s {
	/* Arena that owns the chunk. */
	arena_t		*arena;

	/* Linkage for the arena's chunks_dirty tree. */
	rb_node(arena_chunk_t) link_dirty;

	/* Number of dirty pages. */
	size_t		ndirty;

	/* Map of pages within chunk that keeps track of free/large/small. */
	arena_chunk_map_t map[1]; /* Dynamically sized. */
};
typedef rb_tree(arena_chunk_t) arena_chunk_tree_t;

typedef struct arena_run_s arena_run_t;
struct arena_run_s {
#ifdef MALLOC_DEBUG
	uint32_t	magic;
#  define ARENA_RUN_MAGIC 0x384adf93
#endif

	/* Bin this run is associated with. */
	arena_bin_t	*bin;

	/* Index of first element that might have a free region. */
	unsigned	regs_minelm;

	/* Number of free regions in run. */
	unsigned	nfree;

	/* Bitmask of in-use regions (0: in use, 1: free). */
	unsigned	regs_mask[1]; /* Dynamically sized. */
};

struct arena_bin_s {
	/*
	 * Current run being used to service allocations of this bin's size
	 * class.
	 */
	arena_run_t	*runcur;

	/*
	 * Tree of non-full runs.  This tree is used when looking for an
	 * existing run when runcur is no longer usable.  We choose the
	 * non-full run that is lowest in memory; this policy tends to keep
	 * objects packed well, and it can also help reduce the number of
	 * almost-empty chunks.
	 */
	arena_run_tree_t runs;

	/* Size of regions in a run for this bin's size class. */
	size_t		reg_size;

	/* Total size of a run for this bin's size class. */
	size_t		run_size;

	/* Total number of regions in a run for this bin's size class. */
	uint32_t	nregs;

	/* Number of elements in a run's regs_mask for this bin's size class. */
	uint32_t	regs_mask_nelms;

	/* Offset of first region in a run for this bin's size class. */
	uint32_t	reg0_offset;

#ifdef MALLOC_STATS
	/* Bin statistics. */
	malloc_bin_stats_t stats;
#endif
};

struct arena_s {
#ifdef MALLOC_DEBUG
	uint32_t		magic;
#  define ARENA_MAGIC 0x947d3d24
#endif

	/* All operations on this arena require that lock be locked. */
#ifdef MOZ_MEMORY
	malloc_spinlock_t	lock;
#else
	pthread_mutex_t		lock;
#endif

#ifdef MALLOC_STATS
	arena_stats_t		stats;
#endif

	/*
	 * Chunk allocation sequence number, used to detect races with other
	 * threads during chunk allocation, and then discard unnecessary chunks.
	 */
	uint64_t		chunk_seq;

	/* Tree of dirty-page-containing chunks this arena manages. */
	arena_chunk_tree_t	chunks_dirty;

	/*
	 * In order to avoid rapid chunk allocation/deallocation when an arena
	 * oscillates right on the cusp of needing a new chunk, cache the most
	 * recently freed chunk.  The spare is left in the arena's chunk trees
	 * until it is deleted.
	 *
	 * There is one spare chunk per arena, rather than one spare total, in
	 * order to avoid interactions between multiple threads that could make
	 * a single spare inadequate.
	 */
	arena_chunk_t		*spare;

	/*
	 * Current count of pages within unused runs that are potentially
	 * dirty, and for which madvise(... MADV_FREE) has not been called.  By
	 * tracking this, we can institute a limit on how much dirty unused
	 * memory is mapped for each arena.
	 */
	size_t			ndirty;

	/*
	 * Size/address-ordered tree of this arena's available runs.  This tree
	 * is used for first-best-fit run allocation.
	 */
	arena_avail_tree_t	runs_avail;

#ifdef MALLOC_BALANCE
	/*
	 * The arena load balancing machinery needs to keep track of how much
	 * lock contention there is.  This value is exponentially averaged.
	 */
	uint32_t		contention;
#endif

	/*
	 * bins is used to store rings of free regions of the following sizes,
	 * assuming a 16-byte quantum, 4kB pagesize, and default MALLOC_OPTIONS.
	 *
	 *   bins[i] | size |
	 *   --------+------+
	 *        0  |    2 |
	 *        1  |    4 |
	 *        2  |    8 |
	 *   --------+------+
	 *        3  |   16 |
	 *        4  |   32 |
	 *        5  |   48 |
	 *        6  |   64 |
	 *           :      :
	 *           :      :
	 *       33  |  496 |
	 *       34  |  512 |
	 *   --------+------+
	 *       35  | 1024 |
	 *       36  | 2048 |
	 *   --------+------+
	 */
	arena_bin_t		bins[1]; /* Dynamically sized. */
};

/******************************************************************************/
/*
 * Data.
 */

/* Number of CPUs. */
static unsigned		ncpus;

/* VM page size. */
static size_t		pagesize;
static size_t		pagesize_mask;
static size_t		pagesize_2pow;

/* Various bin-related settings. */
static size_t		bin_maxclass; /* Max size class for bins. */
static unsigned		ntbins; /* Number of (2^n)-spaced tiny bins. */
static unsigned		nqbins; /* Number of quantum-spaced bins. */
static unsigned		nsbins; /* Number of (2^n)-spaced sub-page bins. */
static size_t		small_min;
static size_t		small_max;

/* Various quantum-related settings. */
static size_t		quantum;
static size_t		quantum_mask; /* (quantum - 1). */

/* Various chunk-related settings. */
static size_t		chunksize;
static size_t		chunksize_mask; /* (chunksize - 1). */
static size_t		chunk_npages;
static size_t		arena_chunk_header_npages;
static size_t		arena_maxclass; /* Max size class for arenas. */

/********/
/*
 * Chunks.
 */

#ifdef MALLOC_VALIDATE
static malloc_rtree_t *chunk_rtree;
#endif

/* Protects chunk-related data structures. */
static malloc_mutex_t	huge_mtx;

/* Tree of chunks that are stand-alone huge allocations. */
static extent_tree_t	huge;

#ifdef MALLOC_STATS
/* Huge allocation statistics. */
static uint64_t		huge_nmalloc;
static uint64_t		huge_ndalloc;
static size_t		huge_allocated;
#endif

/****************/
/*
 * Memory reserve.
 */

#ifdef MALLOC_PAGEFILE
static char		pagefile_templ[PATH_MAX];
#endif

/* Protects reserve-related data structures. */
static malloc_mutex_t	reserve_mtx;

/*
 * Bounds on acceptable reserve size, and current reserve size.  Reserve
 * depletion may cause (reserve_cur < reserve_min).
 */
static size_t		reserve_min;
static size_t		reserve_cur;
static size_t		reserve_max;

/* List of registered callbacks. */
static ql_head(reserve_reg_t) reserve_regs;

/*
 * Condition notification sequence number, used to determine whether all
 * registered callbacks have been notified of the most current condition.
 */
static uint64_t		reserve_seq;

/*
 * Trees of chunks currently in the memory reserve.  Depending on function,
 * different tree orderings are needed, which is why there are two trees with
 * the same contents.
 */
static extent_tree_t	reserve_chunks_szad;
static extent_tree_t	reserve_chunks_ad;

/****************************/
/*
 * base (internal allocation).
 */

/*
 * Current pages that are being used for internal memory allocations.  These
 * pages are carved up in cacheline-size quanta, so that there is no chance of
 * false cache line sharing.
 */
static void		*base_pages;
static void		*base_next_addr;
#ifdef MALLOC_DECOMMIT
static void		*base_next_decommitted;
#endif
static void		*base_past_addr; /* Addr immediately past base_pages. */
static extent_node_t	*base_nodes;
static reserve_reg_t	*base_reserve_regs;
static malloc_mutex_t	base_mtx;
#ifdef MALLOC_STATS
static size_t		base_mapped;
#endif

/********/
/*
 * Arenas.
 */

/*
 * Arenas that are used to service external requests.  Not all elements of the
 * arenas array are necessarily used; arenas are created lazily as needed.
 */
static arena_t		**arenas;
static unsigned		narenas;
static unsigned		narenas_2pow;
#ifndef NO_TLS
#  ifdef MALLOC_BALANCE
static unsigned		narenas_2pow;
#  else
static unsigned		next_arena;
#  endif
#endif
#ifdef MOZ_MEMORY
static malloc_spinlock_t arenas_lock; /* Protects arenas initialization. */
#else
static pthread_mutex_t arenas_lock; /* Protects arenas initialization. */
#endif

#ifndef NO_TLS
/*
 * Map of pthread_self() --> arenas[???], used for selecting an arena to use
 * for allocations.
 */
#ifndef MOZ_MEMORY_WINDOWS
static __thread arena_t	*arenas_map;
#endif
#endif

#ifdef MALLOC_STATS
/* Chunk statistics. */
static chunk_stats_t	stats_chunks;
#endif

/*******************************/
/*
 * Runtime configuration options.
 */
const char	*_malloc_options;

#ifndef MALLOC_PRODUCTION
static bool	opt_abort = true;
#ifdef MALLOC_FILL
static bool	opt_junk = true;
#endif
#else
static bool	opt_abort = false;
#ifdef MALLOC_FILL
static bool	opt_junk = false;
#endif
#endif
static size_t	opt_dirty_max = DIRTY_MAX_DEFAULT;
#ifdef MALLOC_BALANCE
static uint64_t	opt_balance_threshold = BALANCE_THRESHOLD_DEFAULT;
#endif
static bool	opt_print_stats = false;
static size_t	opt_quantum_2pow = QUANTUM_2POW_MIN;
static size_t	opt_small_max_2pow = SMALL_MAX_2POW_DEFAULT;
static size_t	opt_chunk_2pow = CHUNK_2POW_DEFAULT;
static int	opt_reserve_min_lshift = 0;
static int	opt_reserve_range_lshift = 0;
#ifdef MALLOC_PAGEFILE
static bool	opt_pagefile = false;
#endif
#ifdef MALLOC_UTRACE
static bool	opt_utrace = false;
#endif
#ifdef MALLOC_SYSV
static bool	opt_sysv = false;
#endif
#ifdef MALLOC_XMALLOC
static bool	opt_xmalloc = false;
#endif
#ifdef MALLOC_FILL
static bool	opt_zero = false;
#endif
static int	opt_narenas_lshift = 0;

#ifdef MALLOC_UTRACE
typedef struct {
	void	*p;
	size_t	s;
	void	*r;
} malloc_utrace_t;

#define	UTRACE(a, b, c)							\
	if (opt_utrace) {						\
		malloc_utrace_t ut;					\
		ut.p = (a);						\
		ut.s = (b);						\
		ut.r = (c);						\
		utrace(&ut, sizeof(ut));				\
	}
#else
#define	UTRACE(a, b, c)
#endif

/******************************************************************************/
/*
 * Begin function prototypes for non-inline static functions.
 */

static char	*umax2s(uintmax_t x, char *s);
static bool	malloc_mutex_init(malloc_mutex_t *mutex);
static bool	malloc_spin_init(malloc_spinlock_t *lock);
static void	wrtmessage(const char *p1, const char *p2, const char *p3,
		const char *p4);
#ifdef MALLOC_STATS
#ifdef MOZ_MEMORY_DARWIN
/* Avoid namespace collision with OS X's malloc APIs. */
#define malloc_printf moz_malloc_printf
#endif
static void	malloc_printf(const char *format, ...);
#endif
static bool	base_pages_alloc_mmap(size_t minsize);
static bool	base_pages_alloc(size_t minsize);
static void	*base_alloc(size_t size);
static void	*base_calloc(size_t number, size_t size);
static extent_node_t *base_node_alloc(void);
static void	base_node_dealloc(extent_node_t *node);
static reserve_reg_t *base_reserve_reg_alloc(void);
static void	base_reserve_reg_dealloc(reserve_reg_t *reg);
#ifdef MALLOC_STATS
static void	stats_print(arena_t *arena);
#endif
static void	*pages_map(void *addr, size_t size, int pfd);
static void	pages_unmap(void *addr, size_t size);
static void	*chunk_alloc_mmap(size_t size, bool pagefile);
#ifdef MALLOC_PAGEFILE
static int	pagefile_init(size_t size);
static void	pagefile_close(int pfd);
#endif
static void	*chunk_recycle_reserve(size_t size, bool zero);
static void	*chunk_alloc(size_t size, bool zero, bool pagefile);
static extent_node_t *chunk_dealloc_reserve(void *chunk, size_t size);
static void	chunk_dealloc_mmap(void *chunk, size_t size);
static void	chunk_dealloc(void *chunk, size_t size);
#ifndef NO_TLS
static arena_t	*choose_arena_hard(void);
#endif
static void	arena_run_split(arena_t *arena, arena_run_t *run, size_t size,
    bool large, bool zero);
static void arena_chunk_init(arena_t *arena, arena_chunk_t *chunk);
static void	arena_chunk_dealloc(arena_t *arena, arena_chunk_t *chunk);
static arena_run_t *arena_run_alloc(arena_t *arena, arena_bin_t *bin,
    size_t size, bool large, bool zero);
static void	arena_purge(arena_t *arena);
static void	arena_run_dalloc(arena_t *arena, arena_run_t *run, bool dirty);
static void	arena_run_trim_head(arena_t *arena, arena_chunk_t *chunk,
    arena_run_t *run, size_t oldsize, size_t newsize);
static void	arena_run_trim_tail(arena_t *arena, arena_chunk_t *chunk,
    arena_run_t *run, size_t oldsize, size_t newsize, bool dirty);
static arena_run_t *arena_bin_nonfull_run_get(arena_t *arena, arena_bin_t *bin);
static void *arena_bin_malloc_hard(arena_t *arena, arena_bin_t *bin);
static size_t arena_bin_run_size_calc(arena_bin_t *bin, size_t min_run_size);
#ifdef MALLOC_BALANCE
static void	arena_lock_balance_hard(arena_t *arena);
#endif
static void	*arena_malloc_large(arena_t *arena, size_t size, bool zero);
static void	*arena_palloc(arena_t *arena, size_t alignment, size_t size,
    size_t alloc_size);
static size_t	arena_salloc(const void *ptr);
static void	arena_dalloc_large(arena_t *arena, arena_chunk_t *chunk,
    void *ptr);
static void	arena_ralloc_large_shrink(arena_t *arena, arena_chunk_t *chunk,
    void *ptr, size_t size, size_t oldsize);
static bool	arena_ralloc_large_grow(arena_t *arena, arena_chunk_t *chunk,
    void *ptr, size_t size, size_t oldsize);
static bool	arena_ralloc_large(void *ptr, size_t size, size_t oldsize);
static void	*arena_ralloc(void *ptr, size_t size, size_t oldsize);
static bool	arena_new(arena_t *arena);
static arena_t	*arenas_extend(unsigned ind);
static void	*huge_malloc(size_t size, bool zero);
static void	*huge_palloc(size_t alignment, size_t size);
static void	*huge_ralloc(void *ptr, size_t size, size_t oldsize);
static void	huge_dalloc(void *ptr);
static void	malloc_print_stats(void);
#ifndef MOZ_MEMORY_WINDOWS
static
#endif
bool		malloc_init_hard(void);
static void	reserve_shrink(void);
static uint64_t	reserve_notify(reserve_cnd_t cnd, size_t size, uint64_t seq);
static uint64_t	reserve_crit(size_t size, const char *fname, uint64_t seq);
static void	reserve_fail(size_t size, const char *fname);

void		_malloc_prefork(void);
void		_malloc_postfork(void);

/*
 * End function prototypes.
 */
/******************************************************************************/

/*
 * umax2s() provides minimal integer printing functionality, which is
 * especially useful for situations where allocation in vsnprintf() calls would
 * potentially cause deadlock.
 */
#define	UMAX2S_BUFSIZE	21
static char *
umax2s(uintmax_t x, char *s)
{
	unsigned i;

	i = UMAX2S_BUFSIZE - 1;
	s[i] = '\0';
	do {
		i--;
		s[i] = "0123456789"[x % 10];
		x /= 10;
	} while (x > 0);

	return (&s[i]);
}

static void
wrtmessage(const char *p1, const char *p2, const char *p3, const char *p4)
{
#ifdef MOZ_MEMORY_WINCE
       wchar_t buf[1024];
#define WRT_PRINT(s) \
       MultiByteToWideChar(CP_ACP, 0, s, -1, buf, 1024); \
       OutputDebugStringW(buf)

       WRT_PRINT(p1);
       WRT_PRINT(p2);
       WRT_PRINT(p3);
       WRT_PRINT(p4);
#else
#if defined(MOZ_MEMORY) && !defined(MOZ_MEMORY_WINDOWS)
#define	_write	write
#endif
	_write(STDERR_FILENO, p1, (unsigned int) strlen(p1));
	_write(STDERR_FILENO, p2, (unsigned int) strlen(p2));
	_write(STDERR_FILENO, p3, (unsigned int) strlen(p3));
	_write(STDERR_FILENO, p4, (unsigned int) strlen(p4));
#endif

}

#define _malloc_message malloc_message

void	(*_malloc_message)(const char *p1, const char *p2, const char *p3,
	    const char *p4) = wrtmessage;

#ifdef MALLOC_DEBUG
#  define assert(e) do {						\
	if (!(e)) {							\
		char line_buf[UMAX2S_BUFSIZE];				\
		_malloc_message(__FILE__, ":", umax2s(__LINE__,		\
		    line_buf), ": Failed assertion: ");			\
		_malloc_message("\"", #e, "\"\n", "");			\
		abort();						\
	}								\
} while (0)
#else
#define assert(e)
#endif

/******************************************************************************/
/*
 * Begin mutex.  We can't use normal pthread mutexes in all places, because
 * they require malloc()ed memory, which causes bootstrapping issues in some
 * cases.
 */

static bool
malloc_mutex_init(malloc_mutex_t *mutex)
{
#if defined(MOZ_MEMORY_WINCE)
	InitializeCriticalSection(mutex);
#elif defined(MOZ_MEMORY_WINDOWS)
  // XXXMB
	//if (__isthreaded)
	//	if (! __crtInitCritSecAndSpinCount(mutex, _CRT_SPINCOUNT))
	//		return (true);
  if (!InitializeCriticalSectionAndSpinCount(mutex, 4000))
    return true;
#elif defined(MOZ_MEMORY_DARWIN)
	mutex->lock = OS_SPINLOCK_INIT;
#elif defined(MOZ_MEMORY_LINUX)
	pthread_mutexattr_t attr;
	if (pthread_mutexattr_init(&attr) != 0)
		return (true);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	if (pthread_mutex_init(mutex, &attr) != 0) {
		pthread_mutexattr_destroy(&attr);
		return (true);
	}
	pthread_mutexattr_destroy(&attr);
#elif defined(MOZ_MEMORY)
	if (pthread_mutex_init(mutex, NULL) != 0)
		return (true);
#else
	static const spinlock_t lock = _SPINLOCK_INITIALIZER;

	mutex->lock = lock;
#endif
	return (false);
}

static inline void
malloc_mutex_lock(malloc_mutex_t *mutex)
{

#if defined(MOZ_MEMORY_WINDOWS)
	EnterCriticalSection(mutex);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockLock(&mutex->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_lock(mutex);
#else
	if (__isthreaded)
		_SPINLOCK(&mutex->lock);
#endif
}

static inline void
malloc_mutex_unlock(malloc_mutex_t *mutex)
{

#if defined(MOZ_MEMORY_WINDOWS)
	LeaveCriticalSection(mutex);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockUnlock(&mutex->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_unlock(mutex);
#else
	if (__isthreaded)
		_SPINUNLOCK(&mutex->lock);
#endif
}

static bool
malloc_spin_init(malloc_spinlock_t *lock)
{
#if defined(MOZ_MEMORY_WINCE)
	InitializeCriticalSection(lock);
#elif defined(MOZ_MEMORY_WINDOWS)
  // XXXMB
	//if (__isthreaded)
	//	if (! __crtInitCritSecAndSpinCount(lock, _CRT_SPINCOUNT))
	//		return (true);
#elif defined(MOZ_MEMORY_DARWIN)
	lock->lock = OS_SPINLOCK_INIT;
#elif defined(MOZ_MEMORY_LINUX)
	pthread_mutexattr_t attr;
	if (pthread_mutexattr_init(&attr) != 0)
		return (true);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	if (pthread_mutex_init(lock, &attr) != 0) {
		pthread_mutexattr_destroy(&attr);
		return (true);
	}
	pthread_mutexattr_destroy(&attr);
#elif defined(MOZ_MEMORY)
	if (pthread_mutex_init(lock, NULL) != 0)
		return (true);
#else
	lock->lock = _SPINLOCK_INITIALIZER;
#endif
	return (false);
}

static inline void
malloc_spin_lock(malloc_spinlock_t *lock)
{

#if defined(MOZ_MEMORY_WINDOWS)
	EnterCriticalSection(lock);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockLock(&lock->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_lock(lock);
#else
	if (__isthreaded)
		_SPINLOCK(&lock->lock);
#endif
}

static inline void
malloc_spin_unlock(malloc_spinlock_t *lock)
{
#if defined(MOZ_MEMORY_WINDOWS)
	LeaveCriticalSection(lock);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockUnlock(&lock->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_unlock(lock);
#else
	if (__isthreaded)
		_SPINUNLOCK(&lock->lock);
#endif
}

/*
 * End mutex.
 */
/******************************************************************************/
/*
 * Begin spin lock.  Spin locks here are actually adaptive mutexes that block
 * after a period of spinning, because unbounded spinning would allow for
 * priority inversion.
 */

#if defined(MOZ_MEMORY) && !defined(MOZ_MEMORY_DARWIN)
#  define	malloc_spin_init	malloc_mutex_init
#  define	malloc_spin_lock	malloc_mutex_lock
#  define	malloc_spin_unlock	malloc_mutex_unlock
#endif

#ifndef MOZ_MEMORY
/*
 * We use an unpublished interface to initialize pthread mutexes with an
 * allocation callback, in order to avoid infinite recursion.
 */
int	_pthread_mutex_init_calloc_cb(pthread_mutex_t *mutex,
    void *(calloc_cb)(size_t, size_t));

__weak_reference(_pthread_mutex_init_calloc_cb_stub,
    _pthread_mutex_init_calloc_cb);

int
_pthread_mutex_init_calloc_cb_stub(pthread_mutex_t *mutex,
    void *(calloc_cb)(size_t, size_t))
{

	return (0);
}

static bool
malloc_spin_init(pthread_mutex_t *lock)
{

	if (_pthread_mutex_init_calloc_cb(lock, base_calloc) != 0)
		return (true);

	return (false);
}

static inline unsigned
malloc_spin_lock(pthread_mutex_t *lock)
{
	unsigned ret = 0;

	if (__isthreaded) {
		if (_pthread_mutex_trylock(lock) != 0) {
			unsigned i;
			volatile unsigned j;

			/* Exponentially back off. */
			for (i = 1; i <= SPIN_LIMIT_2POW; i++) {
				for (j = 0; j < (1U << i); j++)
					ret++;

				CPU_SPINWAIT;
				if (_pthread_mutex_trylock(lock) == 0)
					return (ret);
			}

			/*
			 * Spinning failed.  Block until the lock becomes
			 * available, in order to avoid indefinite priority
			 * inversion.
			 */
			_pthread_mutex_lock(lock);
			assert((ret << BLOCK_COST_2POW) != 0);
			return (ret << BLOCK_COST_2POW);
		}
	}

	return (ret);
}

static inline void
malloc_spin_unlock(pthread_mutex_t *lock)
{

	if (__isthreaded)
		_pthread_mutex_unlock(lock);
}
#endif

/*
 * End spin lock.
 */
/******************************************************************************/
/*
 * Begin Utility functions/macros.
 */

/* Return the chunk address for allocation address a. */
#define	CHUNK_ADDR2BASE(a)						\
	((void *)((uintptr_t)(a) & ~chunksize_mask))

/* Return the chunk offset of address a. */
#define	CHUNK_ADDR2OFFSET(a)						\
	((size_t)((uintptr_t)(a) & chunksize_mask))

/* Return the smallest chunk multiple that is >= s. */
#define	CHUNK_CEILING(s)						\
	(((s) + chunksize_mask) & ~chunksize_mask)

/* Return the smallest cacheline multiple that is >= s. */
#define	CACHELINE_CEILING(s)						\
	(((s) + (CACHELINE - 1)) & ~(CACHELINE - 1))

/* Return the smallest quantum multiple that is >= a. */
#define	QUANTUM_CEILING(a)						\
	(((a) + quantum_mask) & ~quantum_mask)

/* Return the smallest pagesize multiple that is >= s. */
#define	PAGE_CEILING(s)							\
	(((s) + pagesize_mask) & ~pagesize_mask)

/* Compute the smallest power of 2 that is >= x. */
static inline size_t
pow2_ceil(size_t x)
{

	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
#if (SIZEOF_PTR == 8)
	x |= x >> 32;
#endif
	x++;
	return (x);
}

#ifdef MALLOC_BALANCE
/*
 * Use a simple linear congruential pseudo-random number generator:
 *
 *   prn(y) = (a*x + c) % m
 *
 * where the following constants ensure maximal period:
 *
 *   a == Odd number (relatively prime to 2^n), and (a-1) is a multiple of 4.
 *   c == Odd number (relatively prime to 2^n).
 *   m == 2^32
 *
 * See Knuth's TAOCP 3rd Ed., Vol. 2, pg. 17 for details on these constraints.
 *
 * This choice of m has the disadvantage that the quality of the bits is
 * proportional to bit position.  For example. the lowest bit has a cycle of 2,
 * the next has a cycle of 4, etc.  For this reason, we prefer to use the upper
 * bits.
 */
#  define PRN_DEFINE(suffix, var, a, c)					\
static inline void							\
sprn_##suffix(uint32_t seed)						\
{									\
	var = seed;							\
}									\
									\
static inline uint32_t							\
prn_##suffix(uint32_t lg_range)						\
{									\
	uint32_t ret, x;						\
									\
	assert(lg_range > 0);						\
	assert(lg_range <= 32);						\
									\
	x = (var * (a)) + (c);						\
	var = x;							\
	ret = x >> (32 - lg_range);					\
									\
	return (ret);							\
}
#  define SPRN(suffix, seed)	sprn_##suffix(seed)
#  define PRN(suffix, lg_range)	prn_##suffix(lg_range)
#endif

#ifdef MALLOC_BALANCE
/* Define the PRNG used for arena assignment. */
static __thread uint32_t balance_x;
PRN_DEFINE(balance, balance_x, 1297, 1301)
#endif

#ifdef MALLOC_UTRACE
static int
utrace(const void *addr, size_t len)
{
	malloc_utrace_t *ut = (malloc_utrace_t *)addr;

	assert(len == sizeof(malloc_utrace_t));

	if (ut->p == NULL && ut->s == 0 && ut->r == NULL)
		malloc_printf("%d x USER malloc_init()\n", getpid());
	else if (ut->p == NULL && ut->r != NULL) {
		malloc_printf("%d x USER %p = malloc(%zu)\n", getpid(), ut->r,
		    ut->s);
	} else if (ut->p != NULL && ut->r != NULL) {
		malloc_printf("%d x USER %p = realloc(%p, %zu)\n", getpid(),
		    ut->r, ut->p, ut->s);
	} else
		malloc_printf("%d x USER free(%p)\n", getpid(), ut->p);

	return (0);
}
#endif

static inline const char *
_getprogname(void)
{

	return ("<jemalloc>");
}

#ifdef MALLOC_STATS
/*
 * Print to stderr in such a way as to (hopefully) avoid memory allocation.
 */
static void
malloc_printf(const char *format, ...)
{
#ifndef WINCE
	char buf[4096];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	_malloc_message(buf, "", "", "");
#endif
}
#endif

/******************************************************************************/

#ifdef MALLOC_DECOMMIT
static inline void
pages_decommit(void *addr, size_t size)
{

#ifdef MOZ_MEMORY_WINDOWS
	VirtualFree(addr, size, MEM_DECOMMIT);
#else
	if (mmap(addr, size, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1,
	    0) == MAP_FAILED)
		abort();
#endif
}

static inline void
pages_commit(void *addr, size_t size)
{

#  ifdef MOZ_MEMORY_WINDOWS
	VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
#  else
	if (mmap(addr, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE |
	    MAP_ANON, -1, 0) == MAP_FAILED)
		abort();
#  endif
}
#endif

static bool
base_pages_alloc_mmap(size_t minsize)
{
	bool ret;
	size_t csize;
#ifdef MALLOC_DECOMMIT
	size_t pminsize;
#endif
	int pfd;

	assert(minsize != 0);
	csize = CHUNK_CEILING(minsize);
#ifdef MALLOC_PAGEFILE
	if (opt_pagefile) {
		pfd = pagefile_init(csize);
		if (pfd == -1)
			return (true);
	} else
#endif
		pfd = -1;
	base_pages = pages_map(NULL, csize, pfd);
	if (base_pages == NULL) {
		ret = true;
		goto RETURN;
	}
	base_next_addr = base_pages;
	base_past_addr = (void *)((uintptr_t)base_pages + csize);
#ifdef MALLOC_DECOMMIT
	/*
	 * Leave enough pages for minsize committed, since otherwise they would
	 * have to be immediately recommitted.
	 */
	pminsize = PAGE_CEILING(minsize);
	base_next_decommitted = (void *)((uintptr_t)base_pages + pminsize);
	if (pminsize < csize)
		pages_decommit(base_next_decommitted, csize - pminsize);
#endif
#ifdef MALLOC_STATS
	base_mapped += csize;
#endif

	ret = false;
RETURN:
#ifdef MALLOC_PAGEFILE
	if (pfd != -1)
		pagefile_close(pfd);
#endif
	return (false);
}

static bool
base_pages_alloc(size_t minsize)
{

	if (base_pages_alloc_mmap(minsize) == false)
		return (false);

	return (true);
}

static void *
base_alloc(size_t size)
{
	void *ret;
	size_t csize;

	/* Round size up to nearest multiple of the cacheline size. */
	csize = CACHELINE_CEILING(size);

	malloc_mutex_lock(&base_mtx);
	/* Make sure there's enough space for the allocation. */
	if ((uintptr_t)base_next_addr + csize > (uintptr_t)base_past_addr) {
		if (base_pages_alloc(csize)) {
			malloc_mutex_unlock(&base_mtx);
			return (NULL);
		}
	}
	/* Allocate. */
	ret = base_next_addr;
	base_next_addr = (void *)((uintptr_t)base_next_addr + csize);
#ifdef MALLOC_DECOMMIT
	/* Make sure enough pages are committed for the new allocation. */
	if ((uintptr_t)base_next_addr > (uintptr_t)base_next_decommitted) {
		void *pbase_next_addr =
		    (void *)(PAGE_CEILING((uintptr_t)base_next_addr));

		pages_commit(base_next_decommitted, (uintptr_t)pbase_next_addr -
		    (uintptr_t)base_next_decommitted);
		base_next_decommitted = pbase_next_addr;
	}
#endif
	malloc_mutex_unlock(&base_mtx);
	VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, false);

	return (ret);
}

static void *
base_calloc(size_t number, size_t size)
{
	void *ret;

	ret = base_alloc(number * size);
#ifdef MALLOC_VALGRIND
	if (ret != NULL) {
		VALGRIND_FREELIKE_BLOCK(ret, 0);
		VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, true);
	}
#endif
	memset(ret, 0, number * size);

	return (ret);
}

static extent_node_t *
base_node_alloc(void)
{
	extent_node_t *ret;

	malloc_mutex_lock(&base_mtx);
	if (base_nodes != NULL) {
		ret = base_nodes;
		base_nodes = *(extent_node_t **)ret;
		VALGRIND_FREELIKE_BLOCK(ret, 0);
		VALGRIND_MALLOCLIKE_BLOCK(ret, sizeof(extent_node_t), 0, false);
		malloc_mutex_unlock(&base_mtx);
	} else {
		malloc_mutex_unlock(&base_mtx);
		ret = (extent_node_t *)base_alloc(sizeof(extent_node_t));
	}

	return (ret);
}

static void
base_node_dealloc(extent_node_t *node)
{

	malloc_mutex_lock(&base_mtx);
	VALGRIND_FREELIKE_BLOCK(node, 0);
	VALGRIND_MALLOCLIKE_BLOCK(node, sizeof(extent_node_t *), 0, false);
	*(extent_node_t **)node = base_nodes;
	base_nodes = node;
	malloc_mutex_unlock(&base_mtx);
}

static reserve_reg_t *
base_reserve_reg_alloc(void)
{
	reserve_reg_t *ret;

	malloc_mutex_lock(&base_mtx);
	if (base_reserve_regs != NULL) {
		ret = base_reserve_regs;
		base_reserve_regs = *(reserve_reg_t **)ret;
		VALGRIND_FREELIKE_BLOCK(ret, 0);
		VALGRIND_MALLOCLIKE_BLOCK(ret, sizeof(reserve_reg_t), 0, false);
		malloc_mutex_unlock(&base_mtx);
	} else {
		malloc_mutex_unlock(&base_mtx);
		ret = (reserve_reg_t *)base_alloc(sizeof(reserve_reg_t));
	}

	return (ret);
}

static void
base_reserve_reg_dealloc(reserve_reg_t *reg)
{

	malloc_mutex_lock(&base_mtx);
	VALGRIND_FREELIKE_BLOCK(reg, 0);
	VALGRIND_MALLOCLIKE_BLOCK(reg, sizeof(reserve_reg_t *), 0, false);
	*(reserve_reg_t **)reg = base_reserve_regs;
	base_reserve_regs = reg;
	malloc_mutex_unlock(&base_mtx);
}

/******************************************************************************/

#ifdef MALLOC_STATS
static void
stats_print(arena_t *arena)
{
	unsigned i, gap_start;

#ifdef MOZ_MEMORY_WINDOWS
	malloc_printf("dirty: %Iu page%s dirty, %I64u sweep%s,"
	    " %I64u madvise%s, %I64u page%s purged\n",
	    arena->ndirty, arena->ndirty == 1 ? "" : "s",
	    arena->stats.npurge, arena->stats.npurge == 1 ? "" : "s",
	    arena->stats.nmadvise, arena->stats.nmadvise == 1 ? "" : "s",
	    arena->stats.purged, arena->stats.purged == 1 ? "" : "s");
#  ifdef MALLOC_DECOMMIT
	malloc_printf("decommit: %I64u decommit%s, %I64u commit%s,"
	    " %I64u page%s decommitted\n",
	    arena->stats.ndecommit, (arena->stats.ndecommit == 1) ? "" : "s",
	    arena->stats.ncommit, (arena->stats.ncommit == 1) ? "" : "s",
	    arena->stats.decommitted,
	    (arena->stats.decommitted == 1) ? "" : "s");
#  endif

	malloc_printf("            allocated      nmalloc      ndalloc\n");
	malloc_printf("small:   %12Iu %12I64u %12I64u\n",
	    arena->stats.allocated_small, arena->stats.nmalloc_small,
	    arena->stats.ndalloc_small);
	malloc_printf("large:   %12Iu %12I64u %12I64u\n",
	    arena->stats.allocated_large, arena->stats.nmalloc_large,
	    arena->stats.ndalloc_large);
	malloc_printf("total:   %12Iu %12I64u %12I64u\n",
	    arena->stats.allocated_small + arena->stats.allocated_large,
	    arena->stats.nmalloc_small + arena->stats.nmalloc_large,
	    arena->stats.ndalloc_small + arena->stats.ndalloc_large);
	malloc_printf("mapped:  %12Iu\n", arena->stats.mapped);
#else
	malloc_printf("dirty: %zu page%s dirty, %llu sweep%s,"
	    " %llu madvise%s, %llu page%s purged\n",
	    arena->ndirty, arena->ndirty == 1 ? "" : "s",
	    arena->stats.npurge, arena->stats.npurge == 1 ? "" : "s",
	    arena->stats.nmadvise, arena->stats.nmadvise == 1 ? "" : "s",
	    arena->stats.purged, arena->stats.purged == 1 ? "" : "s");
#  ifdef MALLOC_DECOMMIT
	malloc_printf("decommit: %llu decommit%s, %llu commit%s,"
	    " %llu page%s decommitted\n",
	    arena->stats.ndecommit, (arena->stats.ndecommit == 1) ? "" : "s",
	    arena->stats.ncommit, (arena->stats.ncommit == 1) ? "" : "s",
	    arena->stats.decommitted,
	    (arena->stats.decommitted == 1) ? "" : "s");
#  endif

	malloc_printf("            allocated      nmalloc      ndalloc\n");
	malloc_printf("small:   %12zu %12llu %12llu\n",
	    arena->stats.allocated_small, arena->stats.nmalloc_small,
	    arena->stats.ndalloc_small);
	malloc_printf("large:   %12zu %12llu %12llu\n",
	    arena->stats.allocated_large, arena->stats.nmalloc_large,
	    arena->stats.ndalloc_large);
	malloc_printf("total:   %12zu %12llu %12llu\n",
	    arena->stats.allocated_small + arena->stats.allocated_large,
	    arena->stats.nmalloc_small + arena->stats.nmalloc_large,
	    arena->stats.ndalloc_small + arena->stats.ndalloc_large);
	malloc_printf("mapped:  %12zu\n", arena->stats.mapped);
#endif
	malloc_printf("bins:     bin   size regs pgs  requests   newruns"
	    "    reruns maxruns curruns\n");
	for (i = 0, gap_start = UINT_MAX; i < ntbins + nqbins + nsbins; i++) {
		if (arena->bins[i].stats.nrequests == 0) {
			if (gap_start == UINT_MAX)
				gap_start = i;
		} else {
			if (gap_start != UINT_MAX) {
				if (i > gap_start + 1) {
					/* Gap of more than one size class. */
					malloc_printf("[%u..%u]\n",
					    gap_start, i - 1);
				} else {
					/* Gap of one size class. */
					malloc_printf("[%u]\n", gap_start);
				}
				gap_start = UINT_MAX;
			}
			malloc_printf(
#if defined(MOZ_MEMORY_WINDOWS)
			    "%13u %1s %4u %4u %3u %9I64u %9I64u"
			    " %9I64u %7u %7u\n",
#else
			    "%13u %1s %4u %4u %3u %9llu %9llu"
			    " %9llu %7lu %7lu\n",
#endif
			    i,
			    i < ntbins ? "T" : i < ntbins + nqbins ? "Q" : "S",
			    arena->bins[i].reg_size,
			    arena->bins[i].nregs,
			    arena->bins[i].run_size >> pagesize_2pow,
			    arena->bins[i].stats.nrequests,
			    arena->bins[i].stats.nruns,
			    arena->bins[i].stats.reruns,
			    arena->bins[i].stats.highruns,
			    arena->bins[i].stats.curruns);
		}
	}
	if (gap_start != UINT_MAX) {
		if (i > gap_start + 1) {
			/* Gap of more than one size class. */
			malloc_printf("[%u..%u]\n", gap_start, i - 1);
		} else {
			/* Gap of one size class. */
			malloc_printf("[%u]\n", gap_start);
		}
	}
}
#endif

/*
 * End Utility functions/macros.
 */
/******************************************************************************/
/*
 * Begin extent tree code.
 */

static inline int
extent_szad_comp(extent_node_t *a, extent_node_t *b)
{
	int ret;
	size_t a_size = a->size;
	size_t b_size = b->size;

	ret = (a_size > b_size) - (a_size < b_size);
	if (ret == 0) {
		uintptr_t a_addr = (uintptr_t)a->addr;
		uintptr_t b_addr = (uintptr_t)b->addr;

		ret = (a_addr > b_addr) - (a_addr < b_addr);
	}

	return (ret);
}

/* Wrap red-black tree macros in functions. */
rb_wrap(static, extent_tree_szad_, extent_tree_t, extent_node_t,
    link_szad, extent_szad_comp)

static inline int
extent_ad_comp(extent_node_t *a, extent_node_t *b)
{
	uintptr_t a_addr = (uintptr_t)a->addr;
	uintptr_t b_addr = (uintptr_t)b->addr;

	return ((a_addr > b_addr) - (a_addr < b_addr));
}

/* Wrap red-black tree macros in functions. */
rb_wrap(static, extent_tree_ad_, extent_tree_t, extent_node_t, link_ad,
    extent_ad_comp)

/*
 * End extent tree code.
 */
/******************************************************************************/
/*
 * Begin chunk management functions.
 */

#ifdef MOZ_MEMORY_WINDOWS
#ifdef MOZ_MEMORY_WINCE
#define ALIGN_ADDR2OFFSET(al, ad) \
	((uintptr_t)ad & (al - 1))
static void *
pages_map_align(size_t size, int pfd, size_t alignment)
{
	
	void *ret; 
	int offset;
	if (size % alignment)
		size += (alignment - (size % alignment));
	assert(size >= alignment);
	ret = pages_map(NULL, size, pfd);
	offset = ALIGN_ADDR2OFFSET(alignment, ret);
	if (offset) {  
		/* try to over allocate by the ammount we're offset */
		void *tmp;
		pages_unmap(ret, size);
		tmp = VirtualAlloc(NULL, size + alignment - offset, 
					 MEM_RESERVE, PAGE_NOACCESS);
		if (offset == ALIGN_ADDR2OFFSET(alignment, tmp))
			ret = VirtualAlloc((void*)((intptr_t)tmp + alignment 
						   - offset), size, MEM_COMMIT,
					   PAGE_READWRITE);
		else 
			VirtualFree(tmp, 0, MEM_RELEASE);
		offset = ALIGN_ADDR2OFFSET(alignment, ret);
		
	
		if (offset) {  
			/* over allocate to ensure we have an aligned region */
			ret = VirtualAlloc(NULL, size + alignment, MEM_RESERVE, 
					   PAGE_NOACCESS);
			offset = ALIGN_ADDR2OFFSET(alignment, ret);
			ret = VirtualAlloc((void*)((intptr_t)ret + 
						   alignment - offset),
					   size, MEM_COMMIT, PAGE_READWRITE);
		}
	}
	return (ret);
}
#endif

static void *
pages_map(void *addr, size_t size, int pfd)
{
	void *ret = NULL;
#if defined(MOZ_MEMORY_WINCE) && !defined(MOZ_MEMORY_WINCE6)
	void *va_ret;
	assert(addr == NULL);
	va_ret = VirtualAlloc(addr, size, MEM_RESERVE, PAGE_NOACCESS);
	if (va_ret)
		ret = VirtualAlloc(va_ret, size, MEM_COMMIT, PAGE_READWRITE);
	assert(va_ret == ret);
#else
	ret = VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE,
	    PAGE_READWRITE);
#endif
	return (ret);
}

static void
pages_unmap(void *addr, size_t size)
{
	if (VirtualFree(addr, 0, MEM_RELEASE) == 0) {
#if defined(MOZ_MEMORY_WINCE) && !defined(MOZ_MEMORY_WINCE6)
		if (GetLastError() == ERROR_INVALID_PARAMETER) {
			MEMORY_BASIC_INFORMATION info;
			VirtualQuery(addr, &info, sizeof(info));
			if (VirtualFree(info.AllocationBase, 0, MEM_RELEASE))
				return;
		}
#endif
		_malloc_message(_getprogname(),
		    ": (malloc) Error in VirtualFree()\n", "", "");
		if (opt_abort)
			abort();
	}
}
#elif (defined(MOZ_MEMORY_DARWIN))
static void *
pages_map(void *addr, size_t size, int pfd)
{
	void *ret;
	kern_return_t err;
	int flags;

	if (addr != NULL) {
		ret = addr;
		flags = 0;
	} else
		flags = VM_FLAGS_ANYWHERE;

	err = vm_allocate((vm_map_t)mach_task_self(), (vm_address_t *)&ret,
	    (vm_size_t)size, flags);
	if (err != KERN_SUCCESS)
		ret = NULL;

	assert(ret == NULL || (addr == NULL && ret != addr)
	    || (addr != NULL && ret == addr));
	return (ret);
}

static void
pages_unmap(void *addr, size_t size)
{
	kern_return_t err;

	err = vm_deallocate((vm_map_t)mach_task_self(), (vm_address_t)addr,
	    (vm_size_t)size);
	if (err != KERN_SUCCESS) {
		malloc_message(_getprogname(),
		    ": (malloc) Error in vm_deallocate(): ",
		    mach_error_string(err), "\n");
		if (opt_abort)
			abort();
	}
}

#define	VM_COPY_MIN (pagesize << 5)
static inline void
pages_copy(void *dest, const void *src, size_t n)
{

	assert((void *)((uintptr_t)dest & ~pagesize_mask) == dest);
	assert(n >= VM_COPY_MIN);
	assert((void *)((uintptr_t)src & ~pagesize_mask) == src);

	vm_copy(mach_task_self(), (vm_address_t)src, (vm_size_t)n,
	    (vm_address_t)dest);
}
#else /* MOZ_MEMORY_DARWIN */
#ifdef JEMALLOC_USES_MAP_ALIGN
static void *
pages_map_align(size_t size, int pfd, size_t alignment)
{
	void *ret;

	/*
	 * We don't use MAP_FIXED here, because it can cause the *replacement*
	 * of existing mappings, and we only want to create new mappings.
	 */
#ifdef MALLOC_PAGEFILE
	if (pfd != -1) {
		ret = mmap((void *)alignment, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
		    MAP_NOSYNC | MAP_ALIGN, pfd, 0);
	} else
#endif
	       {
		ret = mmap((void *)alignment, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
		    MAP_NOSYNC | MAP_ALIGN | MAP_ANON, -1, 0);
	}
	assert(ret != NULL);

	if (ret == MAP_FAILED)
		ret = NULL;
	return (ret);
}
#endif

static void *
pages_map(void *addr, size_t size, int pfd)
{
	void *ret;

	/*
	 * We don't use MAP_FIXED here, because it can cause the *replacement*
	 * of existing mappings, and we only want to create new mappings.
	 */
#ifdef MALLOC_PAGEFILE
	if (pfd != -1) {
		ret = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
		    MAP_NOSYNC, pfd, 0);
	} else
#endif
	       {
		ret = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
		    MAP_ANON, -1, 0);
	}
	assert(ret != NULL);

	if (ret == MAP_FAILED)
		ret = NULL;
	else if (addr != NULL && ret != addr) {
		/*
		 * We succeeded in mapping memory, but not in the right place.
		 */
		if (munmap(ret, size) == -1) {
			char buf[STRERROR_BUF];

			strerror_r(errno, buf, sizeof(buf));
			_malloc_message(_getprogname(),
			    ": (malloc) Error in munmap(): ", buf, "\n");
			if (opt_abort)
				abort();
		}
		ret = NULL;
	}

	assert(ret == NULL || (addr == NULL && ret != addr)
	    || (addr != NULL && ret == addr));
	return (ret);
}

static void
pages_unmap(void *addr, size_t size)
{

	if (munmap(addr, size) == -1) {
		char buf[STRERROR_BUF];

		strerror_r(errno, buf, sizeof(buf));
		_malloc_message(_getprogname(),
		    ": (malloc) Error in munmap(): ", buf, "\n");
		if (opt_abort)
			abort();
	}
}
#endif

#ifdef MALLOC_VALIDATE
static inline malloc_rtree_t *
malloc_rtree_new(unsigned bits)
{
	malloc_rtree_t *ret;
	unsigned bits_per_level, height, i;

	bits_per_level = ffs(pow2_ceil((MALLOC_RTREE_NODESIZE /
	    sizeof(void *)))) - 1;
	height = bits / bits_per_level;
	if (height * bits_per_level != bits)
		height++;
	assert(height * bits_per_level >= bits);

	ret = (malloc_rtree_t*)base_calloc(1, sizeof(malloc_rtree_t) + (sizeof(unsigned) *
	    (height - 1)));
	if (ret == NULL)
		return (NULL);

	malloc_spin_init(&ret->lock);
	ret->height = height;
	if (bits_per_level * height > bits)
		ret->level2bits[0] = bits % bits_per_level;
	else
		ret->level2bits[0] = bits_per_level;
	for (i = 1; i < height; i++)
		ret->level2bits[i] = bits_per_level;

	ret->root = (void**)base_calloc(1, sizeof(void *) << ret->level2bits[0]);
	if (ret->root == NULL) {
		/*
		 * We leak the rtree here, since there's no generic base
		 * deallocation.
		 */
		return (NULL);
	}

	return (ret);
}

/* The least significant bits of the key are ignored. */
static inline void *
malloc_rtree_get(malloc_rtree_t *rtree, uintptr_t key)
{
	void *ret;
	uintptr_t subkey;
	unsigned i, lshift, height, bits;
	void **node, **child;

	malloc_spin_lock(&rtree->lock);
	for (i = lshift = 0, height = rtree->height, node = rtree->root;
	    i < height - 1;
	    i++, lshift += bits, node = child) {
		bits = rtree->level2bits[i];
		subkey = (key << lshift) >> ((SIZEOF_PTR << 3) - bits);
		child = (void**)node[subkey];
		if (child == NULL) {
			malloc_spin_unlock(&rtree->lock);
			return (NULL);
		}
	}

	/* node is a leaf, so it contains values rather than node pointers. */
	bits = rtree->level2bits[i];
	subkey = (key << lshift) >> ((SIZEOF_PTR << 3) - bits);
	ret = node[subkey];
	malloc_spin_unlock(&rtree->lock);

	return (ret);
}

static inline bool
malloc_rtree_set(malloc_rtree_t *rtree, uintptr_t key, void *val)
{
	uintptr_t subkey;
	unsigned i, lshift, height, bits;
	void **node, **child;

	malloc_spin_lock(&rtree->lock);
	for (i = lshift = 0, height = rtree->height, node = rtree->root;
	    i < height - 1;
	    i++, lshift += bits, node = child) {
		bits = rtree->level2bits[i];
		subkey = (key << lshift) >> ((SIZEOF_PTR << 3) - bits);
		child = (void**)node[subkey];
		if (child == NULL) {
			child = (void**)base_calloc(1, sizeof(void *) <<
			    rtree->level2bits[i+1]);
			if (child == NULL) {
				malloc_spin_unlock(&rtree->lock);
				return (true);
			}
			node[subkey] = child;
		}
	}

	/* node is a leaf, so it contains values rather than node pointers. */
	bits = rtree->level2bits[i];
	subkey = (key << lshift) >> ((SIZEOF_PTR << 3) - bits);
	node[subkey] = val;
	malloc_spin_unlock(&rtree->lock);

	return (false);
}
#endif

static void *
chunk_alloc_mmap(size_t size, bool pagefile)
{
	void *ret;
#ifndef JEMALLOC_USES_MAP_ALIGN
	size_t offset;
#endif
	int pfd;

#ifdef MALLOC_PAGEFILE
	if (opt_pagefile && pagefile) {
		pfd = pagefile_init(size);
		if (pfd == -1)
			return (NULL);
	} else
#endif
		pfd = -1;

	/*
	 * Windows requires that there be a 1:1 mapping between VM
	 * allocation/deallocation operations.  Therefore, take care here to
	 * acquire the final result via one mapping operation.  This means
	 * unmapping any preliminary result that is not correctly aligned.
	 *
	 * The MALLOC_PAGEFILE code also benefits from this mapping algorithm,
	 * since it reduces the number of page files.
	 */

#ifdef JEMALLOC_USES_MAP_ALIGN
	ret = pages_map_align(size, pfd, chunksize);
#else
	ret = pages_map(NULL, size, pfd);
	if (ret == NULL)
		goto RETURN;

	offset = CHUNK_ADDR2OFFSET(ret);
	if (offset != 0) {
		/* Deallocate, then try to allocate at (ret + size - offset). */
		pages_unmap(ret, size);
		ret = pages_map((void *)((uintptr_t)ret + size - offset), size,
		    pfd);
		while (ret == NULL) {
			/*
			 * Over-allocate in order to map a memory region that
			 * is definitely large enough.
			 */
			ret = pages_map(NULL, size + chunksize, -1);
			if (ret == NULL)
				goto RETURN;
			/*
			 * Deallocate, then allocate the correct size, within
			 * the over-sized mapping.
			 */
			offset = CHUNK_ADDR2OFFSET(ret);
			pages_unmap(ret, size + chunksize);
			if (offset == 0)
				ret = pages_map(ret, size, pfd);
			else {
				ret = pages_map((void *)((uintptr_t)ret +
				    chunksize - offset), size, pfd);
			}
			/*
			 * Failure here indicates a race with another thread, so
			 * try again.
			 */
		}
	}
RETURN:
#endif
#ifdef MALLOC_PAGEFILE
	if (pfd != -1)
		pagefile_close(pfd);
#endif
#ifdef MALLOC_STATS
	if (ret != NULL)
		stats_chunks.nchunks += (size / chunksize);
#endif
	return (ret);
}

#ifdef MALLOC_PAGEFILE
static int
pagefile_init(size_t size)
{
	int ret;
	size_t i;
	char pagefile_path[PATH_MAX];
	char zbuf[MALLOC_PAGEFILE_WRITE_SIZE];

	/*
	 * Create a temporary file, then immediately unlink it so that it will
	 * not persist.
	 */
	strcpy(pagefile_path, pagefile_templ);
	ret = mkstemp(pagefile_path);
	if (ret == -1)
		return (ret);
	if (unlink(pagefile_path)) {
		char buf[STRERROR_BUF];

		strerror_r(errno, buf, sizeof(buf));
		_malloc_message(_getprogname(), ": (malloc) Error in unlink(\"",
		    pagefile_path, "\"):");
		_malloc_message(buf, "\n", "", "");
		if (opt_abort)
			abort();
	}

	/*
	 * Write sequential zeroes to the file in order to assure that disk
	 * space is committed, with minimal fragmentation.  It would be
	 * sufficient to write one zero per disk block, but that potentially
	 * results in more system calls, for no real gain.
	 */
	memset(zbuf, 0, sizeof(zbuf));
	for (i = 0; i < size; i += sizeof(zbuf)) {
		if (write(ret, zbuf, sizeof(zbuf)) != sizeof(zbuf)) {
			if (errno != ENOSPC) {
				char buf[STRERROR_BUF];

				strerror_r(errno, buf, sizeof(buf));
				_malloc_message(_getprogname(),
				    ": (malloc) Error in write(): ", buf, "\n");
				if (opt_abort)
					abort();
			}
			pagefile_close(ret);
			return (-1);
		}
	}

	return (ret);
}

static void
pagefile_close(int pfd)
{

	if (close(pfd)) {
		char buf[STRERROR_BUF];

		strerror_r(errno, buf, sizeof(buf));
		_malloc_message(_getprogname(),
		    ": (malloc) Error in close(): ", buf, "\n");
		if (opt_abort)
			abort();
	}
}
#endif

static void *
chunk_recycle_reserve(size_t size, bool zero)
{
	extent_node_t *node, key;

#ifdef MALLOC_DECOMMIT
	if (size != chunksize)
		return (NULL);
#endif

	key.addr = NULL;
	key.size = size;
	malloc_mutex_lock(&reserve_mtx);
	node = extent_tree_szad_nsearch(&reserve_chunks_szad, &key);
	if (node != NULL) {
		void *ret = node->addr;

		/* Remove node from the tree. */
		extent_tree_szad_remove(&reserve_chunks_szad, node);
#ifndef MALLOC_DECOMMIT
		if (node->size == size) {
#else
			assert(node->size == size);
#endif
			extent_tree_ad_remove(&reserve_chunks_ad, node);
			base_node_dealloc(node);
#ifndef MALLOC_DECOMMIT
		} else {
			/*
			 * Insert the remainder of node's address range as a
			 * smaller chunk.  Its position within reserve_chunks_ad
			 * does not change.
			 */
			assert(node->size > size);
			node->addr = (void *)((uintptr_t)node->addr + size);
			node->size -= size;
			extent_tree_szad_insert(&reserve_chunks_szad, node);
		}
#endif
		reserve_cur -= size;
		/*
		 * Try to replenish the reserve if this allocation depleted it.
		 */
#ifndef MALLOC_DECOMMIT
		if (reserve_cur < reserve_min) {
			size_t diff = reserve_min - reserve_cur;
#else
		while (reserve_cur < reserve_min) {
#  define diff chunksize
#endif
			void *chunk;

			malloc_mutex_unlock(&reserve_mtx);
			chunk = chunk_alloc_mmap(diff, true);
			malloc_mutex_lock(&reserve_mtx);
			if (chunk == NULL) {
				uint64_t seq = 0;

				do {
					seq = reserve_notify(RESERVE_CND_LOW,
					    size, seq);
					if (seq == 0)
						goto MALLOC_OUT;
				} while (reserve_cur < reserve_min);
			} else {
				extent_node_t *node;

				node = chunk_dealloc_reserve(chunk, diff);
				if (node == NULL) {
					uint64_t seq = 0;

					pages_unmap(chunk, diff);
					do {
						seq = reserve_notify(
						    RESERVE_CND_LOW, size, seq);
						if (seq == 0)
							goto MALLOC_OUT;
					} while (reserve_cur < reserve_min);
				}
			}
		}
MALLOC_OUT:
		malloc_mutex_unlock(&reserve_mtx);

#ifdef MALLOC_DECOMMIT
		pages_commit(ret, size);
#  undef diff
#else
		if (zero)
			memset(ret, 0, size);
#endif
		return (ret);
	}
	malloc_mutex_unlock(&reserve_mtx);

	return (NULL);
}

static void *
chunk_alloc(size_t size, bool zero, bool pagefile)
{
	void *ret;

	assert(size != 0);
	assert((size & chunksize_mask) == 0);

	ret = chunk_recycle_reserve(size, zero);
	if (ret != NULL)
		goto RETURN;

	ret = chunk_alloc_mmap(size, pagefile);
	if (ret != NULL) {
		goto RETURN;
	}

	/* All strategies for allocation failed. */
	ret = NULL;
RETURN:
#ifdef MALLOC_STATS
	if (ret != NULL)
		stats_chunks.curchunks += (size / chunksize);
	if (stats_chunks.curchunks > stats_chunks.highchunks)
		stats_chunks.highchunks = stats_chunks.curchunks;
#endif

#ifdef MALLOC_VALIDATE
	if (ret != NULL) {
		if (malloc_rtree_set(chunk_rtree, (uintptr_t)ret, ret)) {
			chunk_dealloc(ret, size);
			return (NULL);
		}
	}
#endif

	assert(CHUNK_ADDR2BASE(ret) == ret);
	return (ret);
}

static extent_node_t *
chunk_dealloc_reserve(void *chunk, size_t size)
{
	extent_node_t *node;

#ifdef MALLOC_DECOMMIT
	if (size != chunksize)
		return (NULL);
#else
	extent_node_t *prev, key;

	key.addr = (void *)((uintptr_t)chunk + size);
	node = extent_tree_ad_nsearch(&reserve_chunks_ad, &key);
	/* Try to coalesce forward. */
	if (node != NULL && node->addr == key.addr) {
		/*
		 * Coalesce chunk with the following address range.  This does
		 * not change the position within reserve_chunks_ad, so only
		 * remove/insert from/into reserve_chunks_szad.
		 */
		extent_tree_szad_remove(&reserve_chunks_szad, node);
		node->addr = chunk;
		node->size += size;
		extent_tree_szad_insert(&reserve_chunks_szad, node);
	} else {
#endif
		/* Coalescing forward failed, so insert a new node. */
		node = base_node_alloc();
		if (node == NULL)
			return (NULL);
		node->addr = chunk;
		node->size = size;
		extent_tree_ad_insert(&reserve_chunks_ad, node);
		extent_tree_szad_insert(&reserve_chunks_szad, node);
#ifndef MALLOC_DECOMMIT
	}

	/* Try to coalesce backward. */
	prev = extent_tree_ad_prev(&reserve_chunks_ad, node);
	if (prev != NULL && (void *)((uintptr_t)prev->addr + prev->size) ==
	    chunk) {
		/*
		 * Coalesce chunk with the previous address range.  This does
		 * not change the position within reserve_chunks_ad, so only
		 * remove/insert node from/into reserve_chunks_szad.
		 */
		extent_tree_szad_remove(&reserve_chunks_szad, prev);
		extent_tree_ad_remove(&reserve_chunks_ad, prev);

		extent_tree_szad_remove(&reserve_chunks_szad, node);
		node->addr = prev->addr;
		node->size += prev->size;
		extent_tree_szad_insert(&reserve_chunks_szad, node);

		base_node_dealloc(prev);
	}
#endif

#ifdef MALLOC_DECOMMIT
	pages_decommit(chunk, size);
#else
	madvise(chunk, size, MADV_FREE);
#endif

	reserve_cur += size;
	if (reserve_cur > reserve_max)
		reserve_shrink();

	return (node);
}

static void
chunk_dealloc_mmap(void *chunk, size_t size)
{

	pages_unmap(chunk, size);
}

static void
chunk_dealloc(void *chunk, size_t size)
{
	extent_node_t *node;

	assert(chunk != NULL);
	assert(CHUNK_ADDR2BASE(chunk) == chunk);
	assert(size != 0);
	assert((size & chunksize_mask) == 0);

#ifdef MALLOC_STATS
	stats_chunks.curchunks -= (size / chunksize);
#endif
#ifdef MALLOC_VALIDATE
	malloc_rtree_set(chunk_rtree, (uintptr_t)chunk, NULL);
#endif

	/* Try to merge chunk into the reserve. */
	malloc_mutex_lock(&reserve_mtx);
	node = chunk_dealloc_reserve(chunk, size);
	malloc_mutex_unlock(&reserve_mtx);
	if (node == NULL)
		chunk_dealloc_mmap(chunk, size);
}

/*
 * End chunk management functions.
 */
/******************************************************************************/
/*
 * Begin arena.
 */

/*
 * Choose an arena based on a per-thread value (fast-path code, calls slow-path
 * code if necessary).
 */
static inline arena_t *
choose_arena(void)
{
	arena_t *ret;

	/*
	 * We can only use TLS if this is a PIC library, since for the static
	 * library version, libc's malloc is used by TLS allocation, which
	 * introduces a bootstrapping issue.
	 */
#ifndef NO_TLS
	if (__isthreaded == false) {
	    /* Avoid the overhead of TLS for single-threaded operation. */
	    return (arenas[0]);
	}

#  ifdef MOZ_MEMORY_WINDOWS
	ret = (arena_t*)TlsGetValue(tlsIndex);
#  else
	ret = arenas_map;
#  endif

	if (ret == NULL) {
		ret = choose_arena_hard();
		assert(ret != NULL);
	}
#else
	if (__isthreaded && narenas > 1) {
		unsigned long ind;

		/*
		 * Hash _pthread_self() to one of the arenas.  There is a prime
		 * number of arenas, so this has a reasonable chance of
		 * working.  Even so, the hashing can be easily thwarted by
		 * inconvenient _pthread_self() values.  Without specific
		 * knowledge of how _pthread_self() calculates values, we can't
		 * easily do much better than this.
		 */
		ind = (unsigned long) _pthread_self() % narenas;

		/*
		 * Optimistially assume that arenas[ind] has been initialized.
		 * At worst, we find out that some other thread has already
		 * done so, after acquiring the lock in preparation.  Note that
		 * this lazy locking also has the effect of lazily forcing
		 * cache coherency; without the lock acquisition, there's no
		 * guarantee that modification of arenas[ind] by another thread
		 * would be seen on this CPU for an arbitrary amount of time.
		 *
		 * In general, this approach to modifying a synchronized value
		 * isn't a good idea, but in this case we only ever modify the
		 * value once, so things work out well.
		 */
		ret = arenas[ind];
		if (ret == NULL) {
			/*
			 * Avoid races with another thread that may have already
			 * initialized arenas[ind].
			 */
			malloc_spin_lock(&arenas_lock);
			if (arenas[ind] == NULL)
				ret = arenas_extend((unsigned)ind);
			else
				ret = arenas[ind];
			malloc_spin_unlock(&arenas_lock);
		}
	} else
		ret = arenas[0];
#endif

	assert(ret != NULL);
	return (ret);
}

#ifndef NO_TLS
/*
 * Choose an arena based on a per-thread value (slow-path code only, called
 * only by choose_arena()).
 */
static arena_t *
choose_arena_hard(void)
{
	arena_t *ret;

	assert(__isthreaded);

#ifdef MALLOC_BALANCE
	/* Seed the PRNG used for arena load balancing. */
	SPRN(balance, (uint32_t)(uintptr_t)(_pthread_self()));
#endif

	if (narenas > 1) {
#ifdef MALLOC_BALANCE
		unsigned ind;

		ind = PRN(balance, narenas_2pow);
		if ((ret = arenas[ind]) == NULL) {
			malloc_spin_lock(&arenas_lock);
			if ((ret = arenas[ind]) == NULL)
				ret = arenas_extend(ind);
			malloc_spin_unlock(&arenas_lock);
		}
#else
		malloc_spin_lock(&arenas_lock);
		if ((ret = arenas[next_arena]) == NULL)
			ret = arenas_extend(next_arena);
		next_arena = (next_arena + 1) % narenas;
		malloc_spin_unlock(&arenas_lock);
#endif
	} else
		ret = arenas[0];

#ifdef MOZ_MEMORY_WINDOWS
	TlsSetValue(tlsIndex, ret);
#else
	arenas_map = ret;
#endif

	return (ret);
}
#endif

static inline int
arena_chunk_comp(arena_chunk_t *a, arena_chunk_t *b)
{
	uintptr_t a_chunk = (uintptr_t)a;
	uintptr_t b_chunk = (uintptr_t)b;

	assert(a != NULL);
	assert(b != NULL);

	return ((a_chunk > b_chunk) - (a_chunk < b_chunk));
}

/* Wrap red-black tree macros in functions. */
rb_wrap(static, arena_chunk_tree_dirty_, arena_chunk_tree_t,
    arena_chunk_t, link_dirty, arena_chunk_comp)

static inline int
arena_run_comp(arena_chunk_map_t *a, arena_chunk_map_t *b)
{
	uintptr_t a_mapelm = (uintptr_t)a;
	uintptr_t b_mapelm = (uintptr_t)b;

	assert(a != NULL);
	assert(b != NULL);

	return ((a_mapelm > b_mapelm) - (a_mapelm < b_mapelm));
}

/* Wrap red-black tree macros in functions. */
rb_wrap(static, arena_run_tree_, arena_run_tree_t, arena_chunk_map_t, link,
    arena_run_comp)

static inline int
arena_avail_comp(arena_chunk_map_t *a, arena_chunk_map_t *b)
{
	int ret;
	size_t a_size = a->bits & ~pagesize_mask;
	size_t b_size = b->bits & ~pagesize_mask;

	ret = (a_size > b_size) - (a_size < b_size);
	if (ret == 0) {
		uintptr_t a_mapelm, b_mapelm;

		if ((a->bits & CHUNK_MAP_KEY) == 0)
			a_mapelm = (uintptr_t)a;
		else {
			/*
			 * Treat keys as though they are lower than anything
			 * else.
			 */
			a_mapelm = 0;
		}
		b_mapelm = (uintptr_t)b;

		ret = (a_mapelm > b_mapelm) - (a_mapelm < b_mapelm);
	}

	return (ret);
}

/* Wrap red-black tree macros in functions. */
rb_wrap(static, arena_avail_tree_, arena_avail_tree_t, arena_chunk_map_t, link,
    arena_avail_comp)

static inline void *
arena_run_reg_alloc(arena_run_t *run, arena_bin_t *bin)
{
	void *ret;
	unsigned i, mask, bit, regind;

	assert(run->magic == ARENA_RUN_MAGIC);
	assert(run->regs_minelm < bin->regs_mask_nelms);

	/*
	 * Move the first check outside the loop, so that run->regs_minelm can
	 * be updated unconditionally, without the possibility of updating it
	 * multiple times.
	 */
	i = run->regs_minelm;
	mask = run->regs_mask[i];
	if (mask != 0) {
		/* Usable allocation found. */
		bit = ffs((int)mask) - 1;

		regind = ((i << (SIZEOF_INT_2POW + 3)) + bit);
		assert(regind < bin->nregs);
		ret = (void *)(((uintptr_t)run) + bin->reg0_offset
		    + (bin->reg_size * regind));

		/* Clear bit. */
		mask ^= (1U << bit);
		run->regs_mask[i] = mask;

		return (ret);
	}

	for (i++; i < bin->regs_mask_nelms; i++) {
		mask = run->regs_mask[i];
		if (mask != 0) {
			/* Usable allocation found. */
			bit = ffs((int)mask) - 1;

			regind = ((i << (SIZEOF_INT_2POW + 3)) + bit);
			assert(regind < bin->nregs);
			ret = (void *)(((uintptr_t)run) + bin->reg0_offset
			    + (bin->reg_size * regind));

			/* Clear bit. */
			mask ^= (1U << bit);
			run->regs_mask[i] = mask;

			/*
			 * Make a note that nothing before this element
			 * contains a free region.
			 */
			run->regs_minelm = i; /* Low payoff: + (mask == 0); */

			return (ret);
		}
	}
	/* Not reached. */
	assert(0);
	return (NULL);
}

static inline void
arena_run_reg_dalloc(arena_run_t *run, arena_bin_t *bin, void *ptr, size_t size)
{
	/*
	 * To divide by a number D that is not a power of two we multiply
	 * by (2^21 / D) and then right shift by 21 positions.
	 *
	 *   X / D
	 *
	 * becomes
	 *
	 *   (X * size_invs[(D >> QUANTUM_2POW_MIN) - 3]) >> SIZE_INV_SHIFT
	 */
#define	SIZE_INV_SHIFT 21
#define	SIZE_INV(s) (((1U << SIZE_INV_SHIFT) / (s << QUANTUM_2POW_MIN)) + 1)
	static const unsigned size_invs[] = {
	    SIZE_INV(3),
	    SIZE_INV(4), SIZE_INV(5), SIZE_INV(6), SIZE_INV(7),
	    SIZE_INV(8), SIZE_INV(9), SIZE_INV(10), SIZE_INV(11),
	    SIZE_INV(12),SIZE_INV(13), SIZE_INV(14), SIZE_INV(15),
	    SIZE_INV(16),SIZE_INV(17), SIZE_INV(18), SIZE_INV(19),
	    SIZE_INV(20),SIZE_INV(21), SIZE_INV(22), SIZE_INV(23),
	    SIZE_INV(24),SIZE_INV(25), SIZE_INV(26), SIZE_INV(27),
	    SIZE_INV(28),SIZE_INV(29), SIZE_INV(30), SIZE_INV(31)
#if (QUANTUM_2POW_MIN < 4)
	    ,
	    SIZE_INV(32), SIZE_INV(33), SIZE_INV(34), SIZE_INV(35),
	    SIZE_INV(36), SIZE_INV(37), SIZE_INV(38), SIZE_INV(39),
	    SIZE_INV(40), SIZE_INV(41), SIZE_INV(42), SIZE_INV(43),
	    SIZE_INV(44), SIZE_INV(45), SIZE_INV(46), SIZE_INV(47),
	    SIZE_INV(48), SIZE_INV(49), SIZE_INV(50), SIZE_INV(51),
	    SIZE_INV(52), SIZE_INV(53), SIZE_INV(54), SIZE_INV(55),
	    SIZE_INV(56), SIZE_INV(57), SIZE_INV(58), SIZE_INV(59),
	    SIZE_INV(60), SIZE_INV(61), SIZE_INV(62), SIZE_INV(63)
#endif
	};
	unsigned diff, regind, elm, bit;

	assert(run->magic == ARENA_RUN_MAGIC);
	assert(((sizeof(size_invs)) / sizeof(unsigned)) + 3
	    >= (SMALL_MAX_DEFAULT >> QUANTUM_2POW_MIN));

	/*
	 * Avoid doing division with a variable divisor if possible.  Using
	 * actual division here can reduce allocator throughput by over 20%!
	 */
	diff = (unsigned)((uintptr_t)ptr - (uintptr_t)run - bin->reg0_offset);
	if ((size & (size - 1)) == 0) {
		/*
		 * log2_table allows fast division of a power of two in the
		 * [1..128] range.
		 *
		 * (x / divisor) becomes (x >> log2_table[divisor - 1]).
		 */
		static const unsigned char log2_table[] = {
		    0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7
		};

		if (size <= 128)
			regind = (diff >> log2_table[size - 1]);
		else if (size <= 32768)
			regind = diff >> (8 + log2_table[(size >> 8) - 1]);
		else {
			/*
			 * The run size is too large for us to use the lookup
			 * table.  Use real division.
			 */
			regind = diff / size;
		}
	} else if (size <= ((sizeof(size_invs) / sizeof(unsigned))
	    << QUANTUM_2POW_MIN) + 2) {
		regind = size_invs[(size >> QUANTUM_2POW_MIN) - 3] * diff;
		regind >>= SIZE_INV_SHIFT;
	} else {
		/*
		 * size_invs isn't large enough to handle this size class, so
		 * calculate regind using actual division.  This only happens
		 * if the user increases small_max via the 'S' runtime
		 * configuration option.
		 */
		regind = diff / size;
	};
	assert(diff == regind * size);
	assert(regind < bin->nregs);

	elm = regind >> (SIZEOF_INT_2POW + 3);
	if (elm < run->regs_minelm)
		run->regs_minelm = elm;
	bit = regind - (elm << (SIZEOF_INT_2POW + 3));
	assert((run->regs_mask[elm] & (1U << bit)) == 0);
	run->regs_mask[elm] |= (1U << bit);
#undef SIZE_INV
#undef SIZE_INV_SHIFT
}

static void
arena_run_split(arena_t *arena, arena_run_t *run, size_t size, bool large,
    bool zero)
{
	arena_chunk_t *chunk;
	size_t old_ndirty, run_ind, total_pages, need_pages, rem_pages, i;

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(run);
	old_ndirty = chunk->ndirty;
	run_ind = (unsigned)(((uintptr_t)run - (uintptr_t)chunk)
	    >> pagesize_2pow);
	total_pages = (chunk->map[run_ind].bits & ~pagesize_mask) >>
	    pagesize_2pow;
	need_pages = (size >> pagesize_2pow);
	assert(need_pages > 0);
	assert(need_pages <= total_pages);
	rem_pages = total_pages - need_pages;

	arena_avail_tree_remove(&arena->runs_avail, &chunk->map[run_ind]);

	/* Keep track of trailing unused pages for later use. */
	if (rem_pages > 0) {
		chunk->map[run_ind+need_pages].bits = (rem_pages <<
		    pagesize_2pow) | (chunk->map[run_ind+need_pages].bits &
		    pagesize_mask);
		chunk->map[run_ind+total_pages-1].bits = (rem_pages <<
		    pagesize_2pow) | (chunk->map[run_ind+total_pages-1].bits &
		    pagesize_mask);
		arena_avail_tree_insert(&arena->runs_avail,
		    &chunk->map[run_ind+need_pages]);
	}

	for (i = 0; i < need_pages; i++) {
#ifdef MALLOC_DECOMMIT
		/*
		 * Commit decommitted pages if necessary.  If a decommitted
		 * page is encountered, commit all needed adjacent decommitted
		 * pages in one operation, in order to reduce system call
		 * overhead.
		 */
		if (chunk->map[run_ind + i].bits & CHUNK_MAP_DECOMMITTED) {
			size_t j;

			/*
			 * Advance i+j to just past the index of the last page
			 * to commit.  Clear CHUNK_MAP_DECOMMITTED along the
			 * way.
			 */
			for (j = 0; i + j < need_pages && (chunk->map[run_ind +
			    i + j].bits & CHUNK_MAP_DECOMMITTED); j++) {
				chunk->map[run_ind + i + j].bits ^=
				    CHUNK_MAP_DECOMMITTED;
			}

			pages_commit((void *)((uintptr_t)chunk + ((run_ind + i)
			    << pagesize_2pow)), (j << pagesize_2pow));
#  ifdef MALLOC_STATS
			arena->stats.ncommit++;
#  endif
		} else /* No need to zero since commit zeros. */
#endif

		/* Zero if necessary. */
		if (zero) {
			if ((chunk->map[run_ind + i].bits & CHUNK_MAP_ZEROED)
			    == 0) {
				VALGRIND_MALLOCLIKE_BLOCK((void *)((uintptr_t)
				    chunk + ((run_ind + i) << pagesize_2pow)),
				    pagesize, 0, false);
				memset((void *)((uintptr_t)chunk + ((run_ind
				    + i) << pagesize_2pow)), 0, pagesize);
				VALGRIND_FREELIKE_BLOCK((void *)((uintptr_t)
				    chunk + ((run_ind + i) << pagesize_2pow)),
				    0);
				/* CHUNK_MAP_ZEROED is cleared below. */
			}
		}

		/* Update dirty page accounting. */
		if (chunk->map[run_ind + i].bits & CHUNK_MAP_DIRTY) {
			chunk->ndirty--;
			arena->ndirty--;
			/* CHUNK_MAP_DIRTY is cleared below. */
		}

		/* Initialize the chunk map. */
		if (large) {
			chunk->map[run_ind + i].bits = CHUNK_MAP_LARGE
			    | CHUNK_MAP_ALLOCATED;
		} else {
			chunk->map[run_ind + i].bits = (size_t)run
			    | CHUNK_MAP_ALLOCATED;
		}
	}

	/*
	 * Set the run size only in the first element for large runs.  This is
	 * primarily a debugging aid, since the lack of size info for trailing
	 * pages only matters if the application tries to operate on an
	 * interior pointer.
	 */
	if (large)
		chunk->map[run_ind].bits |= size;

	if (chunk->ndirty == 0 && old_ndirty > 0)
		arena_chunk_tree_dirty_remove(&arena->chunks_dirty, chunk);
}

static void
arena_chunk_init(arena_t *arena, arena_chunk_t *chunk)
{
	arena_run_t *run;
	size_t i;

	VALGRIND_MALLOCLIKE_BLOCK(chunk, (arena_chunk_header_npages <<
	    pagesize_2pow), 0, false);
#ifdef MALLOC_STATS
	arena->stats.mapped += chunksize;
#endif

	chunk->arena = arena;

	/*
	 * Claim that no pages are in use, since the header is merely overhead.
	 */
	chunk->ndirty = 0;

	/* Initialize the map to contain one maximal free untouched run. */
	run = (arena_run_t *)((uintptr_t)chunk + (arena_chunk_header_npages <<
	    pagesize_2pow));
	for (i = 0; i < arena_chunk_header_npages; i++)
		chunk->map[i].bits = 0;
	chunk->map[i].bits = arena_maxclass
#ifdef MALLOC_DECOMMIT
	    | CHUNK_MAP_DECOMMITTED
#endif
	    | CHUNK_MAP_ZEROED;
	for (i++; i < chunk_npages-1; i++) {
		chunk->map[i].bits =
#ifdef MALLOC_DECOMMIT
		    CHUNK_MAP_DECOMMITTED |
#endif
		    CHUNK_MAP_ZEROED;
	}
	chunk->map[chunk_npages-1].bits = arena_maxclass
#ifdef MALLOC_DECOMMIT
	    | CHUNK_MAP_DECOMMITTED
#endif
	    | CHUNK_MAP_ZEROED;

#ifdef MALLOC_DECOMMIT
	/*
	 * Start out decommitted, in order to force a closer correspondence
	 * between dirty pages and committed untouched pages.
	 */
	pages_decommit(run, arena_maxclass);
#  ifdef MALLOC_STATS
	arena->stats.ndecommit++;
	arena->stats.decommitted += (chunk_npages - arena_chunk_header_npages);
#  endif
#endif

	/* Insert the run into the runs_avail tree. */
	arena_avail_tree_insert(&arena->runs_avail,
	    &chunk->map[arena_chunk_header_npages]);
}

static void
arena_chunk_dealloc(arena_t *arena, arena_chunk_t *chunk)
{

	if (arena->spare != NULL) {
		if (arena->spare->ndirty > 0) {
			arena_chunk_tree_dirty_remove(
			    &chunk->arena->chunks_dirty, arena->spare);
			arena->ndirty -= arena->spare->ndirty;
		}
		VALGRIND_FREELIKE_BLOCK(arena->spare, 0);
		chunk_dealloc((void *)arena->spare, chunksize);
#ifdef MALLOC_STATS
		arena->stats.mapped -= chunksize;
#endif
	}

	/*
	 * Remove run from runs_avail, regardless of whether this chunk
	 * will be cached, so that the arena does not use it.  Dirty page
	 * flushing only uses the chunks_dirty tree, so leaving this chunk in
	 * the chunks_* trees is sufficient for that purpose.
	 */
	arena_avail_tree_remove(&arena->runs_avail,
	    &chunk->map[arena_chunk_header_npages]);

	arena->spare = chunk;
}

static arena_run_t *
arena_run_alloc(arena_t *arena, arena_bin_t *bin, size_t size, bool large,
    bool zero)
{
	arena_chunk_t *chunk;
	arena_run_t *run;
	arena_chunk_map_t *mapelm, key;

	assert(size <= arena_maxclass);
	assert((size & pagesize_mask) == 0);

	chunk = NULL;
	while (true) {
		/* Search the arena's chunks for the lowest best fit. */
		key.bits = size | CHUNK_MAP_KEY;
		mapelm = arena_avail_tree_nsearch(&arena->runs_avail, &key);
		if (mapelm != NULL) {
			arena_chunk_t *run_chunk = (arena_chunk_t*)CHUNK_ADDR2BASE(mapelm);
			size_t pageind = ((uintptr_t)mapelm -
			    (uintptr_t)run_chunk->map) /
			    sizeof(arena_chunk_map_t);

			if (chunk != NULL)
				chunk_dealloc(chunk, chunksize);
			run = (arena_run_t *)((uintptr_t)run_chunk + (pageind
			    << pagesize_2pow));
			arena_run_split(arena, run, size, large, zero);
			return (run);
		}

		if (arena->spare != NULL) {
			/* Use the spare. */
			chunk = arena->spare;
			arena->spare = NULL;
			run = (arena_run_t *)((uintptr_t)chunk +
			    (arena_chunk_header_npages << pagesize_2pow));
			/* Insert the run into the runs_avail tree. */
			arena_avail_tree_insert(&arena->runs_avail,
			    &chunk->map[arena_chunk_header_npages]);
			arena_run_split(arena, run, size, large, zero);
			return (run);
		}

		/*
		 * No usable runs.  Create a new chunk from which to allocate
		 * the run.
		 */
		if (chunk == NULL) {
			uint64_t chunk_seq;

			/*
			 * Record the chunk allocation sequence number in order
			 * to detect races.
			 */
			arena->chunk_seq++;
			chunk_seq = arena->chunk_seq;

			/*
			 * Drop the arena lock while allocating a chunk, since
			 * reserve notifications may cause recursive
			 * allocation.  Dropping the lock here opens an
			 * allocataion race, but we recover.
			 */
			malloc_mutex_unlock(&arena->lock);
			chunk = (arena_chunk_t *)chunk_alloc(chunksize, true,
			    true);
			malloc_mutex_lock(&arena->lock);

			/*
			 * Check whether a race allowed a usable run to appear.
			 */
			if (bin != NULL && (run = bin->runcur) != NULL &&
			    run->nfree > 0) {
				if (chunk != NULL)
					chunk_dealloc(chunk, chunksize);
				return (run);
			}

			/*
			 * If this thread raced with another such that multiple
			 * chunks were allocated, make sure that there is still
			 * inadequate space before using this chunk.
			 */
			if (chunk_seq != arena->chunk_seq)
				continue;

			/*
			 * Check for an error *after* checking for a race,
			 * since a race could also cause a transient OOM
			 * condition.
			 */
			if (chunk == NULL)
				return (NULL);
		}

		arena_chunk_init(arena, chunk);
		run = (arena_run_t *)((uintptr_t)chunk +
		    (arena_chunk_header_npages << pagesize_2pow));
		/* Update page map. */
		arena_run_split(arena, run, size, large, zero);
		return (run);
	}
}

static void
arena_purge(arena_t *arena)
{
	arena_chunk_t *chunk;
	size_t i, npages;
#ifdef MALLOC_DEBUG
	size_t ndirty = 0;
	rb_foreach_begin(arena_chunk_t, link_dirty, &arena->chunks_dirty,
	    chunk) {
		ndirty += chunk->ndirty;
	} rb_foreach_end(arena_chunk_t, link_dirty, &arena->chunks_dirty, chunk)
	assert(ndirty == arena->ndirty);
#endif
	assert(arena->ndirty > opt_dirty_max);

#ifdef MALLOC_STATS
	arena->stats.npurge++;
#endif

	/*
	 * Iterate downward through chunks until enough dirty memory has been
	 * purged.  Terminate as soon as possible in order to minimize the
	 * number of system calls, even if a chunk has only been partially
	 * purged.
	 */
	while (arena->ndirty > (opt_dirty_max >> 1)) {
		chunk = arena_chunk_tree_dirty_last(&arena->chunks_dirty);
		assert(chunk != NULL);

		for (i = chunk_npages - 1; chunk->ndirty > 0; i--) {
			assert(i >= arena_chunk_header_npages);

			if (chunk->map[i].bits & CHUNK_MAP_DIRTY) {
#ifdef MALLOC_DECOMMIT
				assert((chunk->map[i].bits &
				    CHUNK_MAP_DECOMMITTED) == 0);
#endif
				chunk->map[i].bits ^=
#ifdef MALLOC_DECOMMIT
				    CHUNK_MAP_DECOMMITTED |
#endif
				    CHUNK_MAP_DIRTY;
				/* Find adjacent dirty run(s). */
				for (npages = 1; i > arena_chunk_header_npages
				    && (chunk->map[i - 1].bits &
				    CHUNK_MAP_DIRTY); npages++) {
					i--;
#ifdef MALLOC_DECOMMIT
					assert((chunk->map[i].bits &
					    CHUNK_MAP_DECOMMITTED) == 0);
#endif
					chunk->map[i].bits ^=
#ifdef MALLOC_DECOMMIT
					    CHUNK_MAP_DECOMMITTED |
#endif
					    CHUNK_MAP_DIRTY;
				}
				chunk->ndirty -= npages;
				arena->ndirty -= npages;

#ifdef MALLOC_DECOMMIT
				pages_decommit((void *)((uintptr_t)
				    chunk + (i << pagesize_2pow)),
				    (npages << pagesize_2pow));
#  ifdef MALLOC_STATS
				arena->stats.ndecommit++;
				arena->stats.decommitted += npages;
#  endif
#else
				madvise((void *)((uintptr_t)chunk + (i <<
				    pagesize_2pow)), (npages << pagesize_2pow),
				    MADV_FREE);
#endif
#ifdef MALLOC_STATS
				arena->stats.nmadvise++;
				arena->stats.purged += npages;
#endif
				if (arena->ndirty <= (opt_dirty_max >> 1))
					break;
			}
		}

		if (chunk->ndirty == 0) {
			arena_chunk_tree_dirty_remove(&arena->chunks_dirty,
			    chunk);
		}
	}
}

static void
arena_run_dalloc(arena_t *arena, arena_run_t *run, bool dirty)
{
	arena_chunk_t *chunk;
	size_t size, run_ind, run_pages;

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(run);
	run_ind = (size_t)(((uintptr_t)run - (uintptr_t)chunk)
	    >> pagesize_2pow);
	assert(run_ind >= arena_chunk_header_npages);
	assert(run_ind < chunk_npages);
	if ((chunk->map[run_ind].bits & CHUNK_MAP_LARGE) != 0)
		size = chunk->map[run_ind].bits & ~pagesize_mask;
	else
		size = run->bin->run_size;
	run_pages = (size >> pagesize_2pow);

	/* Mark pages as unallocated in the chunk map. */
	if (dirty) {
		size_t i;

		for (i = 0; i < run_pages; i++) {
			assert((chunk->map[run_ind + i].bits & CHUNK_MAP_DIRTY)
			    == 0);
			chunk->map[run_ind + i].bits = CHUNK_MAP_DIRTY;
		}

		if (chunk->ndirty == 0) {
			arena_chunk_tree_dirty_insert(&arena->chunks_dirty,
			    chunk);
		}
		chunk->ndirty += run_pages;
		arena->ndirty += run_pages;
	} else {
		size_t i;

		for (i = 0; i < run_pages; i++) {
			chunk->map[run_ind + i].bits &= ~(CHUNK_MAP_LARGE |
			    CHUNK_MAP_ALLOCATED);
		}
	}
	chunk->map[run_ind].bits = size | (chunk->map[run_ind].bits &
	    pagesize_mask);
	chunk->map[run_ind+run_pages-1].bits = size |
	    (chunk->map[run_ind+run_pages-1].bits & pagesize_mask);

	/* Try to coalesce forward. */
	if (run_ind + run_pages < chunk_npages &&
	    (chunk->map[run_ind+run_pages].bits & CHUNK_MAP_ALLOCATED) == 0) {
		size_t nrun_size = chunk->map[run_ind+run_pages].bits &
		    ~pagesize_mask;

		/*
		 * Remove successor from runs_avail; the coalesced run is
		 * inserted later.
		 */
		arena_avail_tree_remove(&arena->runs_avail,
		    &chunk->map[run_ind+run_pages]);

		size += nrun_size;
		run_pages = size >> pagesize_2pow;

		assert((chunk->map[run_ind+run_pages-1].bits & ~pagesize_mask)
		    == nrun_size);
		chunk->map[run_ind].bits = size | (chunk->map[run_ind].bits &
		    pagesize_mask);
		chunk->map[run_ind+run_pages-1].bits = size |
		    (chunk->map[run_ind+run_pages-1].bits & pagesize_mask);
	}

	/* Try to coalesce backward. */
	if (run_ind > arena_chunk_header_npages && (chunk->map[run_ind-1].bits &
	    CHUNK_MAP_ALLOCATED) == 0) {
		size_t prun_size = chunk->map[run_ind-1].bits & ~pagesize_mask;

		run_ind -= prun_size >> pagesize_2pow;

		/*
		 * Remove predecessor from runs_avail; the coalesced run is
		 * inserted later.
		 */
		arena_avail_tree_remove(&arena->runs_avail,
		    &chunk->map[run_ind]);

		size += prun_size;
		run_pages = size >> pagesize_2pow;

		assert((chunk->map[run_ind].bits & ~pagesize_mask) ==
		    prun_size);
		chunk->map[run_ind].bits = size | (chunk->map[run_ind].bits &
		    pagesize_mask);
		chunk->map[run_ind+run_pages-1].bits = size |
		    (chunk->map[run_ind+run_pages-1].bits & pagesize_mask);
	}

	/* Insert into runs_avail, now that coalescing is complete. */
	arena_avail_tree_insert(&arena->runs_avail, &chunk->map[run_ind]);

	/* Deallocate chunk if it is now completely unused. */
	if ((chunk->map[arena_chunk_header_npages].bits & (~pagesize_mask |
	    CHUNK_MAP_ALLOCATED)) == arena_maxclass)
		arena_chunk_dealloc(arena, chunk);

	/* Enforce opt_dirty_max. */
	if (arena->ndirty > opt_dirty_max)
		arena_purge(arena);
}

static void
arena_run_trim_head(arena_t *arena, arena_chunk_t *chunk, arena_run_t *run,
    size_t oldsize, size_t newsize)
{
	size_t pageind = ((uintptr_t)run - (uintptr_t)chunk) >> pagesize_2pow;
	size_t head_npages = (oldsize - newsize) >> pagesize_2pow;

	assert(oldsize > newsize);

	/*
	 * Update the chunk map so that arena_run_dalloc() can treat the
	 * leading run as separately allocated.
	 */
	chunk->map[pageind].bits = (oldsize - newsize) | CHUNK_MAP_LARGE |
	    CHUNK_MAP_ALLOCATED;
	chunk->map[pageind+head_npages].bits = newsize | CHUNK_MAP_LARGE |
	    CHUNK_MAP_ALLOCATED;

	arena_run_dalloc(arena, run, false);
}

static void
arena_run_trim_tail(arena_t *arena, arena_chunk_t *chunk, arena_run_t *run,
    size_t oldsize, size_t newsize, bool dirty)
{
	size_t pageind = ((uintptr_t)run - (uintptr_t)chunk) >> pagesize_2pow;
	size_t npages = newsize >> pagesize_2pow;

	assert(oldsize > newsize);

	/*
	 * Update the chunk map so that arena_run_dalloc() can treat the
	 * trailing run as separately allocated.
	 */
	chunk->map[pageind].bits = newsize | CHUNK_MAP_LARGE |
	    CHUNK_MAP_ALLOCATED;
	chunk->map[pageind+npages].bits = (oldsize - newsize) | CHUNK_MAP_LARGE
	    | CHUNK_MAP_ALLOCATED;

	arena_run_dalloc(arena, (arena_run_t *)((uintptr_t)run + newsize),
	    dirty);
}

static arena_run_t *
arena_bin_nonfull_run_get(arena_t *arena, arena_bin_t *bin)
{
	arena_chunk_map_t *mapelm;
	arena_run_t *run;
	unsigned i, remainder;

	/* Look for a usable run. */
	mapelm = arena_run_tree_first(&bin->runs);
	if (mapelm != NULL) {
		/* run is guaranteed to have available space. */
		arena_run_tree_remove(&bin->runs, mapelm);
		run = (arena_run_t *)(mapelm->bits & ~pagesize_mask);
#ifdef MALLOC_STATS
		bin->stats.reruns++;
#endif
		return (run);
	}
	/* No existing runs have any space available. */

	/* Allocate a new run. */
	run = arena_run_alloc(arena, bin, bin->run_size, false, false);
	if (run == NULL)
		return (NULL);
	/*
	 * Don't initialize if a race in arena_run_alloc() allowed an existing
	 * run to become usable.
	 */
	if (run == bin->runcur)
		return (run);

	VALGRIND_MALLOCLIKE_BLOCK(run, sizeof(arena_run_t) + (sizeof(unsigned) *
	    (bin->regs_mask_nelms - 1)), 0, false);

	/* Initialize run internals. */
	run->bin = bin;

	for (i = 0; i < bin->regs_mask_nelms - 1; i++)
		run->regs_mask[i] = UINT_MAX;
	remainder = bin->nregs & ((1U << (SIZEOF_INT_2POW + 3)) - 1);
	if (remainder == 0)
		run->regs_mask[i] = UINT_MAX;
	else {
		/* The last element has spare bits that need to be unset. */
		run->regs_mask[i] = (UINT_MAX >> ((1U << (SIZEOF_INT_2POW + 3))
		    - remainder));
	}

	run->regs_minelm = 0;

	run->nfree = bin->nregs;
#ifdef MALLOC_DEBUG
	run->magic = ARENA_RUN_MAGIC;
#endif

#ifdef MALLOC_STATS
	bin->stats.nruns++;
	bin->stats.curruns++;
	if (bin->stats.curruns > bin->stats.highruns)
		bin->stats.highruns = bin->stats.curruns;
#endif
	return (run);
}

/* bin->runcur must have space available before this function is called. */
static inline void *
arena_bin_malloc_easy(arena_t *arena, arena_bin_t *bin, arena_run_t *run)
{
	void *ret;

	assert(run->magic == ARENA_RUN_MAGIC);
	assert(run->nfree > 0);

	ret = arena_run_reg_alloc(run, bin);
	assert(ret != NULL);
	run->nfree--;

	return (ret);
}

/* Re-fill bin->runcur, then call arena_bin_malloc_easy(). */
static void *
arena_bin_malloc_hard(arena_t *arena, arena_bin_t *bin)
{

	bin->runcur = arena_bin_nonfull_run_get(arena, bin);
	if (bin->runcur == NULL)
		return (NULL);
	assert(bin->runcur->magic == ARENA_RUN_MAGIC);
	assert(bin->runcur->nfree > 0);

	return (arena_bin_malloc_easy(arena, bin, bin->runcur));
}

/*
 * Calculate bin->run_size such that it meets the following constraints:
 *
 *   *) bin->run_size >= min_run_size
 *   *) bin->run_size <= arena_maxclass
 *   *) bin->run_size <= RUN_MAX_SMALL
 *   *) run header overhead <= RUN_MAX_OVRHD (or header overhead relaxed).
 *
 * bin->nregs, bin->regs_mask_nelms, and bin->reg0_offset are
 * also calculated here, since these settings are all interdependent.
 */
static size_t
arena_bin_run_size_calc(arena_bin_t *bin, size_t min_run_size)
{
	size_t try_run_size, good_run_size;
	unsigned good_nregs, good_mask_nelms, good_reg0_offset;
	unsigned try_nregs, try_mask_nelms, try_reg0_offset;

	assert(min_run_size >= pagesize);
	assert(min_run_size <= arena_maxclass);
	assert(min_run_size <= RUN_MAX_SMALL);

	/*
	 * Calculate known-valid settings before entering the run_size
	 * expansion loop, so that the first part of the loop always copies
	 * valid settings.
	 *
	 * The do..while loop iteratively reduces the number of regions until
	 * the run header and the regions no longer overlap.  A closed formula
	 * would be quite messy, since there is an interdependency between the
	 * header's mask length and the number of regions.
	 */
	try_run_size = min_run_size;
	try_nregs = ((try_run_size - sizeof(arena_run_t)) / bin->reg_size)
	    + 1; /* Counter-act try_nregs-- in loop. */
	do {
		try_nregs--;
		try_mask_nelms = (try_nregs >> (SIZEOF_INT_2POW + 3)) +
		    ((try_nregs & ((1U << (SIZEOF_INT_2POW + 3)) - 1)) ? 1 : 0);
		try_reg0_offset = try_run_size - (try_nregs * bin->reg_size);
	} while (sizeof(arena_run_t) + (sizeof(unsigned) * (try_mask_nelms - 1))
	    > try_reg0_offset);

	/* run_size expansion loop. */
	do {
		/*
		 * Copy valid settings before trying more aggressive settings.
		 */
		good_run_size = try_run_size;
		good_nregs = try_nregs;
		good_mask_nelms = try_mask_nelms;
		good_reg0_offset = try_reg0_offset;

		/* Try more aggressive settings. */
		try_run_size += pagesize;
		try_nregs = ((try_run_size - sizeof(arena_run_t)) /
		    bin->reg_size) + 1; /* Counter-act try_nregs-- in loop. */
		do {
			try_nregs--;
			try_mask_nelms = (try_nregs >> (SIZEOF_INT_2POW + 3)) +
			    ((try_nregs & ((1U << (SIZEOF_INT_2POW + 3)) - 1)) ?
			    1 : 0);
			try_reg0_offset = try_run_size - (try_nregs *
			    bin->reg_size);
		} while (sizeof(arena_run_t) + (sizeof(unsigned) *
		    (try_mask_nelms - 1)) > try_reg0_offset);
	} while (try_run_size <= arena_maxclass && try_run_size <= RUN_MAX_SMALL
	    && RUN_MAX_OVRHD * (bin->reg_size << 3) > RUN_MAX_OVRHD_RELAX
	    && (try_reg0_offset << RUN_BFP) > RUN_MAX_OVRHD * try_run_size);

	assert(sizeof(arena_run_t) + (sizeof(unsigned) * (good_mask_nelms - 1))
	    <= good_reg0_offset);
	assert((good_mask_nelms << (SIZEOF_INT_2POW + 3)) >= good_nregs);

	/* Copy final settings. */
	bin->run_size = good_run_size;
	bin->nregs = good_nregs;
	bin->regs_mask_nelms = good_mask_nelms;
	bin->reg0_offset = good_reg0_offset;

	return (good_run_size);
}

#ifdef MALLOC_BALANCE
static inline void
arena_lock_balance(arena_t *arena)
{
	unsigned contention;

	contention = malloc_spin_lock(&arena->lock);
	if (narenas > 1) {
		/*
		 * Calculate the exponentially averaged contention for this
		 * arena.  Due to integer math always rounding down, this value
		 * decays somewhat faster then normal.
		 */
		arena->contention = (((uint64_t)arena->contention
		    * (uint64_t)((1U << BALANCE_ALPHA_INV_2POW)-1))
		    + (uint64_t)contention) >> BALANCE_ALPHA_INV_2POW;
		if (arena->contention >= opt_balance_threshold)
			arena_lock_balance_hard(arena);
	}
}

static void
arena_lock_balance_hard(arena_t *arena)
{
	uint32_t ind;

	arena->contention = 0;
#ifdef MALLOC_STATS
	arena->stats.nbalance++;
#endif
	ind = PRN(balance, narenas_2pow);
	if (arenas[ind] != NULL) {
#ifdef MOZ_MEMORY_WINDOWS
		TlsSetValue(tlsIndex, arenas[ind]);
#else
		arenas_map = arenas[ind];
#endif
	} else {
		malloc_spin_lock(&arenas_lock);
		if (arenas[ind] != NULL) {
#ifdef MOZ_MEMORY_WINDOWS
			TlsSetValue(tlsIndex, arenas[ind]);
#else
			arenas_map = arenas[ind];
#endif
		} else {
#ifdef MOZ_MEMORY_WINDOWS
			TlsSetValue(tlsIndex, arenas_extend(ind));
#else
			arenas_map = arenas_extend(ind);
#endif
		}
		malloc_spin_unlock(&arenas_lock);
	}
}
#endif

static inline void *
arena_malloc_small(arena_t *arena, size_t size, bool zero)
{
	void *ret;
	arena_bin_t *bin;
	arena_run_t *run;

	if (size < small_min) {
		/* Tiny. */
		size = pow2_ceil(size);
		bin = &arena->bins[ffs((int)(size >> (TINY_MIN_2POW +
		    1)))];
#if (!defined(NDEBUG) || defined(MALLOC_STATS))
		/*
		 * Bin calculation is always correct, but we may need
		 * to fix size for the purposes of assertions and/or
		 * stats accuracy.
		 */
		if (size < (1U << TINY_MIN_2POW))
			size = (1U << TINY_MIN_2POW);
#endif
	} else if (size <= small_max) {
		/* Quantum-spaced. */
		size = QUANTUM_CEILING(size);
		bin = &arena->bins[ntbins + (size >> opt_quantum_2pow)
		    - 1];
	} else {
		/* Sub-page. */
		size = pow2_ceil(size);
		bin = &arena->bins[ntbins + nqbins
		    + (ffs((int)(size >> opt_small_max_2pow)) - 2)];
	}
	assert(size == bin->reg_size);

#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	if ((run = bin->runcur) != NULL && run->nfree > 0)
		ret = arena_bin_malloc_easy(arena, bin, run);
	else
		ret = arena_bin_malloc_hard(arena, bin);

	if (ret == NULL) {
		malloc_spin_unlock(&arena->lock);
		return (NULL);
	}

#ifdef MALLOC_STATS
	bin->stats.nrequests++;
	arena->stats.nmalloc_small++;
	arena->stats.allocated_small += size;
#endif
	malloc_spin_unlock(&arena->lock);

	VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, zero);
	if (zero == false) {
#ifdef MALLOC_FILL
		if (opt_junk)
			memset(ret, 0xa5, size);
		else if (opt_zero)
			memset(ret, 0, size);
#endif
	} else
		memset(ret, 0, size);

	return (ret);
}

static void *
arena_malloc_large(arena_t *arena, size_t size, bool zero)
{
	void *ret;

	/* Large allocation. */
	size = PAGE_CEILING(size);
#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	ret = (void *)arena_run_alloc(arena, NULL, size, true, zero);
	if (ret == NULL) {
		malloc_spin_unlock(&arena->lock);
		return (NULL);
	}
#ifdef MALLOC_STATS
	arena->stats.nmalloc_large++;
	arena->stats.allocated_large += size;
#endif
	malloc_spin_unlock(&arena->lock);

	VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, zero);
	if (zero == false) {
#ifdef MALLOC_FILL
		if (opt_junk)
			memset(ret, 0xa5, size);
		else if (opt_zero)
			memset(ret, 0, size);
#endif
	}

	return (ret);
}

static inline void *
arena_malloc(arena_t *arena, size_t size, bool zero)
{

	assert(arena != NULL);
	assert(arena->magic == ARENA_MAGIC);
	assert(size != 0);
	assert(QUANTUM_CEILING(size) <= arena_maxclass);

	if (size <= bin_maxclass) {
		return (arena_malloc_small(arena, size, zero));
	} else
		return (arena_malloc_large(arena, size, zero));
}

static inline void *
imalloc(size_t size)
{

	assert(size != 0);

	if (size <= arena_maxclass)
		return (arena_malloc(choose_arena(), size, false));
	else
		return (huge_malloc(size, false));
}

static inline void *
icalloc(size_t size)
{

	if (size <= arena_maxclass)
		return (arena_malloc(choose_arena(), size, true));
	else
		return (huge_malloc(size, true));
}

/* Only handles large allocations that require more than page alignment. */
static void *
arena_palloc(arena_t *arena, size_t alignment, size_t size, size_t alloc_size)
{
	void *ret;
	size_t offset;
	arena_chunk_t *chunk;

	assert((size & pagesize_mask) == 0);
	assert((alignment & pagesize_mask) == 0);

#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	ret = (void *)arena_run_alloc(arena, NULL, alloc_size, true, false);
	if (ret == NULL) {
		malloc_spin_unlock(&arena->lock);
		return (NULL);
	}

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ret);

	offset = (uintptr_t)ret & (alignment - 1);
	assert((offset & pagesize_mask) == 0);
	assert(offset < alloc_size);
	if (offset == 0)
		arena_run_trim_tail(arena, chunk, (arena_run_t*)ret, alloc_size, size, false);
	else {
		size_t leadsize, trailsize;

		leadsize = alignment - offset;
		if (leadsize > 0) {
			arena_run_trim_head(arena, chunk, (arena_run_t*)ret, alloc_size,
			    alloc_size - leadsize);
			ret = (void *)((uintptr_t)ret + leadsize);
		}

		trailsize = alloc_size - leadsize - size;
		if (trailsize != 0) {
			/* Trim trailing space. */
			assert(trailsize < alloc_size);
			arena_run_trim_tail(arena, chunk, (arena_run_t*)ret, size + trailsize,
			    size, false);
		}
	}

#ifdef MALLOC_STATS
	arena->stats.nmalloc_large++;
	arena->stats.allocated_large += size;
#endif
	malloc_spin_unlock(&arena->lock);

	VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, false);
#ifdef MALLOC_FILL
	if (opt_junk)
		memset(ret, 0xa5, size);
	else if (opt_zero)
		memset(ret, 0, size);
#endif
	return (ret);
}

static inline void *
ipalloc(size_t alignment, size_t size)
{
	void *ret;
	size_t ceil_size;

	/*
	 * Round size up to the nearest multiple of alignment.
	 *
	 * This done, we can take advantage of the fact that for each small
	 * size class, every object is aligned at the smallest power of two
	 * that is non-zero in the base two representation of the size.  For
	 * example:
	 *
	 *   Size |   Base 2 | Minimum alignment
	 *   -----+----------+------------------
	 *     96 |  1100000 |  32
	 *    144 | 10100000 |  32
	 *    192 | 11000000 |  64
	 *
	 * Depending on runtime settings, it is possible that arena_malloc()
	 * will further round up to a power of two, but that never causes
	 * correctness issues.
	 */
	ceil_size = (size + (alignment - 1)) & (-alignment);
	/*
	 * (ceil_size < size) protects against the combination of maximal
	 * alignment and size greater than maximal alignment.
	 */
	if (ceil_size < size) {
		/* size_t overflow. */
		return (NULL);
	}

	if (ceil_size <= pagesize || (alignment <= pagesize
	    && ceil_size <= arena_maxclass))
		ret = arena_malloc(choose_arena(), ceil_size, false);
	else {
		size_t run_size;

		/*
		 * We can't achieve sub-page alignment, so round up alignment
		 * permanently; it makes later calculations simpler.
		 */
		alignment = PAGE_CEILING(alignment);
		ceil_size = PAGE_CEILING(size);
		/*
		 * (ceil_size < size) protects against very large sizes within
		 * pagesize of SIZE_T_MAX.
		 *
		 * (ceil_size + alignment < ceil_size) protects against the
		 * combination of maximal alignment and ceil_size large enough
		 * to cause overflow.  This is similar to the first overflow
		 * check above, but it needs to be repeated due to the new
		 * ceil_size value, which may now be *equal* to maximal
		 * alignment, whereas before we only detected overflow if the
		 * original size was *greater* than maximal alignment.
		 */
		if (ceil_size < size || ceil_size + alignment < ceil_size) {
			/* size_t overflow. */
			return (NULL);
		}

		/*
		 * Calculate the size of the over-size run that arena_palloc()
		 * would need to allocate in order to guarantee the alignment.
		 */
		if (ceil_size >= alignment)
			run_size = ceil_size + alignment - pagesize;
		else {
			/*
			 * It is possible that (alignment << 1) will cause
			 * overflow, but it doesn't matter because we also
			 * subtract pagesize, which in the case of overflow
			 * leaves us with a very large run_size.  That causes
			 * the first conditional below to fail, which means
			 * that the bogus run_size value never gets used for
			 * anything important.
			 */
			run_size = (alignment << 1) - pagesize;
		}

		if (run_size <= arena_maxclass) {
			ret = arena_palloc(choose_arena(), alignment, ceil_size,
			    run_size);
		} else if (alignment <= chunksize)
			ret = huge_malloc(ceil_size, false);
		else
			ret = huge_palloc(alignment, ceil_size);
	}

	assert(((uintptr_t)ret & (alignment - 1)) == 0);
	return (ret);
}

/* Return the size of the allocation pointed to by ptr. */
static size_t
arena_salloc(const void *ptr)
{
	size_t ret;
	arena_chunk_t *chunk;
	size_t pageind, mapbits;

	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	pageind = (((uintptr_t)ptr - (uintptr_t)chunk) >> pagesize_2pow);
	mapbits = chunk->map[pageind].bits;
	assert((mapbits & CHUNK_MAP_ALLOCATED) != 0);
	if ((mapbits & CHUNK_MAP_LARGE) == 0) {
		arena_run_t *run = (arena_run_t *)(mapbits & ~pagesize_mask);
		assert(run->magic == ARENA_RUN_MAGIC);
		ret = run->bin->reg_size;
	} else {
		ret = mapbits & ~pagesize_mask;
		assert(ret != 0);
	}

	return (ret);
}

#if (defined(MALLOC_VALIDATE) || defined(MOZ_MEMORY_DARWIN))
/*
 * Validate ptr before assuming that it points to an allocation.  Currently,
 * the following validation is performed:
 *
 * + Check that ptr is not NULL.
 *
 * + Check that ptr lies within a mapped chunk.
 */
static inline size_t
isalloc_validate(const void *ptr)
{
	arena_chunk_t *chunk;

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk == NULL)
		return (0);

	if (malloc_rtree_get(chunk_rtree, (uintptr_t)chunk) == NULL)
		return (0);

	if (chunk != ptr) {
		assert(chunk->arena->magic == ARENA_MAGIC);
		return (arena_salloc(ptr));
	} else {
		size_t ret;
		extent_node_t *node;
		extent_node_t key;

		/* Chunk. */
		key.addr = (void *)chunk;
		malloc_mutex_lock(&huge_mtx);
		node = extent_tree_ad_search(&huge, &key);
		if (node != NULL)
			ret = node->size;
		else
			ret = 0;
		malloc_mutex_unlock(&huge_mtx);
		return (ret);
	}
}
#endif

static inline size_t
isalloc(const void *ptr)
{
	size_t ret;
	arena_chunk_t *chunk;

	assert(ptr != NULL);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk != ptr) {
		/* Region. */
		assert(chunk->arena->magic == ARENA_MAGIC);

		ret = arena_salloc(ptr);
	} else {
		extent_node_t *node, key;

		/* Chunk (huge allocation). */

		malloc_mutex_lock(&huge_mtx);

		/* Extract from tree of huge allocations. */
		key.addr = __DECONST(void *, ptr);
		node = extent_tree_ad_search(&huge, &key);
		assert(node != NULL);

		ret = node->size;

		malloc_mutex_unlock(&huge_mtx);
	}

	return (ret);
}

static inline void
arena_dalloc_small(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    arena_chunk_map_t *mapelm)
{
	arena_run_t *run;
	arena_bin_t *bin;
	size_t size;

	run = (arena_run_t *)(mapelm->bits & ~pagesize_mask);
	assert(run->magic == ARENA_RUN_MAGIC);
	bin = run->bin;
	size = bin->reg_size;

#ifdef MALLOC_FILL
	if (opt_junk)
		memset(ptr, 0x5a, size);
#endif

	arena_run_reg_dalloc(run, bin, ptr, size);
	run->nfree++;

	if (run->nfree == bin->nregs) {
		/* Deallocate run. */
		if (run == bin->runcur)
			bin->runcur = NULL;
		else if (bin->nregs != 1) {
			size_t run_pageind = (((uintptr_t)run -
			    (uintptr_t)chunk)) >> pagesize_2pow;
			arena_chunk_map_t *run_mapelm =
			    &chunk->map[run_pageind];
			/*
			 * This block's conditional is necessary because if the
			 * run only contains one region, then it never gets
			 * inserted into the non-full runs tree.
			 */
			assert(arena_run_tree_search(&bin->runs, run_mapelm) ==
			    run_mapelm);
			arena_run_tree_remove(&bin->runs, run_mapelm);
		}
#ifdef MALLOC_DEBUG
		run->magic = 0;
#endif
		VALGRIND_FREELIKE_BLOCK(run, 0);
		arena_run_dalloc(arena, run, true);
#ifdef MALLOC_STATS
		bin->stats.curruns--;
#endif
	} else if (run->nfree == 1 && run != bin->runcur) {
		/*
		 * Make sure that bin->runcur always refers to the lowest
		 * non-full run, if one exists.
		 */
		if (bin->runcur == NULL)
			bin->runcur = run;
		else if ((uintptr_t)run < (uintptr_t)bin->runcur) {
			/* Switch runcur. */
			if (bin->runcur->nfree > 0) {
				arena_chunk_t *runcur_chunk =
				    (arena_chunk_t*)CHUNK_ADDR2BASE(bin->runcur);
				size_t runcur_pageind =
				    (((uintptr_t)bin->runcur -
				    (uintptr_t)runcur_chunk)) >> pagesize_2pow;
				arena_chunk_map_t *runcur_mapelm =
				    &runcur_chunk->map[runcur_pageind];

				/* Insert runcur. */
				assert(arena_run_tree_search(&bin->runs,
				    runcur_mapelm) == NULL);
				arena_run_tree_insert(&bin->runs,
				    runcur_mapelm);
			}
			bin->runcur = run;
		} else {
			size_t run_pageind = (((uintptr_t)run -
			    (uintptr_t)chunk)) >> pagesize_2pow;
			arena_chunk_map_t *run_mapelm =
			    &chunk->map[run_pageind];

			assert(arena_run_tree_search(&bin->runs, run_mapelm) ==
			    NULL);
			arena_run_tree_insert(&bin->runs, run_mapelm);
		}
	}
#ifdef MALLOC_STATS
	arena->stats.allocated_small -= size;
	arena->stats.ndalloc_small++;
#endif
}

static void
arena_dalloc_large(arena_t *arena, arena_chunk_t *chunk, void *ptr)
{
	/* Large allocation. */
	malloc_spin_lock(&arena->lock);

#ifdef MALLOC_FILL
#ifndef MALLOC_STATS
	if (opt_junk)
#endif
#endif
	{
		size_t pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >>
		    pagesize_2pow;
		size_t size = chunk->map[pageind].bits & ~pagesize_mask;

#ifdef MALLOC_FILL
#ifdef MALLOC_STATS
		if (opt_junk)
#endif
			memset(ptr, 0x5a, size);
#endif
#ifdef MALLOC_STATS
		arena->stats.allocated_large -= size;
#endif
	}
#ifdef MALLOC_STATS
	arena->stats.ndalloc_large++;
#endif

	arena_run_dalloc(arena, (arena_run_t *)ptr, true);
	malloc_spin_unlock(&arena->lock);
}

static inline void
arena_dalloc(arena_t *arena, arena_chunk_t *chunk, void *ptr)
{
	size_t pageind;
	arena_chunk_map_t *mapelm;

	assert(arena != NULL);
	assert(arena->magic == ARENA_MAGIC);
	assert(chunk->arena == arena);
	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	pageind = (((uintptr_t)ptr - (uintptr_t)chunk) >> pagesize_2pow);
	mapelm = &chunk->map[pageind];
	assert((mapelm->bits & CHUNK_MAP_ALLOCATED) != 0);
	if ((mapelm->bits & CHUNK_MAP_LARGE) == 0) {
		/* Small allocation. */
		malloc_spin_lock(&arena->lock);
		arena_dalloc_small(arena, chunk, ptr, mapelm);
		malloc_spin_unlock(&arena->lock);
	} else
		arena_dalloc_large(arena, chunk, ptr);
	VALGRIND_FREELIKE_BLOCK(ptr, 0);
}

static inline void
idalloc(void *ptr)
{
	arena_chunk_t *chunk;

	assert(ptr != NULL);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk != ptr)
		arena_dalloc(chunk->arena, chunk, ptr);
	else
		huge_dalloc(ptr);
}

static void
arena_ralloc_large_shrink(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t size, size_t oldsize)
{

	assert(size < oldsize);

	/*
	 * Shrink the run, and make trailing pages available for other
	 * allocations.
	 */
#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	arena_run_trim_tail(arena, chunk, (arena_run_t *)ptr, oldsize, size,
	    true);
#ifdef MALLOC_STATS
	arena->stats.allocated_large -= oldsize - size;
#endif
	malloc_spin_unlock(&arena->lock);
}

static bool
arena_ralloc_large_grow(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t size, size_t oldsize)
{
	size_t pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> pagesize_2pow;
	size_t npages = oldsize >> pagesize_2pow;

	assert(oldsize == (chunk->map[pageind].bits & ~pagesize_mask));

	/* Try to extend the run. */
	assert(size > oldsize);
#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	if (pageind + npages < chunk_npages && (chunk->map[pageind+npages].bits
	    & CHUNK_MAP_ALLOCATED) == 0 && (chunk->map[pageind+npages].bits &
	    ~pagesize_mask) >= size - oldsize) {
		/*
		 * The next run is available and sufficiently large.  Split the
		 * following run, then merge the first part with the existing
		 * allocation.
		 */
		arena_run_split(arena, (arena_run_t *)((uintptr_t)chunk +
		    ((pageind+npages) << pagesize_2pow)), size - oldsize, true,
		    false);

		chunk->map[pageind].bits = size | CHUNK_MAP_LARGE |
		    CHUNK_MAP_ALLOCATED;
		chunk->map[pageind+npages].bits = CHUNK_MAP_LARGE |
		    CHUNK_MAP_ALLOCATED;

#ifdef MALLOC_STATS
		arena->stats.allocated_large += size - oldsize;
#endif
		malloc_spin_unlock(&arena->lock);
		return (false);
	}
	malloc_spin_unlock(&arena->lock);

	return (true);
}

/*
 * Try to resize a large allocation, in order to avoid copying.  This will
 * always fail if growing an object, and the following run is already in use.
 */
static bool
arena_ralloc_large(void *ptr, size_t size, size_t oldsize)
{
	size_t psize;

	psize = PAGE_CEILING(size);
	if (psize == oldsize) {
		/* Same size class. */
#ifdef MALLOC_FILL
		if (opt_junk && size < oldsize) {
			memset((void *)((uintptr_t)ptr + size), 0x5a, oldsize -
			    size);
		}
#endif
		return (false);
	} else {
		arena_chunk_t *chunk;
		arena_t *arena;

		chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
		arena = chunk->arena;
		assert(arena->magic == ARENA_MAGIC);

		if (psize < oldsize) {
#ifdef MALLOC_FILL
			/* Fill before shrinking in order avoid a race. */
			if (opt_junk) {
				memset((void *)((uintptr_t)ptr + size), 0x5a,
				    oldsize - size);
			}
#endif
			arena_ralloc_large_shrink(arena, chunk, ptr, psize,
			    oldsize);
			return (false);
		} else {
			bool ret = arena_ralloc_large_grow(arena, chunk, ptr,
			    psize, oldsize);
#ifdef MALLOC_FILL
			if (ret == false && opt_zero) {
				memset((void *)((uintptr_t)ptr + oldsize), 0,
				    size - oldsize);
			}
#endif
			return (ret);
		}
	}
}

static void *
arena_ralloc(void *ptr, size_t size, size_t oldsize)
{
	void *ret;
	size_t copysize;

	/* Try to avoid moving the allocation. */
	if (size < small_min) {
		if (oldsize < small_min &&
		    ffs((int)(pow2_ceil(size) >> (TINY_MIN_2POW + 1)))
		    == ffs((int)(pow2_ceil(oldsize) >> (TINY_MIN_2POW + 1))))
			goto IN_PLACE; /* Same size class. */
	} else if (size <= small_max) {
		if (oldsize >= small_min && oldsize <= small_max &&
		    (QUANTUM_CEILING(size) >> opt_quantum_2pow)
		    == (QUANTUM_CEILING(oldsize) >> opt_quantum_2pow))
			goto IN_PLACE; /* Same size class. */
	} else if (size <= bin_maxclass) {
		if (oldsize > small_max && oldsize <= bin_maxclass &&
		    pow2_ceil(size) == pow2_ceil(oldsize))
			goto IN_PLACE; /* Same size class. */
	} else if (oldsize > bin_maxclass && oldsize <= arena_maxclass) {
		assert(size > bin_maxclass);
		if (arena_ralloc_large(ptr, size, oldsize) == false)
			return (ptr);
	}

	/*
	 * If we get here, then size and oldsize are different enough that we
	 * need to move the object.  In that case, fall back to allocating new
	 * space and copying.
	 */
	ret = arena_malloc(choose_arena(), size, false);
	if (ret == NULL)
		return (NULL);

	/* Junk/zero-filling were already done by arena_malloc(). */
	copysize = (size < oldsize) ? size : oldsize;
#ifdef VM_COPY_MIN
	if (copysize >= VM_COPY_MIN)
		pages_copy(ret, ptr, copysize);
	else
#endif
		memcpy(ret, ptr, copysize);
	idalloc(ptr);
	return (ret);
IN_PLACE:
#ifdef MALLOC_FILL
	if (opt_junk && size < oldsize)
		memset((void *)((uintptr_t)ptr + size), 0x5a, oldsize - size);
	else if (opt_zero && size > oldsize)
		memset((void *)((uintptr_t)ptr + oldsize), 0, size - oldsize);
#endif
	return (ptr);
}

static inline void *
iralloc(void *ptr, size_t size)
{
	size_t oldsize;

	assert(ptr != NULL);
	assert(size != 0);

	oldsize = isalloc(ptr);

#ifndef MALLOC_VALGRIND
	if (size <= arena_maxclass)
		return (arena_ralloc(ptr, size, oldsize));
	else
		return (huge_ralloc(ptr, size, oldsize));
#else
	/*
	 * Valgrind does not provide a public interface for modifying an
	 * existing allocation, so use malloc/memcpy/free instead.
	 */
	{
		void *ret = imalloc(size);
		if (ret != NULL) {
			if (oldsize < size)
			    memcpy(ret, ptr, oldsize);
			else
			    memcpy(ret, ptr, size);
			idalloc(ptr);
		}
		return (ret);
	}
#endif
}

static bool
arena_new(arena_t *arena)
{
	unsigned i;
	arena_bin_t *bin;
	size_t pow2_size, prev_run_size;

	if (malloc_spin_init(&arena->lock))
		return (true);

#ifdef MALLOC_STATS
	memset(&arena->stats, 0, sizeof(arena_stats_t));
#endif

	arena->chunk_seq = 0;

	/* Initialize chunks. */
	arena_chunk_tree_dirty_new(&arena->chunks_dirty);
	arena->spare = NULL;

	arena->ndirty = 0;

	arena_avail_tree_new(&arena->runs_avail);

#ifdef MALLOC_BALANCE
	arena->contention = 0;
#endif

	/* Initialize bins. */
	prev_run_size = pagesize;

	/* (2^n)-spaced tiny bins. */
	for (i = 0; i < ntbins; i++) {
		bin = &arena->bins[i];
		bin->runcur = NULL;
		arena_run_tree_new(&bin->runs);

		bin->reg_size = (1U << (TINY_MIN_2POW + i));

		prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);

#ifdef MALLOC_STATS
		memset(&bin->stats, 0, sizeof(malloc_bin_stats_t));
#endif
	}

	/* Quantum-spaced bins. */
	for (; i < ntbins + nqbins; i++) {
		bin = &arena->bins[i];
		bin->runcur = NULL;
		arena_run_tree_new(&bin->runs);

		bin->reg_size = quantum * (i - ntbins + 1);

		pow2_size = pow2_ceil(quantum * (i - ntbins + 1));
		prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);

#ifdef MALLOC_STATS
		memset(&bin->stats, 0, sizeof(malloc_bin_stats_t));
#endif
	}

	/* (2^n)-spaced sub-page bins. */
	for (; i < ntbins + nqbins + nsbins; i++) {
		bin = &arena->bins[i];
		bin->runcur = NULL;
		arena_run_tree_new(&bin->runs);

		bin->reg_size = (small_max << (i - (ntbins + nqbins) + 1));

		prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);

#ifdef MALLOC_STATS
		memset(&bin->stats, 0, sizeof(malloc_bin_stats_t));
#endif
	}

#ifdef MALLOC_DEBUG
	arena->magic = ARENA_MAGIC;
#endif

	return (false);
}

/* Create a new arena and insert it into the arenas array at index ind. */
static arena_t *
arenas_extend(unsigned ind)
{
	arena_t *ret;

	/* Allocate enough space for trailing bins. */
	ret = (arena_t *)base_alloc(sizeof(arena_t)
	    + (sizeof(arena_bin_t) * (ntbins + nqbins + nsbins - 1)));
	if (ret != NULL && arena_new(ret) == false) {
		arenas[ind] = ret;
		return (ret);
	}
	/* Only reached if there is an OOM error. */

	/*
	 * OOM here is quite inconvenient to propagate, since dealing with it
	 * would require a check for failure in the fast path.  Instead, punt
	 * by using arenas[0].  In practice, this is an extremely unlikely
	 * failure.
	 */
	_malloc_message(_getprogname(),
	    ": (malloc) Error initializing arena\n", "", "");
	if (opt_abort)
		abort();

	return (arenas[0]);
}

/*
 * End arena.
 */
/******************************************************************************/
/*
 * Begin general internal functions.
 */

static void *
huge_malloc(size_t size, bool zero)
{
	void *ret;
	size_t csize;
#ifdef MALLOC_DECOMMIT
	size_t psize;
#endif
	extent_node_t *node;

	/* Allocate one or more contiguous chunks for this request. */

	csize = CHUNK_CEILING(size);
	if (csize == 0) {
		/* size is large enough to cause size_t wrap-around. */
		return (NULL);
	}

	/* Allocate an extent node with which to track the chunk. */
	node = base_node_alloc();
	if (node == NULL)
		return (NULL);

	ret = chunk_alloc(csize, zero, true);
	if (ret == NULL) {
		base_node_dealloc(node);
		return (NULL);
	}

	/* Insert node into huge. */
	node->addr = ret;
#ifdef MALLOC_DECOMMIT
	psize = PAGE_CEILING(size);
	node->size = psize;
#else
	node->size = csize;
#endif

	malloc_mutex_lock(&huge_mtx);
	extent_tree_ad_insert(&huge, node);
#ifdef MALLOC_STATS
	huge_nmalloc++;
#  ifdef MALLOC_DECOMMIT
	huge_allocated += psize;
#  else
	huge_allocated += csize;
#  endif
#endif
	malloc_mutex_unlock(&huge_mtx);

#ifdef MALLOC_DECOMMIT
	if (csize - psize > 0)
		pages_decommit((void *)((uintptr_t)ret + psize), csize - psize);
#endif

#ifdef MALLOC_DECOMMIT
	VALGRIND_MALLOCLIKE_BLOCK(ret, psize, 0, zero);
#else
	VALGRIND_MALLOCLIKE_BLOCK(ret, csize, 0, zero);
#endif

#ifdef MALLOC_FILL
	if (zero == false) {
		if (opt_junk)
#  ifdef MALLOC_DECOMMIT
			memset(ret, 0xa5, psize);
#  else
			memset(ret, 0xa5, csize);
#  endif
		else if (opt_zero)
#  ifdef MALLOC_DECOMMIT
			memset(ret, 0, psize);
#  else
			memset(ret, 0, csize);
#  endif
	}
#endif

	return (ret);
}

/* Only handles large allocations that require more than chunk alignment. */
static void *
huge_palloc(size_t alignment, size_t size)
{
	void *ret;
	size_t alloc_size, chunk_size, offset;
#ifdef MALLOC_DECOMMIT
	size_t psize;
#endif
	extent_node_t *node;
	int pfd;

	/*
	 * This allocation requires alignment that is even larger than chunk
	 * alignment.  This means that huge_malloc() isn't good enough.
	 *
	 * Allocate almost twice as many chunks as are demanded by the size or
	 * alignment, in order to assure the alignment can be achieved, then
	 * unmap leading and trailing chunks.
	 */
	assert(alignment >= chunksize);

	chunk_size = CHUNK_CEILING(size);

	if (size >= alignment)
		alloc_size = chunk_size + alignment - chunksize;
	else
		alloc_size = (alignment << 1) - chunksize;

	/* Allocate an extent node with which to track the chunk. */
	node = base_node_alloc();
	if (node == NULL)
		return (NULL);

	/*
	 * Windows requires that there be a 1:1 mapping between VM
	 * allocation/deallocation operations.  Therefore, take care here to
	 * acquire the final result via one mapping operation.
	 *
	 * The MALLOC_PAGEFILE code also benefits from this mapping algorithm,
	 * since it reduces the number of page files.
	 */
#ifdef MALLOC_PAGEFILE
	if (opt_pagefile) {
		pfd = pagefile_init(size);
		if (pfd == -1)
			return (NULL);
	} else
#endif
		pfd = -1;
#ifdef JEMALLOC_USES_MAP_ALIGN
		ret = pages_map_align(chunk_size, pfd, alignment);
#else
	do {
		void *over;

		over = chunk_alloc(alloc_size, false, false);
		if (over == NULL) {
			base_node_dealloc(node);
			ret = NULL;
			goto RETURN;
		}

		offset = (uintptr_t)over & (alignment - 1);
		assert((offset & chunksize_mask) == 0);
		assert(offset < alloc_size);
		ret = (void *)((uintptr_t)over + offset);
		chunk_dealloc(over, alloc_size);
		ret = pages_map(ret, chunk_size, pfd);
		/*
		 * Failure here indicates a race with another thread, so try
		 * again.
		 */
	} while (ret == NULL);
#endif
	/* Insert node into huge. */
	node->addr = ret;
#ifdef MALLOC_DECOMMIT
	psize = PAGE_CEILING(size);
	node->size = psize;
#else
	node->size = chunk_size;
#endif

	malloc_mutex_lock(&huge_mtx);
	extent_tree_ad_insert(&huge, node);
#ifdef MALLOC_STATS
	huge_nmalloc++;
#  ifdef MALLOC_DECOMMIT
	huge_allocated += psize;
#  else
	huge_allocated += chunk_size;
#  endif
#endif
	malloc_mutex_unlock(&huge_mtx);

#ifdef MALLOC_DECOMMIT
	if (chunk_size - psize > 0) {
		pages_decommit((void *)((uintptr_t)ret + psize),
		    chunk_size - psize);
	}
#endif

#ifdef MALLOC_DECOMMIT
	VALGRIND_MALLOCLIKE_BLOCK(ret, psize, 0, false);
#else
	VALGRIND_MALLOCLIKE_BLOCK(ret, chunk_size, 0, false);
#endif

#ifdef MALLOC_FILL
	if (opt_junk)
#  ifdef MALLOC_DECOMMIT
		memset(ret, 0xa5, psize);
#  else
		memset(ret, 0xa5, chunk_size);
#  endif
	else if (opt_zero)
#  ifdef MALLOC_DECOMMIT
		memset(ret, 0, psize);
#  else
		memset(ret, 0, chunk_size);
#  endif
#endif

RETURN:
#ifdef MALLOC_PAGEFILE
	if (pfd != -1)
		pagefile_close(pfd);
#endif
	return (ret);
}

static void *
huge_ralloc(void *ptr, size_t size, size_t oldsize)
{
	void *ret;
	size_t copysize;

	/* Avoid moving the allocation if the size class would not change. */

	if (oldsize > arena_maxclass &&
	    CHUNK_CEILING(size) == CHUNK_CEILING(oldsize)) {
#ifdef MALLOC_DECOMMIT
		size_t psize = PAGE_CEILING(size);
#endif
#ifdef MALLOC_FILL
		if (opt_junk && size < oldsize) {
			memset((void *)((uintptr_t)ptr + size), 0x5a, oldsize
			    - size);
		}
#endif
#ifdef MALLOC_DECOMMIT
		if (psize < oldsize) {
			extent_node_t *node, key;

			pages_decommit((void *)((uintptr_t)ptr + psize),
			    oldsize - psize);

			/* Update recorded size. */
			malloc_mutex_lock(&huge_mtx);
			key.addr = __DECONST(void *, ptr);
			node = extent_tree_ad_search(&huge, &key);
			assert(node != NULL);
			assert(node->size == oldsize);
#  ifdef MALLOC_STATS
			huge_allocated -= oldsize - psize;
#  endif
			node->size = psize;
			malloc_mutex_unlock(&huge_mtx);
		} else if (psize > oldsize) {
			extent_node_t *node, key;

			pages_commit((void *)((uintptr_t)ptr + oldsize),
			    psize - oldsize);

			/* Update recorded size. */
			malloc_mutex_lock(&huge_mtx);
			key.addr = __DECONST(void *, ptr);
			node = extent_tree_ad_search(&huge, &key);
			assert(node != NULL);
			assert(node->size == oldsize);
#  ifdef MALLOC_STATS
			huge_allocated += psize - oldsize;
#  endif
			node->size = psize;
			malloc_mutex_unlock(&huge_mtx);
		}
#endif
#ifdef MALLOC_FILL
		if (opt_zero && size > oldsize) {
			memset((void *)((uintptr_t)ptr + oldsize), 0, size
			    - oldsize);
		}
#endif
		return (ptr);
	}

	/*
	 * If we get here, then size and oldsize are different enough that we
	 * need to use a different size class.  In that case, fall back to
	 * allocating new space and copying.
	 */
	ret = huge_malloc(size, false);
	if (ret == NULL)
		return (NULL);

	copysize = (size < oldsize) ? size : oldsize;
#ifdef VM_COPY_MIN
	if (copysize >= VM_COPY_MIN)
		pages_copy(ret, ptr, copysize);
	else
#endif
		memcpy(ret, ptr, copysize);
	idalloc(ptr);
	return (ret);
}

static void
huge_dalloc(void *ptr)
{
	extent_node_t *node, key;

	malloc_mutex_lock(&huge_mtx);

	/* Extract from tree of huge allocations. */
	key.addr = ptr;
	node = extent_tree_ad_search(&huge, &key);
	assert(node != NULL);
	assert(node->addr == ptr);
	extent_tree_ad_remove(&huge, node);

#ifdef MALLOC_STATS
	huge_ndalloc++;
	huge_allocated -= node->size;
#endif

	malloc_mutex_unlock(&huge_mtx);

	/* Unmap chunk. */
#ifdef MALLOC_FILL
	if (opt_junk)
		memset(node->addr, 0x5a, node->size);
#endif
#ifdef MALLOC_DECOMMIT
	chunk_dealloc(node->addr, CHUNK_CEILING(node->size));
#else
	chunk_dealloc(node->addr, node->size);
#endif
	VALGRIND_FREELIKE_BLOCK(node->addr, 0);

	base_node_dealloc(node);
}

#ifdef MOZ_MEMORY_BSD
static inline unsigned
malloc_ncpus(void)
{
	unsigned ret;
	int mib[2];
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(ret);
	if (sysctl(mib, 2, &ret, &len, (void *) 0, 0) == -1) {
		/* Error. */
		return (1);
	}

	return (ret);
}
#elif (defined(MOZ_MEMORY_LINUX))
#include <fcntl.h>

static inline unsigned
malloc_ncpus(void)
{
	unsigned ret;
	int fd, nread, column;
	char buf[1024];
	static const char matchstr[] = "processor\t:";
	int i;

	/*
	 * sysconf(3) would be the preferred method for determining the number
	 * of CPUs, but it uses malloc internally, which causes untennable
	 * recursion during malloc initialization.
	 */
	fd = open("/proc/cpuinfo", O_RDONLY);
	if (fd == -1)
		return (1); /* Error. */
	/*
	 * Count the number of occurrences of matchstr at the beginnings of
	 * lines.  This treats hyperthreaded CPUs as multiple processors.
	 */
	column = 0;
	ret = 0;
	while (true) {
		nread = read(fd, &buf, sizeof(buf));
		if (nread <= 0)
			break; /* EOF or error. */
		for (i = 0;i < nread;i++) {
			char c = buf[i];
			if (c == '\n')
				column = 0;
			else if (column != -1) {
				if (c == matchstr[column]) {
					column++;
					if (column == sizeof(matchstr) - 1) {
						column = -1;
						ret++;
					}
				} else
					column = -1;
			}
		}
	}

	if (ret == 0)
		ret = 1; /* Something went wrong in the parser. */
	close(fd);

	return (ret);
}
#elif (defined(MOZ_MEMORY_DARWIN))
#include <mach/mach_init.h>
#include <mach/mach_host.h>

static inline unsigned
malloc_ncpus(void)
{
	kern_return_t error;
	natural_t n;
	processor_info_array_t pinfo;
	mach_msg_type_number_t pinfocnt;

	error = host_processor_info(mach_host_self(), PROCESSOR_BASIC_INFO,
				    &n, &pinfo, &pinfocnt);
	if (error != KERN_SUCCESS)
		return (1); /* Error. */
	else
		return (n);
}
#elif (defined(MOZ_MEMORY_SOLARIS))

static inline unsigned
malloc_ncpus(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}
#else
static inline unsigned
malloc_ncpus(void)
{

	/*
	 * We lack a way to determine the number of CPUs on this platform, so
	 * assume 1 CPU.
	 */
	return (1);
}
#endif

static void
malloc_print_stats(void)
{

	if (opt_print_stats) {
		char s[UMAX2S_BUFSIZE];
		_malloc_message("___ Begin malloc statistics ___\n", "", "",
		    "");
		_malloc_message("Assertions ",
#ifdef NDEBUG
		    "disabled",
#else
		    "enabled",
#endif
		    "\n", "");
		_malloc_message("Boolean MALLOC_OPTIONS: ",
		    opt_abort ? "A" : "a", "", "");
#ifdef MALLOC_FILL
		_malloc_message(opt_junk ? "J" : "j", "", "", "");
#endif
#ifdef MALLOC_PAGEFILE
		_malloc_message(opt_pagefile ? "o" : "O", "", "", "");
#endif
		_malloc_message("P", "", "", "");
#ifdef MALLOC_UTRACE
		_malloc_message(opt_utrace ? "U" : "u", "", "", "");
#endif
#ifdef MALLOC_SYSV
		_malloc_message(opt_sysv ? "V" : "v", "", "", "");
#endif
#ifdef MALLOC_XMALLOC
		_malloc_message(opt_xmalloc ? "X" : "x", "", "", "");
#endif
#ifdef MALLOC_FILL
		_malloc_message(opt_zero ? "Z" : "z", "", "", "");
#endif
		_malloc_message("\n", "", "", "");

		_malloc_message("CPUs: ", umax2s(ncpus, s), "\n", "");
		_malloc_message("Max arenas: ", umax2s(narenas, s), "\n", "");
#ifdef MALLOC_BALANCE
		_malloc_message("Arena balance threshold: ",
		    umax2s(opt_balance_threshold, s), "\n", "");
#endif
		_malloc_message("Pointer size: ", umax2s(sizeof(void *), s),
		    "\n", "");
		_malloc_message("Quantum size: ", umax2s(quantum, s), "\n", "");
		_malloc_message("Max small size: ", umax2s(small_max, s), "\n",
		    "");
		_malloc_message("Max dirty pages per arena: ",
		    umax2s(opt_dirty_max, s), "\n", "");

		_malloc_message("Chunk size: ", umax2s(chunksize, s), "", "");
		_malloc_message(" (2^", umax2s(opt_chunk_2pow, s), ")\n", "");

#ifdef MALLOC_STATS
		{
			size_t allocated, mapped;
#ifdef MALLOC_BALANCE
			uint64_t nbalance = 0;
#endif
			unsigned i;
			arena_t *arena;

			/* Calculate and print allocated/mapped stats. */

			/* arenas. */
			for (i = 0, allocated = 0; i < narenas; i++) {
				if (arenas[i] != NULL) {
					malloc_spin_lock(&arenas[i]->lock);
					allocated +=
					    arenas[i]->stats.allocated_small;
					allocated +=
					    arenas[i]->stats.allocated_large;
#ifdef MALLOC_BALANCE
					nbalance += arenas[i]->stats.nbalance;
#endif
					malloc_spin_unlock(&arenas[i]->lock);
				}
			}

			/* huge/base. */
			malloc_mutex_lock(&huge_mtx);
			allocated += huge_allocated;
			mapped = stats_chunks.curchunks * chunksize;
			malloc_mutex_unlock(&huge_mtx);

			malloc_mutex_lock(&base_mtx);
			mapped += base_mapped;
			malloc_mutex_unlock(&base_mtx);

#ifdef MOZ_MEMORY_WINDOWS
			malloc_printf("Allocated: %lu, mapped: %lu\n",
			    allocated, mapped);
#else
			malloc_printf("Allocated: %zu, mapped: %zu\n",
			    allocated, mapped);
#endif

			malloc_mutex_lock(&reserve_mtx);
			malloc_printf("Reserve:    min          "
			    "cur          max\n");
#ifdef MOZ_MEMORY_WINDOWS
			malloc_printf("   %12lu %12lu %12lu\n",
			    CHUNK_CEILING(reserve_min) >> opt_chunk_2pow,
			    reserve_cur >> opt_chunk_2pow,
			    reserve_max >> opt_chunk_2pow);
#else
			malloc_printf("   %12zu %12zu %12zu\n",
			    CHUNK_CEILING(reserve_min) >> opt_chunk_2pow,
			    reserve_cur >> opt_chunk_2pow,
			    reserve_max >> opt_chunk_2pow);
#endif
			malloc_mutex_unlock(&reserve_mtx);

#ifdef MALLOC_BALANCE
			malloc_printf("Arena balance reassignments: %llu\n",
			    nbalance);
#endif

			/* Print chunk stats. */
			{
				chunk_stats_t chunks_stats;

				malloc_mutex_lock(&huge_mtx);
				chunks_stats = stats_chunks;
				malloc_mutex_unlock(&huge_mtx);

				malloc_printf("chunks: nchunks   "
				    "highchunks    curchunks\n");
				malloc_printf("  %13llu%13lu%13lu\n",
				    chunks_stats.nchunks,
				    chunks_stats.highchunks,
				    chunks_stats.curchunks);
			}

			/* Print chunk stats. */
			malloc_printf(
			    "huge: nmalloc      ndalloc    allocated\n");
#ifdef MOZ_MEMORY_WINDOWS
			malloc_printf(" %12llu %12llu %12lu\n",
			    huge_nmalloc, huge_ndalloc, huge_allocated);
#else
			malloc_printf(" %12llu %12llu %12zu\n",
			    huge_nmalloc, huge_ndalloc, huge_allocated);
#endif
			/* Print stats for each arena. */
			for (i = 0; i < narenas; i++) {
				arena = arenas[i];
				if (arena != NULL) {
					malloc_printf(
					    "\narenas[%u]:\n", i);
					malloc_spin_lock(&arena->lock);
					stats_print(arena);
					malloc_spin_unlock(&arena->lock);
				}
			}
		}
#endif /* #ifdef MALLOC_STATS */
		_malloc_message("--- End malloc statistics ---\n", "", "", "");
	}
}

/*
 * FreeBSD's pthreads implementation calls malloc(3), so the malloc
 * implementation has to take pains to avoid infinite recursion during
 * initialization.
 */
#if (defined(MOZ_MEMORY_WINDOWS) || defined(MOZ_MEMORY_DARWIN)) && !defined(MOZ_MEMORY_WINCE)
#define	malloc_init() false
#else
static inline bool
malloc_init(void)
{

	if (malloc_initialized == false)
		return (malloc_init_hard());

	return (false);
}
#endif

#if !defined(MOZ_MEMORY_WINDOWS) || defined(MOZ_MEMORY_WINCE) 
static
#endif
bool
je_malloc_init_hard(void)
{
	unsigned i;
	char buf[PATH_MAX + 1];
	const char *opts;
	long result;
#ifndef MOZ_MEMORY_WINDOWS
	int linklen;
#endif

#ifndef MOZ_MEMORY_WINDOWS
	malloc_mutex_lock(&init_lock);
#endif

	if (malloc_initialized) {
		/*
		 * Another thread initialized the allocator before this one
		 * acquired init_lock.
		 */
#ifndef MOZ_MEMORY_WINDOWS
		malloc_mutex_unlock(&init_lock);
#endif
		return (false);
	}

#ifdef MOZ_MEMORY_WINDOWS
	/* get a thread local storage index */
	tlsIndex = TlsAlloc();
#endif

	/* Get page size and number of CPUs */
#ifdef MOZ_MEMORY_WINDOWS
	{
		SYSTEM_INFO info;

		GetSystemInfo(&info);
		result = info.dwPageSize;

		pagesize = (unsigned) result;

		ncpus = info.dwNumberOfProcessors;
	}
#else
	ncpus = malloc_ncpus();

	result = sysconf(_SC_PAGESIZE);
	assert(result != -1);

	pagesize = (unsigned) result;
#endif

	/*
	 * We assume that pagesize is a power of 2 when calculating
	 * pagesize_mask and pagesize_2pow.
	 */
	assert(((result - 1) & result) == 0);
	pagesize_mask = result - 1;
	pagesize_2pow = ffs((int)result) - 1;

#ifdef MALLOC_PAGEFILE
	/*
	 * Determine where to create page files.  It is insufficient to
	 * unconditionally use P_tmpdir (typically "/tmp"), since for some
	 * operating systems /tmp is a separate filesystem that is rather small.
	 * Therefore prefer, in order, the following locations:
	 *
	 * 1) MALLOC_TMPDIR
	 * 2) TMPDIR
	 * 3) P_tmpdir
	 */
	{
		char *s;
		size_t slen;
		static const char suffix[] = "/jemalloc.XXXXXX";

		if ((s = getenv("MALLOC_TMPDIR")) == NULL && (s =
		    getenv("TMPDIR")) == NULL)
			s = P_tmpdir;
		slen = strlen(s);
		if (slen + sizeof(suffix) > sizeof(pagefile_templ)) {
			_malloc_message(_getprogname(),
			    ": (malloc) Page file path too long\n",
			    "", "");
			abort();
		}
		memcpy(pagefile_templ, s, slen);
		memcpy(&pagefile_templ[slen], suffix, sizeof(suffix));
	}
#endif

	for (i = 0; i < 3; i++) {
		unsigned j;

		/* Get runtime configuration. */
		switch (i) {
		case 0:
#ifndef MOZ_MEMORY_WINDOWS
			if ((linklen = readlink("/etc/malloc.conf", buf,
						sizeof(buf) - 1)) != -1) {
				/*
				 * Use the contents of the "/etc/malloc.conf"
				 * symbolic link's name.
				 */
				buf[linklen] = '\0';
				opts = buf;
			} else
#endif
			{
				/* No configuration specified. */
				buf[0] = '\0';
				opts = buf;
			}
			break;
		case 1:
			if (issetugid() == 0 && (opts =
			    getenv("MALLOC_OPTIONS")) != NULL) {
				/*
				 * Do nothing; opts is already initialized to
				 * the value of the MALLOC_OPTIONS environment
				 * variable.
				 */
			} else {
				/* No configuration specified. */
				buf[0] = '\0';
				opts = buf;
			}
			break;
		case 2:
			if (_malloc_options != NULL) {
				/*
				 * Use options that were compiled into the
				 * program.
				 */
				opts = _malloc_options;
			} else {
				/* No configuration specified. */
				buf[0] = '\0';
				opts = buf;
			}
			break;
		default:
			/* NOTREACHED */
			buf[0] = '\0';
			opts = buf;
			assert(false);
		}

		for (j = 0; opts[j] != '\0'; j++) {
			unsigned k, nreps;
			bool nseen;

			/* Parse repetition count, if any. */
			for (nreps = 0, nseen = false;; j++, nseen = true) {
				switch (opts[j]) {
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
					case '8': case '9':
						nreps *= 10;
						nreps += opts[j] - '0';
						break;
					default:
						goto MALLOC_OUT;
				}
			}
MALLOC_OUT:
			if (nseen == false)
				nreps = 1;

			for (k = 0; k < nreps; k++) {
				switch (opts[j]) {
				case 'a':
					opt_abort = false;
					break;
				case 'A':
					opt_abort = true;
					break;
				case 'b':
#ifdef MALLOC_BALANCE
					opt_balance_threshold >>= 1;
#endif
					break;
				case 'B':
#ifdef MALLOC_BALANCE
					if (opt_balance_threshold == 0)
						opt_balance_threshold = 1;
					else if ((opt_balance_threshold << 1)
					    > opt_balance_threshold)
						opt_balance_threshold <<= 1;
#endif
					break;
				case 'f':
					opt_dirty_max >>= 1;
					break;
				case 'F':
					if (opt_dirty_max == 0)
						opt_dirty_max = 1;
					else if ((opt_dirty_max << 1) != 0)
						opt_dirty_max <<= 1;
					break;
				case 'g':
					opt_reserve_range_lshift--;
					break;
				case 'G':
					opt_reserve_range_lshift++;
					break;
#ifdef MALLOC_FILL
				case 'j':
					opt_junk = false;
					break;
				case 'J':
					opt_junk = true;
					break;
#endif
				case 'k':
					/*
					 * Chunks always require at least one
					 * header page, so chunks can never be
					 * smaller than two pages.
					 */
					if (opt_chunk_2pow > pagesize_2pow + 1)
						opt_chunk_2pow--;
					break;
				case 'K':
					if (opt_chunk_2pow + 1 <
					    (sizeof(size_t) << 3))
						opt_chunk_2pow++;
					break;
				case 'n':
					opt_narenas_lshift--;
					break;
				case 'N':
					opt_narenas_lshift++;
					break;
#ifdef MALLOC_PAGEFILE
				case 'o':
					/* Do not over-commit. */
					opt_pagefile = true;
					break;
				case 'O':
					/* Allow over-commit. */
					opt_pagefile = false;
					break;
#endif
				case 'p':
					opt_print_stats = false;
					break;
				case 'P':
					opt_print_stats = true;
					break;
				case 'q':
					if (opt_quantum_2pow > QUANTUM_2POW_MIN)
						opt_quantum_2pow--;
					break;
				case 'Q':
					if (opt_quantum_2pow < pagesize_2pow -
					    1)
						opt_quantum_2pow++;
					break;
				case 'r':
					opt_reserve_min_lshift--;
					break;
				case 'R':
					opt_reserve_min_lshift++;
					break;
				case 's':
					if (opt_small_max_2pow >
					    QUANTUM_2POW_MIN)
						opt_small_max_2pow--;
					break;
				case 'S':
					if (opt_small_max_2pow < pagesize_2pow
					    - 1)
						opt_small_max_2pow++;
					break;
#ifdef MALLOC_UTRACE
				case 'u':
					opt_utrace = false;
					break;
				case 'U':
					opt_utrace = true;
					break;
#endif
#ifdef MALLOC_SYSV
				case 'v':
					opt_sysv = false;
					break;
				case 'V':
					opt_sysv = true;
					break;
#endif
#ifdef MALLOC_XMALLOC
				case 'x':
					opt_xmalloc = false;
					break;
				case 'X':
					opt_xmalloc = true;
					break;
#endif
#ifdef MALLOC_FILL
				case 'z':
					opt_zero = false;
					break;
				case 'Z':
					opt_zero = true;
					break;
#endif
				default: {
					char cbuf[2];

					cbuf[0] = opts[j];
					cbuf[1] = '\0';
					_malloc_message(_getprogname(),
					    ": (malloc) Unsupported character "
					    "in malloc options: '", cbuf,
					    "'\n");
				}
				}
			}
		}
	}

	/* Take care to call atexit() only once. */
	if (opt_print_stats) {
#ifndef MOZ_MEMORY_WINDOWS
		/* Print statistics at exit. */
		atexit(malloc_print_stats);
#endif
	}

#if (!defined(MOZ_MEMORY_WINDOWS) && !defined(MOZ_MEMORY_DARWIN))
	/* Prevent potential deadlock on malloc locks after fork. */
	pthread_atfork(_malloc_prefork, _malloc_postfork, _malloc_postfork);
#endif

	/* Set variables according to the value of opt_small_max_2pow. */
	if (opt_small_max_2pow < opt_quantum_2pow)
		opt_small_max_2pow = opt_quantum_2pow;
	small_max = (1U << opt_small_max_2pow);

	/* Set bin-related variables. */
	bin_maxclass = (pagesize >> 1);
	assert(opt_quantum_2pow >= TINY_MIN_2POW);
	ntbins = opt_quantum_2pow - TINY_MIN_2POW;
	assert(ntbins <= opt_quantum_2pow);
	nqbins = (small_max >> opt_quantum_2pow);
	nsbins = pagesize_2pow - opt_small_max_2pow - 1;

	/* Set variables according to the value of opt_quantum_2pow. */
	quantum = (1U << opt_quantum_2pow);
	quantum_mask = quantum - 1;
	if (ntbins > 0)
		small_min = (quantum >> 1) + 1;
	else
		small_min = 1;
	assert(small_min <= quantum);

	/* Set variables according to the value of opt_chunk_2pow. */
	chunksize = (1LU << opt_chunk_2pow);
	chunksize_mask = chunksize - 1;
	chunk_npages = (chunksize >> pagesize_2pow);
	{
		size_t header_size;

		/*
		 * Compute the header size such that it is large
		 * enough to contain the page map and enough nodes for the
		 * worst case: one node per non-header page plus one extra for
		 * situations where we briefly have one more node allocated
		 * than we will need.
		 */
		header_size = sizeof(arena_chunk_t) +
		    (sizeof(arena_chunk_map_t) * (chunk_npages - 1));
		arena_chunk_header_npages = (header_size >> pagesize_2pow) +
		    ((header_size & pagesize_mask) != 0);
	}
	arena_maxclass = chunksize - (arena_chunk_header_npages <<
	    pagesize_2pow);

#ifdef JEMALLOC_USES_MAP_ALIGN
	/*
	 * When using MAP_ALIGN, the alignment parameter must be a power of two
	 * multiple of the system pagesize, or mmap will fail.
	 */
	assert((chunksize % pagesize) == 0);
	assert((1 << (ffs(chunksize / pagesize) - 1)) == (chunksize/pagesize));
#endif

	UTRACE(0, 0, 0);

#ifdef MALLOC_STATS
	memset(&stats_chunks, 0, sizeof(chunk_stats_t));
#endif

	/* Various sanity checks that regard configuration. */
	assert(quantum >= sizeof(void *));
	assert(quantum <= pagesize);
	assert(chunksize >= pagesize);
	assert(quantum * 4 <= chunksize);

	/* Initialize chunks data. */
	malloc_mutex_init(&huge_mtx);
	extent_tree_ad_new(&huge);
#ifdef MALLOC_STATS
	huge_nmalloc = 0;
	huge_ndalloc = 0;
	huge_allocated = 0;
#endif

	/* Initialize base allocation data structures. */
#ifdef MALLOC_STATS
	base_mapped = 0;
#endif
	base_nodes = NULL;
	base_reserve_regs = NULL;
	malloc_mutex_init(&base_mtx);

#ifdef MOZ_MEMORY_NARENAS_DEFAULT_ONE
	narenas = 1;
#else
	if (ncpus > 1) {
		/*
		 * For SMP systems, create four times as many arenas as there
		 * are CPUs by default.
		 */
		opt_narenas_lshift += 2;
	}

	/* Determine how many arenas to use. */
	narenas = ncpus;
#endif
	if (opt_narenas_lshift > 0) {
		if ((narenas << opt_narenas_lshift) > narenas)
			narenas <<= opt_narenas_lshift;
		/*
		 * Make sure not to exceed the limits of what base_alloc() can
		 * handle.
		 */
		if (narenas * sizeof(arena_t *) > chunksize)
			narenas = chunksize / sizeof(arena_t *);
	} else if (opt_narenas_lshift < 0) {
		if ((narenas >> -opt_narenas_lshift) < narenas)
			narenas >>= -opt_narenas_lshift;
		/* Make sure there is at least one arena. */
		if (narenas == 0)
			narenas = 1;
	}
#ifdef MALLOC_BALANCE
	assert(narenas != 0);
	for (narenas_2pow = 0;
	     (narenas >> (narenas_2pow + 1)) != 0;
	     narenas_2pow++);
#endif

#ifdef NO_TLS
	if (narenas > 1) {
		static const unsigned primes[] = {1, 3, 5, 7, 11, 13, 17, 19,
		    23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83,
		    89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149,
		    151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211,
		    223, 227, 229, 233, 239, 241, 251, 257, 263};
		unsigned nprimes, parenas;

		/*
		 * Pick a prime number of hash arenas that is more than narenas
		 * so that direct hashing of pthread_self() pointers tends to
		 * spread allocations evenly among the arenas.
		 */
		assert((narenas & 1) == 0); /* narenas must be even. */
		nprimes = (sizeof(primes) >> SIZEOF_INT_2POW);
		parenas = primes[nprimes - 1]; /* In case not enough primes. */
		for (i = 1; i < nprimes; i++) {
			if (primes[i] > narenas) {
				parenas = primes[i];
				break;
			}
		}
		narenas = parenas;
	}
#endif

#ifndef NO_TLS
#  ifndef MALLOC_BALANCE
	next_arena = 0;
#  endif
#endif

	/* Allocate and initialize arenas. */
	arenas = (arena_t **)base_alloc(sizeof(arena_t *) * narenas);
	if (arenas == NULL) {
#ifndef MOZ_MEMORY_WINDOWS
		malloc_mutex_unlock(&init_lock);
#endif
		return (true);
	}
	/*
	 * Zero the array.  In practice, this should always be pre-zeroed,
	 * since it was just mmap()ed, but let's be sure.
	 */
	memset(arenas, 0, sizeof(arena_t *) * narenas);

	/*
	 * Initialize one arena here.  The rest are lazily created in
	 * choose_arena_hard().
	 */
	arenas_extend(0);
	if (arenas[0] == NULL) {
#ifndef MOZ_MEMORY_WINDOWS
		malloc_mutex_unlock(&init_lock);
#endif
		return (true);
	}
#ifndef NO_TLS
	/*
	 * Assign the initial arena to the initial thread, in order to avoid
	 * spurious creation of an extra arena if the application switches to
	 * threaded mode.
	 */
#ifdef MOZ_MEMORY_WINDOWS
	TlsSetValue(tlsIndex, arenas[0]);
#else
	arenas_map = arenas[0];
#endif
#endif

	/*
	 * Seed here for the initial thread, since choose_arena_hard() is only
	 * called for other threads.  The seed value doesn't really matter.
	 */
#ifdef MALLOC_BALANCE
	SPRN(balance, 42);
#endif

	malloc_spin_init(&arenas_lock);

#ifdef MALLOC_VALIDATE
	chunk_rtree = malloc_rtree_new((SIZEOF_PTR << 3) - opt_chunk_2pow);
	if (chunk_rtree == NULL)
		return (true);
#endif

	/*
	 * Configure and initialize the memory reserve.  This needs to happen
	 * late during initialization, since chunks are allocated.
	 */
	malloc_mutex_init(&reserve_mtx);
	reserve_min = 0;
	reserve_cur = 0;
	reserve_max = 0;
	if (RESERVE_RANGE_2POW_DEFAULT + opt_reserve_range_lshift >= 0) {
		reserve_max += chunksize << (RESERVE_RANGE_2POW_DEFAULT +
		    opt_reserve_range_lshift);
	}
	ql_new(&reserve_regs);
	reserve_seq = 0;
	extent_tree_szad_new(&reserve_chunks_szad);
	extent_tree_ad_new(&reserve_chunks_ad);
	if (RESERVE_MIN_2POW_DEFAULT + opt_reserve_min_lshift >= 0) {
		reserve_min_set(chunksize << (RESERVE_MIN_2POW_DEFAULT +
		    opt_reserve_min_lshift));
	}

	malloc_initialized = true;
#ifndef MOZ_MEMORY_WINDOWS
	malloc_mutex_unlock(&init_lock);
#endif
	return (false);
}

/* XXX Why not just expose malloc_print_stats()? */
#ifdef MOZ_MEMORY_WINDOWS
void
malloc_shutdown()
{

	malloc_print_stats();
}
#endif

/*
 * End general internal functions.
 */
/******************************************************************************/
/*
 * Begin malloc(3)-compatible functions.
 */

/*
 * Inline the standard malloc functions if they are being subsumed by Darwin's
 * zone infrastructure.
 */
#ifdef MOZ_MEMORY_DARWIN
#  define ZONE_INLINE	inline
#else
#  define ZONE_INLINE
#endif

/* Mangle standard interfaces on Darwin and Windows CE, 
   in order to avoid linking problems. */
#ifdef MOZ_MEMORY_DARWIN
#define DONT_OVERRIDE_LIBC
#endif

#if defined(DONT_OVERRIDE_LIBC)
#define	malloc(a)	je_malloc(a)
#define	valloc(a)	je_valloc(a)
#define	calloc(a, b)	je_calloc(a, b)
#define	realloc(a, b)	je_realloc(a, b)
#define	free(a)		je_free(a)
#define _msize(p) je_msize(p)
#define _recalloc(p, n, s) je_recalloc(p, n, s)
#endif

ZONE_INLINE
void *
malloc(size_t size)
{
	void *ret;

	if (malloc_init()) {
		ret = NULL;
		goto RETURN;
	}

	if (size == 0) {
#ifdef MALLOC_SYSV
		if (opt_sysv == false)
#endif
			size = 1;
#ifdef MALLOC_SYSV
		else {
			ret = NULL;
			goto RETURN;
		}
#endif
	}

	ret = imalloc(size);

RETURN:
	if (ret == NULL) {
#ifdef MALLOC_XMALLOC
		if (opt_xmalloc) {
			_malloc_message(_getprogname(),
			    ": (malloc) Error in malloc(): out of memory\n", "",
			    "");
			abort();
		}
#endif
		errno = ENOMEM;
	}

	UTRACE(0, size, ret);
	return (ret);
}

#ifdef MOZ_MEMORY_SOLARIS
#  ifdef __SUNPRO_C
void *
memalign(size_t alignment, size_t size);
#pragma no_inline(memalign)
#  elif (defined(__GNU_C__))
__attribute__((noinline))
#  endif
#else
inline
#endif
void *
memalign(size_t alignment, size_t size)
{
	void *ret;

	assert(((alignment - 1) & alignment) == 0 && alignment >=
	    sizeof(void *));

	if (malloc_init()) {
		ret = NULL;
		goto RETURN;
	}

	ret = ipalloc(alignment, size);

RETURN:
#ifdef MALLOC_XMALLOC
	if (opt_xmalloc && ret == NULL) {
		_malloc_message(_getprogname(),
		": (malloc) Error in memalign(): out of memory\n", "", "");
		abort();
	}
#endif
	UTRACE(0, size, ret);
	return (ret);
}

ZONE_INLINE
int
posix_memalign(void **memptr, size_t alignment, size_t size)
{
	void *result;

	/* Make sure that alignment is a large enough power of 2. */
	if (((alignment - 1) & alignment) != 0 || alignment < sizeof(void *)) {
#ifdef MALLOC_XMALLOC
		if (opt_xmalloc) {
			_malloc_message(_getprogname(),
			    ": (malloc) Error in posix_memalign(): "
			    "invalid alignment\n", "", "");
			abort();
		}
#endif
		return (EINVAL);
	}

#ifdef MOZ_MEMORY_DARWIN
	result = moz_memalign(alignment, size);
#else
	result = memalign(alignment, size);
#endif
	if (result == NULL)
		return (ENOMEM);

	*memptr = result;
	return (0);
}

ZONE_INLINE
void *
valloc(size_t size)
{
#ifdef MOZ_MEMORY_DARWIN
	return (moz_memalign(pagesize, size));
#else
	return (memalign(pagesize, size));
#endif
}

ZONE_INLINE
void *
calloc(size_t num, size_t size)
{
	void *ret;
	size_t num_size;

	if (malloc_init()) {
		num_size = 0;
		ret = NULL;
		goto RETURN;
	}

	num_size = num * size;
	if (num_size == 0) {
#ifdef MALLOC_SYSV
		if ((opt_sysv == false) && ((num == 0) || (size == 0)))
#endif
			num_size = 1;
#ifdef MALLOC_SYSV
		else {
			ret = NULL;
			goto RETURN;
		}
#endif
	/*
	 * Try to avoid division here.  We know that it isn't possible to
	 * overflow during multiplication if neither operand uses any of the
	 * most significant half of the bits in a size_t.
	 */
	} else if (((num | size) & (SIZE_T_MAX << (sizeof(size_t) << 2)))
	    && (num_size / size != num)) {
		/* size_t overflow. */
		ret = NULL;
		goto RETURN;
	}

	ret = icalloc(num_size);

RETURN:
	if (ret == NULL) {
#ifdef MALLOC_XMALLOC
		if (opt_xmalloc) {
			_malloc_message(_getprogname(),
			    ": (malloc) Error in calloc(): out of memory\n", "",
			    "");
			abort();
		}
#endif
		errno = ENOMEM;
	}

	UTRACE(0, num_size, ret);
	return (ret);
}

ZONE_INLINE
void *
realloc(void *ptr, size_t size)
{
	void *ret;

	if (size == 0) {
#ifdef MALLOC_SYSV
		if (opt_sysv == false)
#endif
			size = 1;
#ifdef MALLOC_SYSV
		else {
			if (ptr != NULL)
				idalloc(ptr);
			ret = NULL;
			goto RETURN;
		}
#endif
	}

	if (ptr != NULL) {
		assert(malloc_initialized);

		ret = iralloc(ptr, size);

		if (ret == NULL) {
#ifdef MALLOC_XMALLOC
			if (opt_xmalloc) {
				_malloc_message(_getprogname(),
				    ": (malloc) Error in realloc(): out of "
				    "memory\n", "", "");
				abort();
			}
#endif
			errno = ENOMEM;
		}
	} else {
		if (malloc_init())
			ret = NULL;
		else
			ret = imalloc(size);

		if (ret == NULL) {
#ifdef MALLOC_XMALLOC
			if (opt_xmalloc) {
				_malloc_message(_getprogname(),
				    ": (malloc) Error in realloc(): out of "
				    "memory\n", "", "");
				abort();
			}
#endif
			errno = ENOMEM;
		}
	}

#ifdef MALLOC_SYSV
RETURN:
#endif
	UTRACE(ptr, size, ret);
	return (ret);
}

ZONE_INLINE
void
free(void *ptr)
{

	UTRACE(ptr, 0, 0);
	if (ptr != NULL) {
		assert(malloc_initialized);

		idalloc(ptr);
	}
}

/*
 * End malloc(3)-compatible functions.
 */
/******************************************************************************/
/*
 * Begin non-standard functions.
 */

size_t
malloc_usable_size(const void *ptr)
{

#ifdef MALLOC_VALIDATE
	return (isalloc_validate(ptr));
#else
	assert(ptr != NULL);

	return (isalloc(ptr));
#endif
}

void
jemalloc_stats(jemalloc_stats_t *stats)
{
	size_t i;

	assert(stats != NULL);

	/*
	 * Gather runtime settings.
	 */
	stats->opt_abort = opt_abort;
	stats->opt_junk =
#ifdef MALLOC_FILL
	    opt_junk ? true :
#endif
	    false;
	stats->opt_utrace =
#ifdef MALLOC_UTRACE
	    opt_utrace ? true :
#endif
	    false;
	stats->opt_sysv =
#ifdef MALLOC_SYSV
	    opt_sysv ? true :
#endif
	    false;
	stats->opt_xmalloc =
#ifdef MALLOC_XMALLOC
	    opt_xmalloc ? true :
#endif
	    false;
	stats->opt_zero =
#ifdef MALLOC_FILL
	    opt_zero ? true :
#endif
	    false;
	stats->narenas = narenas;
	stats->balance_threshold =
#ifdef MALLOC_BALANCE
	    opt_balance_threshold
#else
	    SIZE_T_MAX
#endif
	    ;
	stats->quantum = quantum;
	stats->small_max = small_max;
	stats->large_max = arena_maxclass;
	stats->chunksize = chunksize;
	stats->dirty_max = opt_dirty_max;

	malloc_mutex_lock(&reserve_mtx);
	stats->reserve_min = reserve_min;
	stats->reserve_max = reserve_max;
	stats->reserve_cur = reserve_cur;
	malloc_mutex_unlock(&reserve_mtx);

	/*
	 * Gather current memory usage statistics.
	 */
	stats->mapped = 0;
	stats->committed = 0;
	stats->allocated = 0;
	stats->dirty = 0;

	/* Get huge mapped/allocated. */
	malloc_mutex_lock(&huge_mtx);
	stats->mapped += stats_chunks.curchunks * chunksize;
#ifdef MALLOC_DECOMMIT
	stats->committed += huge_allocated;
#endif
	stats->allocated += huge_allocated;
	malloc_mutex_unlock(&huge_mtx);

	/* Get base mapped. */
	malloc_mutex_lock(&base_mtx);
	stats->mapped += base_mapped;
#ifdef MALLOC_DECOMMIT
	stats->committed += base_mapped;
#endif
	malloc_mutex_unlock(&base_mtx);

	/* Iterate over arenas and their chunks. */
	for (i = 0; i < narenas; i++) {
		arena_t *arena = arenas[i];
		if (arena != NULL) {
			arena_chunk_t *chunk;

			malloc_spin_lock(&arena->lock);
			stats->allocated += arena->stats.allocated_small;
			stats->allocated += arena->stats.allocated_large;
#ifdef MALLOC_DECOMMIT
			rb_foreach_begin(arena_chunk_t, link_dirty,
			    &arena->chunks_dirty, chunk) {
				size_t j;

				for (j = 0; j < chunk_npages; j++) {
					if ((chunk->map[j].bits &
					    CHUNK_MAP_DECOMMITTED) == 0)
						stats->committed += pagesize;
				}
			} rb_foreach_end(arena_chunk_t, link_dirty,
			    &arena->chunks_dirty, chunk)
#endif
			stats->dirty += (arena->ndirty << pagesize_2pow);
			malloc_spin_unlock(&arena->lock);
		}
	}

#ifndef MALLOC_DECOMMIT
	stats->committed = stats->mapped;
#endif
}

void *
xmalloc(size_t size)
{
	void *ret;

	if (malloc_init())
		reserve_fail(size, "xmalloc");

	if (size == 0) {
#ifdef MALLOC_SYSV
		if (opt_sysv == false)
#endif
			size = 1;
#ifdef MALLOC_SYSV
		else {
			_malloc_message(_getprogname(),
			    ": (malloc) Error in xmalloc(): ",
			    "invalid size 0", "\n");
			abort();
		}
#endif
	}

	ret = imalloc(size);
	if (ret == NULL) {
		uint64_t seq = 0;

		do {
			seq = reserve_crit(size, "xmalloc", seq);
			ret = imalloc(size);
		} while (ret == NULL);
	}

	UTRACE(0, size, ret);
	return (ret);
}

void *
xcalloc(size_t num, size_t size)
{
	void *ret;
	size_t num_size;

	num_size = num * size;
	if (malloc_init())
		reserve_fail(num_size, "xcalloc");

	if (num_size == 0) {
#ifdef MALLOC_SYSV
		if ((opt_sysv == false) && ((num == 0) || (size == 0)))
#endif
			num_size = 1;
#ifdef MALLOC_SYSV
		else {
			_malloc_message(_getprogname(),
			    ": (malloc) Error in xcalloc(): ",
			    "invalid size 0", "\n");
			abort();
		}
#endif
	/*
	 * Try to avoid division here.  We know that it isn't possible to
	 * overflow during multiplication if neither operand uses any of the
	 * most significant half of the bits in a size_t.
	 */
	} else if (((num | size) & (SIZE_T_MAX << (sizeof(size_t) << 2)))
	    && (num_size / size != num)) {
		/* size_t overflow. */
		_malloc_message(_getprogname(),
		    ": (malloc) Error in xcalloc(): ",
		    "size overflow", "\n");
		abort();
	}

	ret = icalloc(num_size);
	if (ret == NULL) {
		uint64_t seq = 0;

		do {
			seq = reserve_crit(num_size, "xcalloc", seq);
			ret = icalloc(num_size);
		} while (ret == NULL);
	}

	UTRACE(0, num_size, ret);
	return (ret);
}

void *
xrealloc(void *ptr, size_t size)
{
	void *ret;

	if (size == 0) {
#ifdef MALLOC_SYSV
		if (opt_sysv == false)
#endif
			size = 1;
#ifdef MALLOC_SYSV
		else {
			if (ptr != NULL)
				idalloc(ptr);
			_malloc_message(_getprogname(),
			    ": (malloc) Error in xrealloc(): ",
			    "invalid size 0", "\n");
			abort();
		}
#endif
	}

	if (ptr != NULL) {
		assert(malloc_initialized);

		ret = iralloc(ptr, size);
		if (ret == NULL) {
			uint64_t seq = 0;

			do {
				seq = reserve_crit(size, "xrealloc", seq);
				ret = iralloc(ptr, size);
			} while (ret == NULL);
		}
	} else {
		if (malloc_init())
			reserve_fail(size, "xrealloc");

		ret = imalloc(size);
		if (ret == NULL) {
			uint64_t seq = 0;

			do {
				seq = reserve_crit(size, "xrealloc", seq);
				ret = imalloc(size);
			} while (ret == NULL);
		}
	}

	UTRACE(ptr, size, ret);
	return (ret);
}

void *
xmemalign(size_t alignment, size_t size)
{
	void *ret;

	assert(((alignment - 1) & alignment) == 0 && alignment >=
	    sizeof(void *));

	if (malloc_init())
		reserve_fail(size, "xmemalign");

	ret = ipalloc(alignment, size);
	if (ret == NULL) {
		uint64_t seq = 0;

		do {
			seq = reserve_crit(size, "xmemalign", seq);
			ret = ipalloc(alignment, size);
		} while (ret == NULL);
	}

	UTRACE(0, size, ret);
	return (ret);
}

static void
reserve_shrink(void)
{
	extent_node_t *node;

	assert(reserve_cur > reserve_max);
#ifdef MALLOC_DEBUG
	{
		extent_node_t *node;
		size_t reserve_size;

		reserve_size = 0;
		rb_foreach_begin(extent_node_t, link_szad, &reserve_chunks_szad,
		    node) {
			reserve_size += node->size;
		} rb_foreach_end(extent_node_t, link_szad, &reserve_chunks_szad,
		    node)
		assert(reserve_size == reserve_cur);

		reserve_size = 0;
		rb_foreach_begin(extent_node_t, link_ad, &reserve_chunks_ad,
		    node) {
			reserve_size += node->size;
		} rb_foreach_end(extent_node_t, link_ad, &reserve_chunks_ad,
		    node)
		assert(reserve_size == reserve_cur);
	}
#endif

	/* Discard chunks until the the reserve is below the size limit. */
	rb_foreach_reverse_begin(extent_node_t, link_ad, &reserve_chunks_ad,
	    node) {
#ifndef MALLOC_DECOMMIT
		if (node->size <= reserve_cur - reserve_max) {
#endif
			extent_node_t *tnode = extent_tree_ad_prev(
			    &reserve_chunks_ad, node);

#ifdef MALLOC_DECOMMIT
			assert(node->size <= reserve_cur - reserve_max);
#endif

			/* Discard the entire [multi-]chunk. */
			extent_tree_szad_remove(&reserve_chunks_szad, node);
			extent_tree_ad_remove(&reserve_chunks_ad, node);
			reserve_cur -= node->size;
			pages_unmap(node->addr, node->size);
#ifdef MALLOC_STATS
			stats_chunks.curchunks -= (node->size / chunksize);
#endif
			base_node_dealloc(node);
			if (reserve_cur == reserve_max)
				break;

			rb_foreach_reverse_prev(extent_node_t, link_ad,
			    extent_ad_comp, &reserve_chunks_ad, tnode);
#ifndef MALLOC_DECOMMIT
		} else {
			/* Discard the end of the multi-chunk. */
			extent_tree_szad_remove(&reserve_chunks_szad, node);
			node->size -= reserve_cur - reserve_max;
			extent_tree_szad_insert(&reserve_chunks_szad, node);
			pages_unmap((void *)((uintptr_t)node->addr +
			    node->size), reserve_cur - reserve_max);
#ifdef MALLOC_STATS
			stats_chunks.curchunks -= ((reserve_cur - reserve_max) /
			    chunksize);
#endif
			reserve_cur = reserve_max;
			break;
		}
#endif
		assert(reserve_cur > reserve_max);
	} rb_foreach_reverse_end(extent_node_t, link_ad, &reserve_chunks_ad,
	    node)
}

/* Send a condition notification. */
static uint64_t
reserve_notify(reserve_cnd_t cnd, size_t size, uint64_t seq)
{
	reserve_reg_t *reg;

	/* seq is used to keep track of distinct condition-causing events. */
	if (seq == 0) {
		/* Allocate new sequence number. */
		reserve_seq++;
		seq = reserve_seq;
	}

	/*
	 * Advance to the next callback registration and send a notification,
	 * unless one has already been sent for this condition-causing event.
	 */
	reg = ql_first(&reserve_regs);
	if (reg == NULL)
		return (0);
	ql_first(&reserve_regs) = ql_next(&reserve_regs, reg, link);
	if (reg->seq == seq)
		return (0);
	reg->seq = seq;
	malloc_mutex_unlock(&reserve_mtx);
	reg->cb(reg->ctx, cnd, size);
	malloc_mutex_lock(&reserve_mtx);

	return (seq);
}

/* Allocation failure due to OOM.  Try to free some memory via callbacks. */
static uint64_t
reserve_crit(size_t size, const char *fname, uint64_t seq)
{

	/*
	 * Send one condition notification.  Iteration is handled by the
	 * caller of this function.
	 */
	malloc_mutex_lock(&reserve_mtx);
	seq = reserve_notify(RESERVE_CND_CRIT, size, seq);
	malloc_mutex_unlock(&reserve_mtx);

	/* If no notification could be sent, then no further recourse exists. */
	if (seq == 0)
		reserve_fail(size, fname);

	return (seq);
}

/* Permanent allocation failure due to OOM. */
static void
reserve_fail(size_t size, const char *fname)
{
	uint64_t seq = 0;

	/* Send fail notifications. */
	malloc_mutex_lock(&reserve_mtx);
	do {
		seq = reserve_notify(RESERVE_CND_FAIL, size, seq);
	} while (seq != 0);
	malloc_mutex_unlock(&reserve_mtx);

	/* Terminate the application. */
	_malloc_message(_getprogname(),
	    ": (malloc) Error in ", fname, "(): out of memory\n");
	abort();
}

bool
reserve_cb_register(reserve_cb_t *cb, void *ctx)
{
	reserve_reg_t *reg = base_reserve_reg_alloc();
	if (reg == NULL)
		return (true);

	ql_elm_new(reg, link);
	reg->cb = cb;
	reg->ctx = ctx;
	reg->seq = 0;

	malloc_mutex_lock(&reserve_mtx);
	ql_head_insert(&reserve_regs, reg, link);
	malloc_mutex_unlock(&reserve_mtx);

	return (false);
}

bool
reserve_cb_unregister(reserve_cb_t *cb, void *ctx)
{
	reserve_reg_t *reg = NULL;

	malloc_mutex_lock(&reserve_mtx);
	ql_foreach(reg, &reserve_regs, link) {
		if (reg->cb == cb && reg->ctx == ctx) {
			ql_remove(&reserve_regs, reg, link);
			break;
		}
	}
	malloc_mutex_unlock(&reserve_mtx);

	if (reg != NULL)
		base_reserve_reg_dealloc(reg);
		return (false);
	return (true);
}

size_t
reserve_cur_get(void)
{
	size_t ret;

	malloc_mutex_lock(&reserve_mtx);
	ret = reserve_cur;
	malloc_mutex_unlock(&reserve_mtx);

	return (ret);
}

size_t
reserve_min_get(void)
{
	size_t ret;

	malloc_mutex_lock(&reserve_mtx);
	ret = reserve_min;
	malloc_mutex_unlock(&reserve_mtx);

	return (ret);
}

bool
reserve_min_set(size_t min)
{

	min = CHUNK_CEILING(min);

	malloc_mutex_lock(&reserve_mtx);
	/* Keep |reserve_max - reserve_min| the same. */
	if (min < reserve_min) {
		reserve_max -= reserve_min - min;
		reserve_min = min;
	} else {
		/* Protect against wrap-around. */
		if (reserve_max + min - reserve_min < reserve_max) {
			reserve_min = SIZE_T_MAX - (reserve_max - reserve_min)
			    - chunksize + 1;
			reserve_max = SIZE_T_MAX - chunksize + 1;
		} else {
			reserve_max += min - reserve_min;
			reserve_min = min;
		}
	}

	/* Resize the reserve if necessary. */
	if (reserve_cur < reserve_min) {
		size_t size = reserve_min - reserve_cur;

		/* Force the reserve to grow by allocating/deallocating. */
		malloc_mutex_unlock(&reserve_mtx);
#ifdef MALLOC_DECOMMIT
		{
			void **chunks;
			size_t i, n;

			n = size >> opt_chunk_2pow;
			chunks = (void**)imalloc(n * sizeof(void *));
			if (chunks == NULL)
				return (true);
			for (i = 0; i < n; i++) {
				chunks[i] = huge_malloc(chunksize, false);
				if (chunks[i] == NULL) {
					size_t j;

					for (j = 0; j < i; j++) {
						huge_dalloc(chunks[j]);
					}
					idalloc(chunks);
					return (true);
				}
			}
			for (i = 0; i < n; i++)
				huge_dalloc(chunks[i]);
			idalloc(chunks);
		}
#else
		{
			void *x = huge_malloc(size, false);
			if (x == NULL) {
				return (true);
			}
			huge_dalloc(x);
		}
#endif
	} else if (reserve_cur > reserve_max) {
		reserve_shrink();
		malloc_mutex_unlock(&reserve_mtx);
	} else
		malloc_mutex_unlock(&reserve_mtx);

	return (false);
}

#ifdef MOZ_MEMORY_WINDOWS
void*
_recalloc(void *ptr, size_t count, size_t size)
{
	size_t oldsize = (ptr != NULL) ? isalloc(ptr) : 0;
	size_t newsize = count * size;

	/*
	 * In order for all trailing bytes to be zeroed, the caller needs to
	 * use calloc(), followed by recalloc().  However, the current calloc()
	 * implementation only zeros the bytes requested, so if recalloc() is
	 * to work 100% correctly, calloc() will need to change to zero
	 * trailing bytes.
	 */

	ptr = realloc(ptr, newsize);
	if (ptr != NULL && oldsize < newsize) {
		memset((void *)((uintptr_t)ptr + oldsize), 0, newsize -
		    oldsize);
	}

	return ptr;
}

/*
 * This impl of _expand doesn't ever actually expand or shrink blocks: it
 * simply replies that you may continue using a shrunk block.
 */
void*
_expand(void *ptr, size_t newsize)
{
	if (isalloc(ptr) >= newsize)
		return ptr;

	return NULL;
}

size_t
_msize(const void *ptr)
{
	return malloc_usable_size(ptr);
}
#endif

/*
 * End non-standard functions.
 */
/******************************************************************************/
/*
 * Begin library-private functions, used by threading libraries for protection
 * of malloc during fork().  These functions are only called if the program is
 * running in threaded mode, so there is no need to check whether the program
 * is threaded here.
 */

void
_malloc_prefork(void)
{
	unsigned i;

	/* Acquire all mutexes in a safe order. */

	malloc_spin_lock(&arenas_lock);
	for (i = 0; i < narenas; i++) {
		if (arenas[i] != NULL)
			malloc_spin_lock(&arenas[i]->lock);
	}
	malloc_spin_unlock(&arenas_lock);

	malloc_mutex_lock(&base_mtx);

	malloc_mutex_lock(&huge_mtx);
}

void
_malloc_postfork(void)
{
	unsigned i;

	/* Release all mutexes, now that fork() has completed. */

	malloc_mutex_unlock(&huge_mtx);

	malloc_mutex_unlock(&base_mtx);

	malloc_spin_lock(&arenas_lock);
	for (i = 0; i < narenas; i++) {
		if (arenas[i] != NULL)
			malloc_spin_unlock(&arenas[i]->lock);
	}
	malloc_spin_unlock(&arenas_lock);
}

/*
 * End library-private functions.
 */
/******************************************************************************/

#ifdef HAVE_LIBDL
#  include <dlfcn.h>
#endif

#ifdef MOZ_MEMORY_DARWIN
static malloc_zone_t zone;
static struct malloc_introspection_t zone_introspect;

static size_t
zone_size(malloc_zone_t *zone, void *ptr)
{

	/*
	 * There appear to be places within Darwin (such as setenv(3)) that
	 * cause calls to this function with pointers that *no* zone owns.  If
	 * we knew that all pointers were owned by *some* zone, we could split
	 * our zone into two parts, and use one as the default allocator and
	 * the other as the default deallocator/reallocator.  Since that will
	 * not work in practice, we must check all pointers to assure that they
	 * reside within a mapped chunk before determining size.
	 */
	return (isalloc_validate(ptr));
}

static void *
zone_malloc(malloc_zone_t *zone, size_t size)
{

	return (malloc(size));
}

static void *
zone_calloc(malloc_zone_t *zone, size_t num, size_t size)
{

	return (calloc(num, size));
}

static void *
zone_valloc(malloc_zone_t *zone, size_t size)
{
	void *ret = NULL; /* Assignment avoids useless compiler warning. */

	posix_memalign(&ret, pagesize, size);

	return (ret);
}

static void
zone_free(malloc_zone_t *zone, void *ptr)
{

	free(ptr);
}

static void *
zone_realloc(malloc_zone_t *zone, void *ptr, size_t size)
{

	return (realloc(ptr, size));
}

static void *
zone_destroy(malloc_zone_t *zone)
{

	/* This function should never be called. */
	assert(false);
	return (NULL);
}

static size_t
zone_good_size(malloc_zone_t *zone, size_t size)
{
	size_t ret;
	void *p;

	/*
	 * Actually create an object of the appropriate size, then find out
	 * how large it could have been without moving up to the next size
	 * class.
	 */
	p = malloc(size);
	if (p != NULL) {
		ret = isalloc(p);
		free(p);
	} else
		ret = size;

	return (ret);
}

static void
zone_force_lock(malloc_zone_t *zone)
{

	_malloc_prefork();
}

static void
zone_force_unlock(malloc_zone_t *zone)
{

	_malloc_postfork();
}

static malloc_zone_t *
create_zone(void)
{

	assert(malloc_initialized);

	zone.size = (void *)zone_size;
	zone.malloc = (void *)zone_malloc;
	zone.calloc = (void *)zone_calloc;
	zone.valloc = (void *)zone_valloc;
	zone.free = (void *)zone_free;
	zone.realloc = (void *)zone_realloc;
	zone.destroy = (void *)zone_destroy;
	zone.zone_name = "jemalloc_zone";
	zone.batch_malloc = NULL;
	zone.batch_free = NULL;
	zone.introspect = &zone_introspect;

	zone_introspect.enumerator = NULL;
	zone_introspect.good_size = (void *)zone_good_size;
	zone_introspect.check = NULL;
	zone_introspect.print = NULL;
	zone_introspect.log = NULL;
	zone_introspect.force_lock = (void *)zone_force_lock;
	zone_introspect.force_unlock = (void *)zone_force_unlock;
	zone_introspect.statistics = NULL;

	return (&zone);
}

__attribute__((constructor))
void
jemalloc_darwin_init(void)
{
	extern unsigned malloc_num_zones;
	extern malloc_zone_t **malloc_zones;

	if (malloc_init_hard())
		abort();

	/*
	 * The following code is *not* thread-safe, so it's critical that
	 * initialization be manually triggered.
	 */

	/* Register the custom zones. */
	malloc_zone_register(create_zone());
	assert(malloc_zones[malloc_num_zones - 1] == &zone);

	/*
	 * Shift malloc_zones around so that zone is first, which makes it the
	 * default zone.
	 */
	assert(malloc_num_zones > 1);
	memmove(&malloc_zones[1], &malloc_zones[0],
		sizeof(malloc_zone_t *) * (malloc_num_zones - 1));
	malloc_zones[0] = &zone;
}

#elif defined(__GLIBC__) && !defined(__UCLIBC__)
/*
 * glibc provides the RTLD_DEEPBIND flag for dlopen which can make it possible
 * to inconsistently reference libc's malloc(3)-compatible functions
 * (bug 493541).
 *
 * These definitions interpose hooks in glibc.  The functions are actually
 * passed an extra argument for the caller return address, which will be
 * ignored.
 */
void (*__free_hook)(void *ptr) = free;
void *(*__malloc_hook)(size_t size) = malloc;
void *(*__realloc_hook)(void *ptr, size_t size) = realloc;
void *(*__memalign_hook)(size_t alignment, size_t size) = memalign;

#elif defined(RTLD_DEEPBIND)
/*
 * XXX On systems that support RTLD_GROUP or DF_1_GROUP, do their
 * implementations permit similar inconsistencies?  Should STV_SINGLETON
 * visibility be used for interposition where available?
 */
#  error "Interposing malloc is unsafe on this system without libc malloc hooks."
#endif

