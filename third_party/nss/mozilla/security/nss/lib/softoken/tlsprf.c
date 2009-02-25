/* tlsprf.c - TLS Pseudo Random Function (PRF) implementation
 *
 * ***** BEGIN LICENSE BLOCK *****
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
/* $Id: tlsprf.c,v 1.6 2005/08/06 09:27:28 nelsonb%netscape.com Exp $ */

#include "pkcs11i.h"
#include "blapi.h"

#define SFTK_OFFSETOF(str, memb) ((PRPtrdiff)(&(((str *)0)->memb)))

static void sftk_TLSPRFNull(void *data, PRBool freeit)
{
    return;
} 

typedef struct {
    PRUint32	   cxSize;	/* size of allocated block, in bytes.        */
    PRUint32       cxBufSize;   /* sizeof buffer at cxBufPtr.                */
    unsigned char *cxBufPtr;	/* points to real buffer, may be cxBuf.      */
    PRUint32	   cxKeyLen;	/* bytes of cxBufPtr containing key.         */
    PRUint32	   cxDataLen;	/* bytes of cxBufPtr containing data.        */
    SECStatus	   cxRv;	/* records failure of void functions.        */
    PRBool	   cxIsFIPS;	/* true if conforming to FIPS 198.           */
    unsigned char  cxBuf[512];	/* actual size may be larger than 512.       */
} TLSPRFContext;

static void
sftk_TLSPRFHashUpdate(TLSPRFContext *cx, const unsigned char *data, 
                        unsigned int data_len)
{
    PRUint32 bytesUsed = cx->cxKeyLen + cx->cxDataLen;

    if (cx->cxRv != SECSuccess)	/* function has previously failed. */
    	return;
    if (bytesUsed + data_len > cx->cxBufSize) {
	/* We don't use realloc here because 
	** (a) realloc doesn't zero out the old block, and 
	** (b) if realloc fails, we lose the old block.
	*/
	PRUint32 newBufSize = bytesUsed + data_len + 512;
    	unsigned char * newBuf = (unsigned char *)PORT_Alloc(newBufSize);
	if (!newBuf) {
	   cx->cxRv = SECFailure;
	   return;
	}
	PORT_Memcpy(newBuf, cx->cxBufPtr, bytesUsed);
	if (cx->cxBufPtr != cx->cxBuf) {
	    PORT_ZFree(cx->cxBufPtr, bytesUsed);
	}
	cx->cxBufPtr  = newBuf;
	cx->cxBufSize = newBufSize;
    }
    PORT_Memcpy(cx->cxBufPtr + bytesUsed, data, data_len);
    cx->cxDataLen += data_len;
}

static void 
sftk_TLSPRFEnd(TLSPRFContext *ctx, unsigned char *hashout,
	 unsigned int *pDigestLen, unsigned int maxDigestLen)
{
    *pDigestLen = 0; /* tells Verify that no data has been input yet. */
}

/* Compute the PRF values from the data previously input. */
static SECStatus
sftk_TLSPRFUpdate(TLSPRFContext *cx, 
                  unsigned char *sig,		/* output goes here. */
		  unsigned int * sigLen, 	/* how much output.  */
		  unsigned int   maxLen, 	/* output buffer size */
		  unsigned char *hash, 		/* unused. */
		  unsigned int   hashLen)	/* unused. */
{
    SECStatus rv;
    SECItem sigItem;
    SECItem seedItem;
    SECItem secretItem;

    if (cx->cxRv != SECSuccess)
    	return cx->cxRv;

    secretItem.data = cx->cxBufPtr;
    secretItem.len  = cx->cxKeyLen;

    seedItem.data = cx->cxBufPtr + cx->cxKeyLen;
    seedItem.len  = cx->cxDataLen;

    sigItem.data = sig;
    sigItem.len  = maxLen;

    rv = TLS_PRF(&secretItem, NULL, &seedItem, &sigItem, cx->cxIsFIPS);
    if (rv == SECSuccess && sigLen != NULL)
    	*sigLen = sigItem.len;
    return rv;

}

static SECStatus
sftk_TLSPRFVerify(TLSPRFContext *cx, 
                  unsigned char *sig, 		/* input, for comparison. */
		  unsigned int   sigLen,	/* length of sig.         */
		  unsigned char *hash, 		/* data to be verified.   */
		  unsigned int   hashLen)	/* size of hash data.     */
{
    unsigned char * tmp    = (unsigned char *)PORT_Alloc(sigLen);
    unsigned int    tmpLen = sigLen;
    SECStatus       rv;

    if (!tmp)
    	return SECFailure;
    if (hashLen) {
    	/* hashLen is non-zero when the user does a one-step verify.
	** In this case, none of the data has been input yet.
	*/
    	sftk_TLSPRFHashUpdate(cx, hash, hashLen);
    }
    rv = sftk_TLSPRFUpdate(cx, tmp, &tmpLen, sigLen, NULL, 0);
    if (rv == SECSuccess) {
    	rv = (SECStatus)(1 - !PORT_Memcmp(tmp, sig, sigLen));
    }
    PORT_ZFree(tmp, sigLen);
    return rv;
}

static void
sftk_TLSPRFHashDestroy(TLSPRFContext *cx, PRBool freeit)
{
    if (freeit) {
	if (cx->cxBufPtr != cx->cxBuf) 
	    PORT_ZFree(cx->cxBufPtr, cx->cxBufSize);
	PORT_ZFree(cx, cx->cxSize);
    }
}

CK_RV
sftk_TLSPRFInit(SFTKSessionContext *context, 
		  SFTKObject *        key, 
		  CK_KEY_TYPE         key_type)
{
    SFTKAttribute * keyVal;
    TLSPRFContext * prf_cx;
    CK_RV           crv = CKR_HOST_MEMORY;
    PRUint32        keySize;
    PRUint32        blockSize;

    if (key_type != CKK_GENERIC_SECRET)
    	return CKR_KEY_TYPE_INCONSISTENT; /* CKR_KEY_FUNCTION_NOT_PERMITTED */

    context->multi = PR_TRUE;

    keyVal = sftk_FindAttribute(key, CKA_VALUE);
    keySize = (!keyVal) ? 0 : keyVal->attrib.ulValueLen;
    blockSize = keySize + sizeof(TLSPRFContext);
    prf_cx = (TLSPRFContext *)PORT_Alloc(blockSize);
    if (!prf_cx) 
    	goto done;
    prf_cx->cxSize    = blockSize;
    prf_cx->cxKeyLen  = keySize;
    prf_cx->cxDataLen = 0;
    prf_cx->cxBufSize = blockSize - SFTK_OFFSETOF(TLSPRFContext, cxBuf);
    prf_cx->cxRv      = SECSuccess;
    prf_cx->cxIsFIPS  = (key->slot->slotID == FIPS_SLOT_ID);
    prf_cx->cxBufPtr  = prf_cx->cxBuf;
    if (keySize)
	PORT_Memcpy(prf_cx->cxBufPtr, keyVal->attrib.pValue, keySize);

    context->hashInfo    = (void *) prf_cx;
    context->cipherInfo  = (void *) prf_cx;
    context->hashUpdate  = (SFTKHash)    sftk_TLSPRFHashUpdate;
    context->end         = (SFTKEnd)     sftk_TLSPRFEnd;
    context->update      = (SFTKCipher)  sftk_TLSPRFUpdate;
    context->verify      = (SFTKVerify)  sftk_TLSPRFVerify;
    context->destroy     = (SFTKDestroy) sftk_TLSPRFNull;
    context->hashdestroy = (SFTKDestroy) sftk_TLSPRFHashDestroy;
    crv = CKR_OK;

done:
    if (keyVal) 
	sftk_FreeAttribute(keyVal);
    return crv;
}

