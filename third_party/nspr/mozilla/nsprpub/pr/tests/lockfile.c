/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape Portable Runtime (NSPR).
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
** File:        lockfile.c
** Purpose:     test basic locking functions
**              Just because this times stuff, don't think its a perforamnce
**              test!!!
**
** Modification History:
** 19-May-97 AGarcia- Converted the test to accomodate the debug_mode flag.
**	         The debug mode will print all of the printfs associated with this test.
**			 The regress mode will be the default mode. Since the regress tool limits
**           the output to a one line status:PASS or FAIL,all of the printf statements
**			 have been handled with an if (debug_mode) statement.
** 04-June-97 AGarcia removed the Test_Result function. Regress tool has been updated to
**			recognize the return code from tha main program.
***********************************************************************/
/***********************************************************************
** Includes
***********************************************************************/
/* Used to get the command line option */
#include "plgetopt.h"

#include "prcmon.h"
#include "prerror.h"
#include "prinit.h"
#include "prinrval.h"
#include "prlock.h"
#include "prlog.h"
#include "prmon.h"
#include "prthread.h"
#include "prtypes.h"

#ifndef XP_MAC
#include "private/pprio.h"
#else
#include "pprio.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef XP_MAC
#include "prlog.h"
#define printf PR_LogPrint
extern void SetupMacPrintfLog(char *logFile);
#endif

PRIntn failed_already=0;
PRIntn debug_mode;

const static PRIntervalTime contention_interval = 50;

typedef struct LockContentious_s {
    PRLock *ml;
    PRInt32 loops;
    PRIntervalTime overhead;
    PRIntervalTime interval;
} LockContentious_t;

#define LOCKFILE "prlock.fil"



static PRIntervalTime NonContentiousLock(PRInt32 loops)
{
    PRFileDesc *_lockfile;
    while (loops-- > 0)
    {
        _lockfile = PR_Open(LOCKFILE, PR_CREATE_FILE|PR_RDWR, 0666);
        if (!_lockfile) {
            if (debug_mode) printf(
                "could not create lockfile: %d [%d]\n",
                PR_GetError(), PR_GetOSError());
            return PR_INTERVAL_NO_TIMEOUT;
        }
        PR_LockFile(_lockfile);
        PR_UnlockFile(_lockfile);
        PR_Close(_lockfile);
    }
    return 0;
}  /* NonContentiousLock */

static void PR_CALLBACK LockContender(void *arg)
{
    LockContentious_t *contention = (LockContentious_t*)arg;
    PRFileDesc *_lockfile;
    while (contention->loops-- > 0)
    {
        _lockfile = PR_Open(LOCKFILE, PR_CREATE_FILE|PR_RDWR, 0666);
        if (!_lockfile) {
            if (debug_mode) printf(
                "could not create lockfile: %d [%d]\n",
                PR_GetError(), PR_GetOSError());
            break;
        }
        PR_LockFile(_lockfile);
        PR_Sleep(contention->interval);
        PR_UnlockFile(_lockfile);
        PR_Close(_lockfile);
    }

}  /* LockContender */

/*
** Win16 requires things passed to Threads not be on the stack
*/
static LockContentious_t contention;

static PRIntervalTime ContentiousLock(PRInt32 loops)
{
    PRStatus status;
    PRThread *thread = NULL;
    PRIntervalTime overhead, timein = PR_IntervalNow();

    contention.loops = loops;
    contention.overhead = 0;
    contention.ml = PR_NewLock();
    contention.interval = contention_interval;
    thread = PR_CreateThread(
        PR_USER_THREAD, LockContender, &contention,
        PR_PRIORITY_LOW, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
    PR_ASSERT(thread != NULL);

    overhead = PR_IntervalNow() - timein;

    while (contention.loops > 0)
    {
        PR_Lock(contention.ml);
        contention.overhead += contention.interval;
        PR_Sleep(contention.interval);
        PR_Unlock(contention.ml);
    }

    timein = PR_IntervalNow();
    status = PR_JoinThread(thread);
    PR_DestroyLock(contention.ml);
    overhead += (PR_IntervalNow() - timein);
    return overhead + contention.overhead;
}  /* ContentiousLock */

static PRIntervalTime Test(
    const char* msg, PRIntervalTime (*test)(PRInt32 loops),
    PRInt32 loops, PRIntervalTime overhead)
{ 
    /*
     * overhead - overhead not measured by the test.
     * duration - wall clock time it took to perform test.
     * predicted - extra time test says should not be counted 
     *
     * Time accountable to the test is duration - overhead - predicted
     * All times are Intervals and accumulated for all iterations.
     */
    PRFloat64 elapsed;
    PRIntervalTime accountable, duration;    
    PRUintn spaces = strlen(msg);
    PRIntervalTime timeout, timein = PR_IntervalNow();
    PRIntervalTime predicted = test(loops);
    timeout = PR_IntervalNow();
    duration = timeout - timein;
    accountable = duration - predicted;
    accountable -= overhead;
    elapsed = (PRFloat64)PR_IntervalToMicroseconds(accountable);
    if (debug_mode) printf("%s:", msg);
    while (spaces++ < 50) if (debug_mode) printf(" ");
    if ((PRInt32)accountable < 0) {
        if (debug_mode) printf("*****.** usecs/iteration\n");
    } else {
        if (debug_mode) printf("%8.2f usecs/iteration\n", elapsed/loops);
    }
    return duration;
}  /* Test */

int main(int argc,  char **argv)
{
    PRIntervalTime duration;
    PRUint32 cpu, cpus = 2;
    PRInt32 loops = 100;

	
	/* The command line argument: -d is used to determine if the test is being run
	in debug mode. The regress tool requires only one line output:PASS or FAIL.
	All of the printfs associated with this test has been handled with a if (debug_mode)
	test.
	Usage: test_name -d
	*/
	PLOptStatus os;
	PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  /* debug mode */
			debug_mode = 1;
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);

 /* main test */
	
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

#ifdef XP_MAC
	SetupMacPrintfLog("lockfile.log");
	debug_mode = 1;
#endif

    if (argc > 1) loops = atoi(argv[1]);
    if (loops == 0) loops = 100;
    if (debug_mode) printf("Lock: Using %d loops\n", loops);

    cpus = (argc < 3) ? 2 : atoi(argv[2]);
    if (cpus == 0) cpus = 2;
    if (debug_mode) printf("Lock: Using %d cpu(s)\n", cpus);


    for (cpu = 1; cpu <= cpus; ++cpu)
    {
        if (debug_mode) printf("\nLockFile: Using %d CPU(s)\n", cpu);
        PR_SetConcurrency(cpu);
        
        duration = Test("LockFile non-contentious locking/unlocking", NonContentiousLock, loops, 0);
        (void)Test("LockFile contentious locking/unlocking", ContentiousLock, loops, duration);
    }

    PR_Delete(LOCKFILE);  /* try to get rid of evidence */

    if (debug_mode) printf("%s: test %s\n", "Lock(mutex) test", ((failed_already) ? "failed" : "passed"));
	if(failed_already)	
		return 1;
	else
		return 0;
}  /* main */

/* testlock.c */
