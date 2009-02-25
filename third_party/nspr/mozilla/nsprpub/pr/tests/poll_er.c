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

/***********************************************************************
**
** Name: prpoll_err.c
**
** Description: This program tests PR_Poll with sockets.
**              error reporting operation is tested
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

#ifdef XP_BEOS
#include <stdio.h>
int main()
{
    printf( "This test is not ported to the BeOS\n" );
    return 0;
}
#else

/***********************************************************************
** Includes
***********************************************************************/
/* Used to get the command line option */
#include "plgetopt.h"

#include "primpl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

PRIntn failed_already=0;
PRIntn debug_mode;

static void
ClientThreadFunc(void *arg)
{
    PRFileDesc *badFD = (PRFileDesc *) arg;
    /*
     * Make the fd invalid
     */
#if defined(XP_UNIX)
    close(PR_FileDesc2NativeHandle(badFD));
#elif defined(XP_OS2)
    soclose(PR_FileDesc2NativeHandle(badFD));
#elif defined(WIN32) || defined(WIN16)
    closesocket(PR_FileDesc2NativeHandle(badFD));
#elif defined(XP_MAC)
    _PR_MD_CLOSE_SOCKET(PR_FileDesc2NativeHandle(badFD));
#else
#error "Unknown architecture"
#endif
}

int main(int argc, char **argv)
{
    PRFileDesc *listenSock1, *listenSock2;
    PRFileDesc *badFD;
    PRUint16 listenPort1, listenPort2;
    PRNetAddr addr;
    char buf[128];
    PRPollDesc pds0[10], pds1[10], *pds, *other_pds;
    PRIntn npds;
    PRInt32 retVal;

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

    if (debug_mode) {
		printf("This program tests PR_Poll with sockets.\n");
		printf("error reporting is  tested.\n\n");
	}

    /* Create two listening sockets */
    if ((listenSock1 = PR_NewTCPSocket()) == NULL) {
	fprintf(stderr, "Can't create a new TCP socket\n");
	failed_already=1;
	goto exit_now;
    }
    addr.inet.family = AF_INET;
    addr.inet.ip = PR_htonl(INADDR_ANY);
    addr.inet.port = PR_htons(0);
    if (PR_Bind(listenSock1, &addr) == PR_FAILURE) {
	fprintf(stderr, "Can't bind socket\n");
	failed_already=1;
	goto exit_now;
    }
    if (PR_GetSockName(listenSock1, &addr) == PR_FAILURE) {
	fprintf(stderr, "PR_GetSockName failed\n");
	failed_already=1;
	goto exit_now;
    }
    listenPort1 = PR_ntohs(addr.inet.port);
    if (PR_Listen(listenSock1, 5) == PR_FAILURE) {
	fprintf(stderr, "Can't listen on a socket\n");
	failed_already=1;
	goto exit_now;
    }

    if ((listenSock2  = PR_NewTCPSocket()) == NULL) {
	fprintf(stderr, "Can't create a new TCP socket\n");
	failed_already=1;	
	goto exit_now;
    }
    addr.inet.family = AF_INET;
    addr.inet.ip = PR_htonl(INADDR_ANY);
    addr.inet.port = PR_htons(0);
    if (PR_Bind(listenSock2, &addr) == PR_FAILURE) {
	fprintf(stderr, "Can't bind socket\n");
	failed_already=1;	
	goto exit_now;
    }
    if (PR_GetSockName(listenSock2, &addr) == PR_FAILURE) {
	fprintf(stderr, "PR_GetSockName failed\n");
	failed_already=1;	
	goto exit_now;
    }
    listenPort2 = PR_ntohs(addr.inet.port);
    if (PR_Listen(listenSock2, 5) == PR_FAILURE) {
	fprintf(stderr, "Can't listen on a socket\n");
	failed_already=1;	
	goto exit_now;
    }
    PR_snprintf(buf, sizeof(buf),
	    "The server thread is listening on ports %hu and %hu\n\n",
	    listenPort1, listenPort2);
    if (debug_mode) printf("%s", buf);

    /* Set up the poll descriptor array */
    pds = pds0;
    other_pds = pds1;
    memset(pds, 0, sizeof(pds));
    pds[0].fd = listenSock1;
    pds[0].in_flags = PR_POLL_READ;
    pds[1].fd = listenSock2;
    pds[1].in_flags = PR_POLL_READ;
    npds = 2;


    /* Testing bad fd */
    if (debug_mode) printf("PR_Poll should detect a bad file descriptor\n");
    if ((badFD = PR_NewTCPSocket()) == NULL) {
	fprintf(stderr, "Can't create a TCP socket\n");
	goto exit_now;
    }

    pds[2].fd = badFD;
    pds[2].in_flags = PR_POLL_READ;
    npds = 3;

    if (PR_CreateThread(PR_USER_THREAD, ClientThreadFunc,
            badFD, PR_PRIORITY_NORMAL, PR_LOCAL_THREAD,
            PR_UNJOINABLE_THREAD, 0) == NULL) {
        fprintf(stderr, "cannot create thread\n");
        exit(1);
    }

    retVal = PR_Poll(pds, npds, PR_INTERVAL_NO_TIMEOUT);
    if (retVal != 1 || (unsigned short) pds[2].out_flags != PR_POLL_NVAL) {
	fprintf(stderr, "Failed to detect the bad fd: "
		"PR_Poll returns %d, out_flags is 0x%hx\n",
		retVal, pds[2].out_flags);
	failed_already=1;	
	goto exit_now;
    }
    if (debug_mode) printf("PR_Poll detected the bad fd.  Test passed.\n\n");
    PR_Cleanup();
	goto exit_now;
exit_now:
	if(failed_already)	
		return 1;
	else
		return 0;
}

#endif /* XP_BEOS */
