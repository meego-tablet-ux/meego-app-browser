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
 * Portions created by the Initial Developer are Copyright (C) 1999-2000
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

#include "nspr.h"
#include "plgetopt.h"

#include <stdio.h>

#define SEM_NAME1 "/tmp/foo.sem"
#define SEM_NAME2 "/tmp/bar.sem"
#define SEM_MODE  0666
#define ITERATIONS 1000

static PRBool debug_mode = PR_FALSE;
static PRIntn iterations = ITERATIONS;
static PRIntn counter;
static PRSem *sem1, *sem2;

/*
 * Thread 2 waits on semaphore 2 and posts to semaphore 1.
 */
void ThreadFunc(void *arg)
{
    PRIntn i;

    for (i = 0; i < iterations; i++) {
        if (PR_WaitSemaphore(sem2) == PR_FAILURE) {
            fprintf(stderr, "PR_WaitSemaphore failed\n");
            exit(1);
        }
        if (counter == 2*i+1) {
            if (debug_mode) printf("thread 2: counter = %d\n", counter);
        } else {
            fprintf(stderr, "thread 2: counter should be %d but is %d\n",
                    2*i+1, counter);
            exit(1);
        }
        counter++;
        if (PR_PostSemaphore(sem1) == PR_FAILURE) {
            fprintf(stderr, "PR_PostSemaphore failed\n");
            exit(1);
        }
    }
}

static void Help(void)
{
    fprintf(stderr, "sema test program usage:\n");
    fprintf(stderr, "\t-d           debug mode         (FALSE)\n");
    fprintf(stderr, "\t-c <count>   loop count         (%d)\n", ITERATIONS);
    fprintf(stderr, "\t-h           this message\n");
}  /* Help */

int main(int argc, char **argv)
{
    PRThread *thred;
    PRIntn i;
    PLOptStatus os;
    PLOptState *opt = PL_CreateOptState(argc, argv, "dc:h");

    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt))) {
        if (PL_OPT_BAD == os) continue;
        switch (opt->option) {
            case 'd':  /* debug mode */
                debug_mode = PR_TRUE;
                break;
            case 'c':  /* loop count */
                iterations = atoi(opt->value);
                break;
            case 'h':
            default:
                Help();
                return 2;
        }
    }
    PL_DestroyOptState(opt);

    if (PR_DeleteSemaphore(SEM_NAME1) == PR_SUCCESS) {
        fprintf(stderr, "warning: removed semaphore %s left over "
                "from previous run\n", SEM_NAME1);
    }
    if (PR_DeleteSemaphore(SEM_NAME2) == PR_SUCCESS) {
        fprintf(stderr, "warning: removed semaphore %s left over "
                "from previous run\n", SEM_NAME2);
    }

    sem1 = PR_OpenSemaphore(SEM_NAME1, PR_SEM_CREATE, SEM_MODE, 1);
    if (NULL == sem1) {
        fprintf(stderr, "PR_OpenSemaphore failed (%d, %d)\n",
                PR_GetError(), PR_GetOSError());
        exit(1);
    }
    sem2 = PR_OpenSemaphore(SEM_NAME2, PR_SEM_CREATE, SEM_MODE, 0);
    if (NULL == sem2) {
        fprintf(stderr, "PR_OpenSemaphore failed\n");
        exit(1);
    }
    thred = PR_CreateThread(PR_USER_THREAD, ThreadFunc, NULL,
            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (NULL == thred) {
        fprintf(stderr, "PR_CreateThread failed\n");
        exit(1);
    }

    /*
     * Thread 1 waits on semaphore 1 and posts to semaphore 2.
     */
    for (i = 0; i < iterations; i++) {
        if (PR_WaitSemaphore(sem1) == PR_FAILURE) {
            fprintf(stderr, "PR_WaitSemaphore failed\n");
            exit(1);
        }
        if (counter == 2*i) {
            if (debug_mode) printf("thread 1: counter = %d\n", counter);
        } else {
            fprintf(stderr, "thread 1: counter should be %d but is %d\n",
                    2*i, counter);
            exit(1);
        }
        counter++;
        if (PR_PostSemaphore(sem2) == PR_FAILURE) {
            fprintf(stderr, "PR_PostSemaphore failed\n");
            exit(1);
        }
    }

    if (PR_JoinThread(thred) == PR_FAILURE) {
        fprintf(stderr, "PR_JoinThread failed\n");
        exit(1);
    }

    if (PR_CloseSemaphore(sem1) == PR_FAILURE) {
        fprintf(stderr, "PR_CloseSemaphore failed\n");
    }
    if (PR_CloseSemaphore(sem2) == PR_FAILURE) {
        fprintf(stderr, "PR_CloseSemaphore failed\n");
    }
    if (PR_DeleteSemaphore(SEM_NAME1) == PR_FAILURE) {
        fprintf(stderr, "PR_DeleteSemaphore failed\n");
    }
    if (PR_DeleteSemaphore(SEM_NAME2) == PR_FAILURE) {
        fprintf(stderr, "PR_DeleteSemaphore failed\n");
    }
    printf("PASS\n");
    return 0;
}
