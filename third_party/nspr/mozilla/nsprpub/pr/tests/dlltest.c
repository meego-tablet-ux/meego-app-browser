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
** Name: dlltest.c
**
** Description: test dll functionality.
**
** Modification History:
** 14-May-97 AGarcia- Converted the test to accomodate the debug_mode flag.
**	         The debug mode will print all of the printfs associated with this test.
**			 The regress mode will be the default mode. Since the regress tool limits
**           the output to a one line status:PASS or FAIL,all of the printf statements
**			 have been handled with an if (debug_mode) statement.
** 04-June-97 AGarcia removed the Test_Result function. Regress tool has been updated to
**			recognize the return code from tha main program.
** 12-June-97 Revert to return code 0 and 1.
***********************************************************************/

/***********************************************************************
** Includes
***********************************************************************/
#include "prinit.h"
#include "prlink.h"
#include "prmem.h"
#include "prerror.h"

#include "plstr.h"

#include <stdio.h>
#include <stdlib.h>

typedef PRIntn (PR_CALLBACK *GetFcnType)(void);
typedef void (PR_CALLBACK *SetFcnType)(PRIntn);

PRIntn failed_already=0;
PRIntn debug_mode;

int main(int argc, char** argv)
{
    PRLibrary *lib, *lib2;  /* two handles to the same library */
    GetFcnType getFcn;
    SetFcnType setFcn;
    PRIntn value;
    PRStatus status;
    char *libName;

    if (argc >= 2 && PL_strcmp(argv[1], "-d") == 0) {
        debug_mode = 1;
    }

    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

    /*
     * Test 1: load the library, look up the symbols, call the functions,
     * and check the results.
     */

    libName = PR_GetLibraryName("dll", "my");
    if (debug_mode) printf("Loading library %s\n", libName);
    lib = PR_LoadLibrary(libName);
    PR_FreeLibraryName(libName);
    if (lib == NULL) {
        PRInt32 textLength = PR_GetErrorTextLength();
        char *text = (char*)PR_MALLOC(textLength);
        (void)PR_GetErrorText(text);
        fprintf(
            stderr, "PR_LoadLibrary failed (%d, %d, %s)\n",
            PR_GetError(), PR_GetOSError(), text);
        if (!debug_mode) failed_already=1;
    }
    getFcn = (GetFcnType) PR_FindSymbol(lib, "My_GetValue");
    setFcn = (SetFcnType) PR_FindFunctionSymbol(lib, "My_SetValue");
    (*setFcn)(888);
    value = (*getFcn)();
    if (value != 888) {
        fprintf(stderr, "Test 1 failed: set value to 888, but got %d\n", value);
        if (!debug_mode) failed_already=1;
    }
    if (debug_mode) printf("Test 1 passed\n");

    /*
     * Test 2: get a second handle to the same library (this should increment
     * the reference count), look up the symbols, call the functions, and
     * check the results.
     */

    getFcn = (GetFcnType) PR_FindSymbolAndLibrary("My_GetValue", &lib2);
    if (NULL == getFcn || lib != lib2) {
        fprintf(stderr, "Test 2 failed: handles for the same library are not "
            "equal: handle 1: %p, handle 2: %p\n", lib, lib2);
        if (!debug_mode) failed_already=1;
    }
    setFcn = (SetFcnType) PR_FindSymbol(lib2, "My_SetValue");
    value = (*getFcn)();
    if (value != 888) {
        fprintf(stderr, "Test 2 failed: value should be 888, but got %d\n",
            value);
        if (!debug_mode) failed_already=1;
    }
    (*setFcn)(777);
    value = (*getFcn)();
    if (value != 777) {
        fprintf(stderr, "Test 2 failed: set value to 777, but got %d\n", value);
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    if (debug_mode) printf("Test 2 passed\n");

    /*
     * Test 3: unload the library.  The library should still be accessible
     * via the second handle.  do the same things as above.
     */

    status = PR_UnloadLibrary(lib);
    if (PR_FAILURE == status) {
        fprintf(stderr, "Test 3 failed: cannot unload library: (%d, %d)\n",
            PR_GetError(), PR_GetOSError());
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    getFcn = (GetFcnType) PR_FindFunctionSymbol(lib2, "My_GetValue");
    setFcn = (SetFcnType) PR_FindSymbol(lib2, "My_SetValue");
    (*setFcn)(666);
    value = (*getFcn)();
    if (value != 666) {
        fprintf(stderr, "Test 3 failed: set value to 666, but got %d\n", value);
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    if (debug_mode) printf("Test 3 passed\n");

    /*
     * Test 4: unload the library, testing the reference count mechanism.
     */

    status = PR_UnloadLibrary(lib2);
    if (PR_FAILURE == status) {
        fprintf(stderr, "Test 4 failed: cannot unload library: (%d, %d)\n",
            PR_GetError(), PR_GetOSError());
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    getFcn = (GetFcnType) PR_FindFunctionSymbolAndLibrary("My_GetValue", &lib2);
    if (NULL != getFcn) {
        fprintf(stderr, "Test 4 failed: how can we find a symbol "
            "in an already unloaded library?\n");
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    if (debug_mode) {
        printf("Test 4 passed\n");
    }

    /*
    ** Test 5: LoadStaticLibrary()
    */
    {
        PRStaticLinkTable   slt[10];
        PRLibrary           *lib;
        
        lib = PR_LoadStaticLibrary( "my.dll", slt );
        if ( lib == NULL )
        {
            fprintf(stderr, "Test 5: LoadStatiLibrary() failed\n" );
            goto exit_now;
        }
        if (debug_mode)
        {
            printf("Test 5 passed\n");
        }
    }

    goto exit_now;
exit_now: 
    PR_Cleanup();

    if (failed_already) {
        printf("FAILED\n");
        return 1;
    } else {
        printf("PASSED\n");
        return 0;
    }
}
