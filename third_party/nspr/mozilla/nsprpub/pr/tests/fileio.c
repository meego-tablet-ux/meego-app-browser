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
** Name: fileio.c
**
** Description: Program to copy one file to another. 
**
** Modification History:
** 14-May-97 AGarcia- Converted the test to accomodate the debug_mode flag.
**	         The debug mode will print all of the printfs associated with this test.
**			 The regress mode will be the default mode. Since the regress tool limits
**           the output to a one line status:PASS or FAIL,all of the printf statements
**			 have been handled with an if (debug_mode) statement.
** 04-June-97 AGarcia removed the Test_Result function. Regress tool has been updated to
**			recognize the return code from tha main program.
** 12-June-97 Revert to return code 0 and 1, remove debug option (obsolete).
***********************************************************************/

/***********************************************************************
** Includes
***********************************************************************/
#include "prinit.h"
#include "prthread.h"
#include "prlock.h"
#include "prcvar.h"
#include "prmon.h"
#include "prmem.h"
#include "prio.h"
#include "prlog.h"

#include <stdio.h>

#ifdef XP_MAC
#include "prsem.h"
#include "prlog.h"
#define printf PR_LogPrint
extern void SetupMacPrintfLog(char *logFile);
#else
#include "obsolete/prsem.h"
#endif


#define TBSIZE 1024

static PRUint8 tbuf[TBSIZE];

static PRFileDesc *t1, *t2;

PRIntn failed_already=0;
PRIntn debug_mode;
static void InitialSetup(void)
{
	PRUintn	i;
	PRInt32 nWritten, rv;
	
	t1 = PR_Open("t1.tmp", PR_CREATE_FILE | PR_RDWR, 0);
	PR_ASSERT(t1 != NULL);	
	
	for (i=0; i<TBSIZE; i++)
		tbuf[i] = i;
		
	nWritten = PR_Write((PRFileDesc*)t1, tbuf, TBSIZE);
	PR_ASSERT(nWritten == TBSIZE);	
   		
	rv = PR_Seek(t1,0,PR_SEEK_SET);
	PR_ASSERT(rv == 0);	

   	t2 = PR_Open("t2.tmp", PR_CREATE_FILE | PR_RDWR, 0);
	PR_ASSERT(t2 != NULL);	
}


static void VerifyAndCleanup(void)
{
	PRUintn	i;
	PRInt32 nRead, rv;
	
	for (i=0; i<TBSIZE; i++)
		tbuf[i] = 0;
		
	rv = PR_Seek(t2,0,PR_SEEK_SET);
	PR_ASSERT(rv == 0);	

	nRead = PR_Read((PRFileDesc*)t2, tbuf, TBSIZE);
	PR_ASSERT(nRead == TBSIZE);	
   		
	for (i=0; i<TBSIZE; i++)
		if (tbuf[i] != (PRUint8)i) {
			if (debug_mode) printf("data mismatch for index= %d \n", i);
			else failed_already=1;
		}
   	PR_Close(t1);
   	PR_Close(t2);

   	PR_Delete("t1.tmp");
   	PR_Delete("t2.tmp");

    if (debug_mode) printf("fileio test passed\n");
}


/*------------------ Following is the real test program ---------*/
/*
	Program to copy one file to another.  Two temporary files get
	created.  First one gets written in one write call.  Then,
	a reader thread reads from this file into a double buffer.
	The writer thread writes from double buffer into the other
	temporary file.  The second temporary file gets verified
	for accurate data.
*/

PRSemaphore	*emptyBufs;	/* number of empty buffers */
PRSemaphore *fullBufs;	/* number of buffers that are full */

#define BSIZE	100

struct {
	char data[BSIZE];
	PRUintn nbytes;		/* number of bytes in this buffer */
} buf[2];

static void PR_CALLBACK reader(void *arg)
{
	PRUintn	i = 0;
	PRInt32	nbytes;
	
	do {
		(void) PR_WaitSem(emptyBufs);
		nbytes = PR_Read((PRFileDesc*)arg, buf[i].data, BSIZE);
		if (nbytes >= 0) {
			buf[i].nbytes = nbytes;
			PR_PostSem(fullBufs);
			i = (i + 1) % 2;
		}
	} while (nbytes > 0);
}

static void PR_CALLBACK writer(void *arg)
{
	PRUintn	i = 0;
	PRInt32	nbytes;
	
	do {
		(void) PR_WaitSem(fullBufs);
		nbytes = buf[i].nbytes;
		if (nbytes > 0) {
			nbytes = PR_Write((PRFileDesc*)arg, buf[i].data, nbytes);
			PR_PostSem(emptyBufs);
			i = (i + 1) % 2;
		}
	} while (nbytes > 0);
}

int main(int argc, char **argv)
{
	PRThread *r, *w;


	PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

#ifdef XP_MAC
	SetupMacPrintfLog("fileio.log");
	debug_mode = 1;
#endif

    emptyBufs = PR_NewSem(2);	/* two empty buffers */

    fullBufs = PR_NewSem(0);	/* zero full buffers */

	/* Create initial temp file setup */
	InitialSetup();
	
	/* create the reader thread */
	
	r = PR_CreateThread(PR_USER_THREAD,
				      reader, t1, 
				      PR_PRIORITY_NORMAL,
				      PR_LOCAL_THREAD,
    				  PR_JOINABLE_THREAD,
				      0);

	w = PR_CreateThread(PR_USER_THREAD,
				      writer, t2, 
				      PR_PRIORITY_NORMAL,
                      PR_LOCAL_THREAD,
                      PR_JOINABLE_THREAD,
                      0);

    /* Do the joining for both threads */
    (void) PR_JoinThread(r);
    (void) PR_JoinThread(w);

    /* Do the verification and clean up */
    VerifyAndCleanup();

    PR_DestroySem(emptyBufs);
    PR_DestroySem(fullBufs);

    PR_Cleanup();

    if(failed_already)
    {
        printf("Fail\n");
        return 1;
    }
    else
    {
        printf("PASS\n");
        return 0;
    }


}
