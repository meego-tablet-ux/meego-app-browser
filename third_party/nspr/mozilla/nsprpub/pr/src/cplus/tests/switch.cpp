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
** File:            switch.cpp
** Description:     trying to time context switches
*/

#include "rccv.h"
#include "rcinrval.h"
#include "rclock.h"
#include "rcthread.h"

#include <prio.h>
#include <prlog.h>
#include <prprf.h>
#include <plerror.h>
#include <plgetopt.h>

#include <stdlib.h>

#define INNER_LOOPS 100
#define DEFAULT_LOOPS 100
#define DEFAULT_THREADS 10

static PRFileDesc *debug_out = NULL;
static PRBool debug_mode = PR_FALSE, verbosity = PR_FALSE, failed = PR_FALSE;

class Home: public RCCondition
{
public:
    virtual ~Home();
    Home(Home *link, RCLock* ml);

public:
    Home *next;
    RCLock* ml;
    PRBool twiddle;
};  /* Home */

Home::~Home() { }

Home::Home(Home *link, RCLock* lock): RCCondition(lock)
{
    ml = lock;
    next = link;
    twiddle = PR_FALSE;
}  /* Home::Home */

class Shared: public Home, public RCThread
{
public:
    Shared(RCThread::Scope scope, Home* link, RCLock* ml);

private:
    ~Shared();
    void RootFunction();
};  /* Shared */

Shared::Shared(RCThread::Scope scope, Home* link, RCLock* lock):
    Home(link, lock), RCThread(scope, RCThread::joinable) { }

Shared::~Shared() { }

void Shared::RootFunction()
{
    PRStatus status = PR_SUCCESS;
    while (PR_SUCCESS == status)
    {
        RCEnter entry(ml);
        while (twiddle && (PR_SUCCESS == status)) status = Wait();
		if (verbosity) PR_fprintf(debug_out, "+");
        twiddle = PR_TRUE;
        next->twiddle = PR_FALSE;
        next->Notify();
    }
}  /* Shared::RootFunction */

static void Help(void)
{
    debug_out = PR_STDOUT;

    PR_fprintf(
		debug_out, "Usage: >./switch [-d] [-c n] [-t n] [-T n] [-G]\n");
    PR_fprintf(
		debug_out, "-c n\tloops at thread level (default: %d)\n", DEFAULT_LOOPS);
    PR_fprintf(
		debug_out, "-t n\tnumber of threads (default: %d)\n", DEFAULT_THREADS);
    PR_fprintf(debug_out, "-d\tturn on debugging output (default: FALSE)\n");
    PR_fprintf(debug_out, "-v\tturn on verbose output (default: FALSE)\n");
    PR_fprintf(debug_out, "-G n\tglobal threads only (default: FALSE)\n");
    PR_fprintf(debug_out, "-C n\tconcurrency setting (default: 1)\n");
}  /* Help */

PRIntn main(PRIntn argc, char **argv)
{
	PLOptStatus os;
    PRStatus status;
    PRBool help = PR_FALSE;
    PRUintn concurrency = 1;
    RCThread::Scope thread_scope = RCThread::local;
    PRUintn thread_count, inner_count, loop_count, average;
    PRUintn thread_limit = DEFAULT_THREADS, loop_limit = DEFAULT_LOOPS;
	PLOptState *opt = PL_CreateOptState(argc, argv, "hdvc:t:C:G");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'v':  /* verbose mode */
			verbosity = PR_TRUE;
        case 'd':  /* debug mode */
			debug_mode = PR_TRUE;
            break;
        case 'c':  /* loop counter */
			loop_limit = atoi(opt->value);
            break;
        case 't':  /* thread limit */
			thread_limit = atoi(opt->value);
            break;
        case 'C':  /* Concurrency limit */
			concurrency = atoi(opt->value);
            break;
        case 'G':  /* global threads only */
			thread_scope = RCThread::global;
            break;
        case 'h':  /* help message */
			Help();
			help = PR_TRUE;
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);

    if (help) return -1;

	if (PR_TRUE == debug_mode)
	{
		debug_out = PR_STDOUT;
		PR_fprintf(debug_out, "Test parameters\n");
		PR_fprintf(debug_out, "\tThreads involved: %d\n", thread_limit);
		PR_fprintf(debug_out, "\tIteration limit: %d\n", loop_limit);
		PR_fprintf(debug_out, "\tConcurrency: %d\n", concurrency);
		PR_fprintf(
			debug_out, "\tThread type: %s\n",
			(PR_GLOBAL_THREAD == thread_scope) ? "GLOBAL" : "LOCAL");
	}

    /*
    ** The interesting part starts here
    */
    RCLock lock;
    Shared* shared;
    Home home(NULL, &lock);
    Home* link = &home;
    RCInterval timein, timeout = 0;

    /* Build up the string of objects */
    for (thread_count = 1; thread_count <= thread_limit; ++thread_count)
    {
        shared = new Shared(thread_scope, link, &lock);
        shared->Start();  /* make it run */
        link = (Home*)shared;
	}

    /* Pass the message around the horn a few times */
    for (loop_count = 1; loop_count <= loop_limit; ++loop_count)
    {
		timein.SetToNow();
		for (inner_count = 0; inner_count < INNER_LOOPS; ++inner_count)
		{
			RCEnter entry(&lock);
			home.twiddle = PR_TRUE;
			shared->twiddle = PR_FALSE;
			shared->Notify();
			while (home.twiddle)
            {
				failed = (PR_FAILURE == home.Wait()) ? PR_TRUE : PR_FALSE;
            }
		}
		timeout += (RCInterval(RCInterval::now) - timein);
	}

    /* Figure out how well we did */
	if (debug_mode)
	{
		average = timeout.ToMicroseconds()
			/ (INNER_LOOPS * loop_limit * thread_count);
		PR_fprintf(
			debug_out, "Average switch times %d usecs for %d threads\n",
            average, thread_limit);
	}

    /* Start reclamation process */
    link = shared;
    for (thread_count = 1; thread_count <= thread_limit; ++thread_count)
    {
        if (&home == link) break;
        status = ((Shared*)link)->Interrupt();
		if (PR_SUCCESS != status)
        {
            failed = PR_TRUE;
            if (debug_mode)
			    PL_FPrintError(debug_out, "Failed to interrupt");
        }
		link = link->next; 
    }

    for (thread_count = 1; thread_count <= thread_limit; ++thread_count)
    {
        link = shared->next;
        status = shared->Join();
		if (PR_SUCCESS != status)
		{
            failed = PR_TRUE;
            if (debug_mode)
			    PL_FPrintError(debug_out, "Failed to join");
        }
        if (&home == link) break;
        shared = (Shared*)link;
    }

    PR_fprintf(PR_STDOUT, ((failed) ? "FAILED\n" : "PASSED\n"));

    failed |= (PR_SUCCESS == RCPrimordialThread::Cleanup());

    return ((failed) ? 1 : 0);
}  /* main */

/* switch.c */
