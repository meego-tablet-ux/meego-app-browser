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
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
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

#include "prtypes.h"
#include "prtime.h"
#include "prlong.h"

#include "nss.h"
#include "secutil.h"
#include "secitem.h"
#include "pk11func.h"
#include "pk11pqg.h"

#if defined(XP_UNIX)
#include <unistd.h>
#endif

#include "plgetopt.h"

#define BPB 8 /* bits per byte. */

char  *progName;


const SEC_ASN1Template seckey_PQGParamsTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SECKEYPQGParams) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYPQGParams,prime) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYPQGParams,subPrime) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYPQGParams,base) },
    { 0, }
};



void
Usage(void)
{
    fprintf(stderr, "Usage:  %s\n", progName);
    fprintf(stderr, 
"-a   Output DER-encoded PQG params, BTOA encoded.\n"
"     -l prime-length       Length of prime in bits (1024 is default)\n"
"     -o file               Output to this file (default is stdout)\n"
"-b   Output DER-encoded PQG params in binary\n"
"     -l prime-length       Length of prime in bits (1024 is default)\n"
"     -o file               Output to this file (default is stdout)\n"
"-r   Output P, Q and G in ASCII hexadecimal. \n"
"     -l prime-length       Length of prime in bits (1024 is default)\n"
"     -o file               Output to this file (default is stdout)\n"   
"-g bits       Generate SEED this many bits long.\n"
);
    exit(-1);

}

SECStatus
outputPQGParams(PQGParams * pqgParams, PRBool output_binary, PRBool output_raw,
                FILE * outFile)
{
    PRArenaPool   * arena 		= NULL;
    char          * PQG;
    SECItem       * pItem;
    int             cc;
    SECStatus       rv;
    SECItem         encodedParams;

    if (output_raw) {
    	SECItem item;

	rv = PK11_PQG_GetPrimeFromParams(pqgParams, &item);
	if (rv) {
	    SECU_PrintError(progName, "PK11_PQG_GetPrimeFromParams");
	    return rv;
	}
	SECU_PrintInteger(outFile, &item,    "Prime",    1);
	SECITEM_FreeItem(&item, PR_FALSE);

	rv = PK11_PQG_GetSubPrimeFromParams(pqgParams, &item);
	if (rv) {
	    SECU_PrintError(progName, "PK11_PQG_GetPrimeFromParams");
	    return rv;
	}
	SECU_PrintInteger(outFile, &item, "Subprime", 1);
	SECITEM_FreeItem(&item, PR_FALSE);

	rv = PK11_PQG_GetBaseFromParams(pqgParams, &item);
	if (rv) {
	    SECU_PrintError(progName, "PK11_PQG_GetPrimeFromParams");
	    return rv;
	}
	SECU_PrintInteger(outFile, &item,     "Base",     1);
	SECITEM_FreeItem(&item, PR_FALSE);

	fprintf(outFile, "\n");
	return SECSuccess;
    }

    encodedParams.data = NULL;
    encodedParams.len  = 0;
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) {
    	SECU_PrintError(progName, "PORT_NewArena");
	return SECFailure;
    }
    pItem = SEC_ASN1EncodeItem(arena, &encodedParams, pqgParams,
			       seckey_PQGParamsTemplate);
    if (!pItem) {
    	SECU_PrintError(progName, "SEC_ASN1EncodeItem");
	PORT_FreeArena(arena, PR_FALSE);
    	return SECFailure;
    }
    if (output_binary) {
	size_t len;
	len = fwrite(encodedParams.data, 1, encodedParams.len, outFile);
	PORT_FreeArena(arena, PR_FALSE);
	if (len != encodedParams.len) {
	     fprintf(stderr, "%s: fwrite failed\n", progName);
	     return SECFailure;
	}
	return SECSuccess;
    }

    /* must be output ASCII */
    PQG = BTOA_DataToAscii(encodedParams.data, encodedParams.len);    
    PORT_FreeArena(arena, PR_FALSE);
    if (!PQG) {
    	SECU_PrintError(progName, "BTOA_DataToAscii");
	return SECFailure;
    }

    cc = fprintf(outFile,"%s\n",PQG);
    PORT_Free(PQG);
    if (cc <= 0) {
	 fprintf(stderr, "%s: fprintf failed\n", progName);
	 return SECFailure;
    }
    return SECSuccess;
}

SECStatus
outputPQGVerify(PQGVerify * pqgVerify, PRBool output_binary, PRBool output_raw,
                FILE * outFile)
{
    SECStatus rv = SECSuccess;
    if (output_raw) {
    	SECItem item;
	unsigned int counter;

	rv = PK11_PQG_GetHFromVerify(pqgVerify, &item);
	if (rv) {
	    SECU_PrintError(progName, "PK11_PQG_GetHFromVerify");
	    return rv;
	}
	SECU_PrintInteger(outFile, &item,        "h",        1);
	SECITEM_FreeItem(&item, PR_FALSE);

	rv = PK11_PQG_GetSeedFromVerify(pqgVerify, &item);
	if (rv) {
	    SECU_PrintError(progName, "PK11_PQG_GetSeedFromVerify");
	    return rv;
	}
	SECU_PrintInteger(outFile, &item,     "SEED",     1);
	fprintf(outFile, "    g:       %d\n", item.len * BPB);
	SECITEM_FreeItem(&item, PR_FALSE);

	counter = PK11_PQG_GetCounterFromVerify(pqgVerify);
	fprintf(outFile, "    counter: %d\n", counter);
	fprintf(outFile, "\n");
    }
    return rv;
}

int
main(int argc, char **argv)
{
    FILE          * outFile 		= NULL;
    char          * outFileName         = NULL;
    PQGParams     * pqgParams 		= NULL;
    PQGVerify     * pqgVerify           = NULL;
    int             keySizeInBits	= 1024;
    int             j;
    int             g                   = 0;
    SECStatus       rv 			= 0;
    SECStatus       passed 		= 0;
    PRBool          output_ascii	= PR_FALSE;
    PRBool          output_binary	= PR_FALSE;
    PRBool          output_raw		= PR_FALSE;
    PLOptState *optstate;
    PLOptStatus status;


    progName = strrchr(argv[0], '/');
    if (!progName)
	progName = strrchr(argv[0], '\\');
    progName = progName ? progName+1 : argv[0];

    /* Parse command line arguments */
    optstate = PL_CreateOptState(argc, argv, "?abg:l:o:r" );
    while ((status = PL_GetNextOpt(optstate)) == PL_OPT_OK) {
	switch (optstate->option) {

	  case 'l':
	    keySizeInBits = atoi(optstate->value);
	    break;

	  case 'a':
	    output_ascii = PR_TRUE;  
	    break;

	  case 'b':
	    output_binary = PR_TRUE;
	    break;

	  case 'r':
	    output_raw = PR_TRUE;
	    break;

	  case 'o':
	    if (outFileName) {
	    	PORT_Free(outFileName);
	    }
	    outFileName = PORT_Strdup(optstate->value);
	    if (!outFileName) {
		rv = -1;
	    }
	    break;

	  case 'g':
	    g = atoi(optstate->value);
	    break;

	  default:
	  case '?':
	    Usage();
	    break;

	}
    }
    PL_DestroyOptState(optstate);

    if (status == PL_OPT_BAD) {
        Usage();
    }

    /* exactly 1 of these options must be set. */
    if (1 != ((output_ascii  != PR_FALSE) + 
	      (output_binary != PR_FALSE) +
	      (output_raw    != PR_FALSE))) {
	Usage();
    }

    j = PQG_PBITS_TO_INDEX(keySizeInBits);
    if (j < 0) {
	fprintf(stderr, "%s: Illegal prime length, \n"
			"\tacceptable values are between 512 and 1024,\n"
			"\tand divisible by 64\n", progName);
	return 2;
    }
    if (g != 0 && (g < 160 || g >= 2048 || g % 8 != 0)) {
	fprintf(stderr, "%s: Illegal g bits, \n"
			"\tacceptable values are between 160 and 2040,\n"
			"\tand divisible by 8\n", progName);
	return 3;
    }

    if (!rv && outFileName) {
	outFile = fopen(outFileName, output_binary ? "wb" : "w");
	if (!outFile) {
	    fprintf(stderr, "%s: unable to open \"%s\" for writing\n",
		    progName, outFileName);
	    rv = -1;
	}
    }
    if (outFileName) {
	PORT_Free(outFileName);
    }
    if (rv != 0) {
	return 1;
    }

    if (outFile == NULL) {
	outFile = stdout;
    }


    NSS_NoDB_Init(NULL);

    if (g) 
	rv = PK11_PQG_ParamGenSeedLen((unsigned)j, (unsigned)(g/8), 
	                         &pqgParams, &pqgVerify);
    else 
	rv = PK11_PQG_ParamGen((unsigned)j, &pqgParams, &pqgVerify);
    /* below here, must go to loser */

    if (rv != SECSuccess || pqgParams == NULL || pqgVerify == NULL) {
	SECU_PrintError(progName, "PQG parameter generation failed.\n");
	goto loser;
    } 
    fprintf(stderr, "%s: PQG parameter generation completed.\n", progName);

    rv = outputPQGParams(pqgParams, output_binary, output_raw, outFile);
    if (rv) {
    	fprintf(stderr, "%s: failed to output PQG params.\n", progName);
	goto loser;
    }
    rv = outputPQGVerify(pqgVerify, output_binary, output_raw, outFile);
    if (rv) {
    	fprintf(stderr, "%s: failed to output PQG Verify.\n", progName);
	goto loser;
    }

    rv = PK11_PQG_VerifyParams(pqgParams, pqgVerify, &passed);
    if (rv != SECSuccess) {
	fprintf(stderr, "%s: PQG parameter verification aborted.\n", progName);
	goto loser;
    }
    if (passed != SECSuccess) {
	fprintf(stderr, "%s: PQG parameters failed verification.\n", progName);
	goto loser;
    } 
    fprintf(stderr, "%s: PQG parameters passed verification.\n", progName);

    PK11_PQG_DestroyParams(pqgParams);
    PK11_PQG_DestroyVerify(pqgVerify);
    return 0;

loser:
    PK11_PQG_DestroyParams(pqgParams);
    PK11_PQG_DestroyVerify(pqgVerify);
    return 1;
}
