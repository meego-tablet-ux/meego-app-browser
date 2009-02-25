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
 * The Original Code is the PKIX-C library.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are
 * Copyright 2004-2007 Sun Microsystems, Inc.  All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems, Inc.
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
 * This file defines functions associated with CertStore types.
 *
 */


#ifndef _PKIX_SAMPLEMODULES_H
#define _PKIX_SAMPLEMODULES_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* General
 *
 * Please refer to the libpkix Programmer's Guide for detailed information
 * about how to use the libpkix library. Certain key warnings and notices from
 * that document are repeated here for emphasis.
 *
 * All identifiers in this file (and all public identifiers defined in
 * libpkix) begin with "PKIX_". Private identifiers only intended for use
 * within the library begin with "pkix_".
 *
 * A function returns NULL upon success, and a PKIX_Error pointer upon failure.
 *
 * Unless otherwise noted, for all accessor (gettor) functions that return a
 * PKIX_PL_Object pointer, callers should assume that this pointer refers to a
 * shared object. Therefore, the caller should treat this shared object as
 * read-only and should not modify this shared object. When done using the
 * shared object, the caller should release the reference to the object by
 * using the PKIX_PL_Object_DecRef function.
 *
 * While a function is executing, if its arguments (or anything referred to by
 * its arguments) are modified, free'd, or destroyed, the function's behavior
 * is undefined.
 *
 */

/* PKIX_PL_CollectionCertStore
 *
 * A PKIX_CollectionCertStore provides an example for showing how to retrieve
 * certificates and CRLs from a repository, such as a directory in the system.
 * It is expected the directory is an absolute directory which contains CRL
 * and Cert data files.  CRL files are expected to have the suffix of .crl
 * and Cert files are expected to have the suffix of .crt .
 *
 * Once the caller has created the CollectionCertStoreContext object, the caller
 * then can call pkix_pl_CollectionCertStore_GetCert or
 * pkix_pl_CollectionCertStore_GetCRL to obtain Lists of PKIX_PL_Cert or
 * PKIX_PL_CRL objects, respectively.
 */

/*
 * FUNCTION: PKIX_PL_CollectionCertStore_Create
 * DESCRIPTION:
 *
 *  Creates a new CollectionCertStore and returns it at
 *      "pColCertStore".
 *
 * PARAMETERS:
 *  "storeDir"
 *      The absolute path where *.crl files are located.
 *  "pColCertStoreContext"
 *      Address where object pointer will be stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CollectionCertStoreContext Error if the function fails in
 *      a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
PKIX_PL_CollectionCertStore_Create(
        PKIX_PL_String *storeDir,
        PKIX_CertStore **pCertStore,
        void *plContext);

/* PKIX_PL_PK11CertStore
 *
 * A PKIX_PL_PK11CertStore retrieves certificates and CRLs from a PKCS11
 * database. The directory that contains the cert8.db, key3.db, and secmod.db
 * files that comprise a PKCS11 database are specified in NSS initialization.
 *
 * Once the caller has created the Pk11CertStore object, the caller can call
 * pkix_pl_Pk11CertStore_GetCert or pkix_pl_Pk11CertStore_GetCert to obtain
 * a List of PKIX_PL_Certs or PKIX_PL_CRL objects, respectively.
 */

/*
 * FUNCTION: PKIX_PL_Pk11CertStore_Create
 * DESCRIPTION:
 *
 *  Creates a new Pk11CertStore and returns it at "pPk11CertStore".
 *
 * PARAMETERS:
 *  "pPk11CertStore"
 *      Address where object pointer will be stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
PKIX_PL_Pk11CertStore_Create(
        PKIX_CertStore **pPk11CertStore,
        void *plContext);

/* PKIX_PL_LdapCertStore
 *
 * A PKIX_PL_LdapCertStore retrieves certificates and CRLs from an LDAP server
 * over a socket connection. It used the LDAP protocol as described in RFC1777.
 *
 * Once the caller has created the LdapCertStore object, the caller can call
 * pkix_pl_LdapCertStore_GetCert or pkix_pl_LdapCertStore_GetCert to obtain
 * a List of PKIX_PL_Certs or PKIX_PL_CRL objects, respectively.
 */

/*
 * FUNCTION: PKIX_PL_LdapDefaultClient_Create
 * DESCRIPTION:
 *
 *  Creates an LdapDefaultClient using the PRNetAddr poined to by "sockaddr",
 *  with a timeout value of "timeout", and a BindAPI pointed to by "bindAPI";
 *  and stores the address of the default LdapClient at "pClient".
 *
 *  At the time of this version, there are unresolved questions about the LDAP
 *  protocol. Although RFC1777 describes a BIND and UNBIND message, it is not
 *  clear whether they are appropriate to this application. We have tested only
 *  using servers that do not expect authentication, and that reject BIND
 *  messages. It is not clear what values might be appropriate for the bindname
 *  and authentication fields, which are currently implemented as char strings
 *  supplied by the caller. (If this changes, the API and possibly the templates
 *  will have to change.) Therefore the Client_Create API contains a BindAPI
 *  structure, a union, which will have to be revised and extended when this
 *  area of the protocol is better understood.
 *
 * PARAMETERS:
 *  "sockaddr"
 *      Address of the PRNetAddr to be used for the socket connection. Must be
 *      non-NULL.
 *  "timeout"
 *      The PRIntervalTime value to be used as a timeout value in socket calls;
 *      a zero value indicates non-blocking I/O is to be used.
 *  "bindAPI"
 *      The address of a BindAPI to be used if a BIND message is required. If
 *      this argument is NULL, no Bind (or Unbind) will be sent.
 *  "pClient"
 *      Address where object pointer will be stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
PKIX_PL_LdapDefaultClient_Create(
        PRNetAddr *sockaddr,
        PRIntervalTime timeout,
        LDAPBindAPI *bindAPI,
        PKIX_PL_LdapDefaultClient **pClient,
        void *plContext);

/*
 * FUNCTION: PKIX_PL_LdapDefaultClient_CreateByName
 * DESCRIPTION:
 *
 *  Creates an LdapDefaultClient using the hostname poined to by "hostname",
 *  with a timeout value of "timeout", and a BindAPI pointed to by "bindAPI";
 *  and stores the address of the default LdapClient at "pClient".
 *
 *  At the time of this version, there are unresolved questions about the LDAP
 *  protocol. Although RFC1777 describes a BIND and UNBIND message, it is not
 *  clear whether they are appropriate to this application. We have tested only
 *  using servers that do not expect authentication, and that reject BIND
 *  messages. It is not clear what values might be appropriate for the bindname
 *  and authentication fields, which are currently implemented as char strings
 *  supplied by the caller. (If this changes, the API and possibly the templates
 *  will have to change.) Therefore the Client_Create API contains a BindAPI
 *  structure, a union, which will have to be revised and extended when this
 *  area of the protocol is better understood.
 *
 * PARAMETERS:
 *  "hostname"
 *      Address of the hostname to be used for the socket connection. Must be
 *      non-NULL.
 *  "timeout"
 *      The PRIntervalTime value to be used as a timeout value in socket calls;
 *      a zero value indicates non-blocking I/O is to be used.
 *  "bindAPI"
 *      The address of a BindAPI to be used if a BIND message is required. If
 *      this argument is NULL, no Bind (or Unbind) will be sent.
 *  "pClient"
 *      Address where object pointer will be stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
PKIX_PL_LdapDefaultClient_CreateByName(
        char *hostname,
        PRIntervalTime timeout,
        LDAPBindAPI *bindAPI,
        PKIX_PL_LdapDefaultClient **pClient,
        void *plContext);

/*
 * FUNCTION: PKIX_PL_LdapCertStore_Create
 * DESCRIPTION:
 *
 *  Creates a new LdapCertStore using the LdapClient pointed to by "client",
 *  and stores the address of the CertStore at "pCertStore".
 *
 * PARAMETERS:
 *  "client"
 *      Address of the LdapClient to be used. Must be non-NULL.
 *  "pCertStore"
 *      Address where object pointer will be stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
PKIX_PL_LdapCertStore_Create(
        PKIX_PL_LdapClient *client,
        PKIX_CertStore **pCertStore,
        void *plContext);

/*
 * FUNCTION: PKIX_PL_EkuChecker_Create
 *
 * DESCRIPTION:
 *  Create a CertChainChecker with EkuCheckerState and add it into
 *  PKIX_ProcessingParams object.
 *
 * PARAMETERS
 *  "params"
 *      a PKIX_ProcessingParams links to PKIX_ComCertSelParams where a list of
 *      Extended Key Usage OIDs specified by application can be retrieved for
 *      verification.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a UserDefinedModules Error if the function fails in a non-fatal
 *  way.
 *  Returns a Fatal Error
 */
PKIX_Error *
PKIX_PL_EkuChecker_Create(
        PKIX_ProcessingParams *params,
        void *plContext);

/*
 * FUNCTION: PKIX_PL_EkuChecker_GetRequiredEku
 *
 * DESCRIPTION:
 *  This function retrieves application specified ExtenedKeyUsage(s) from
 *  ComCertSetparams and converts its OID representations to SECCertUsageEnum.
 *  The result is stored and returned in bit mask at "pRequiredExtKeyUsage".
 *
 * PARAMETERS
 *  "certSelector"
 *      a PKIX_CertSelector links to PKIX_ComCertSelParams where a list of
 *      Extended Key Usage OIDs specified by application can be retrieved for
 *      verification. Must be non-NULL.
 *  "pRequiredExtKeyUsage"
 *      Address where the result is returned. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a UserDefinedModules Error if the function fails in a non-fatal
 *  way.
 *  Returns a Fatal Error
 */
PKIX_Error *
pkix_pl_EkuChecker_GetRequiredEku(
        PKIX_CertSelector *certSelector,
        PKIX_UInt32 *pRequiredExtKeyUsage,
        void *plContext);

/* PKIX_PL_NssContext
 *
 * A PKIX_PL_NssContext provides an example showing how the "plContext"
 * argument, that is part of every libpkix function call, can be used.
 * The "plContext" is the Portability Layer Context, which can be used
 * to communicate layer-specific information from the application to the
 * underlying Portability Layer (while bypassing the Portable Code, which
 * blindly passes the plContext on to every function call).
 *
 * In this case, NSS serves as both the application and the Portability Layer.
 * We define an NSS-specific structure, which includes an arena and a number
 * of SECCertificateUsage bit flags encoded as a PKIX_UInt32. A third argument,
 * wincx, is used on Windows platforms for PKCS11 access, and should be set to
 * NULL for other platforms.
 * Before calling any of the libpkix functions, the caller should create the NSS
 * context, by calling PKIX_PL_NssContext_Create, and provide that NSS context
 * as the "plContext" argument in every libpkix function call the caller makes.
 * When the caller is finished using the NSS context (usually just after he
 * calls PKIX_Shutdown), the caller should call PKIX_PL_NssContext_Destroy to
 * free the NSS context structure.
 */

/*
 * FUNCTION: PKIX_PL_NssContext_Create
 * DESCRIPTION:
 *
 *  Creates a new NssContext using the certificate usage(s) specified by
 *  "certUsage" and stores it at "pNssContext". This function also internally
 *  creates an arena and stores it as part of the NssContext structure. Unlike
 *  most other libpkix API functions, this function does not take a "plContext"
 *  parameter.
 *
 * PARAMETERS:
 *  "certUsage"
 *      The desired SECCertificateUsage(s).
 *  "useNssArena"
 *      Boolean flag indicates NSS Arena is used for memory allocation.
 *  "wincx"
 *      A Windows-dependent pointer for PKCS11 token handling.
 *  "pNssContext"
 *      Address where object pointer will be stored. Must be non-NULL.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Context Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
PKIX_PL_NssContext_Create(
        PKIX_UInt32 certificateUsage,
        PKIX_Boolean useNssArena,
        void *wincx,
        void **pNssContext);

/*
 * FUNCTION: PKIX_PL_NssContext_Destroy
 * DESCRIPTION:
 *
 *  Frees the structure pointed to by "nssContext" along with any of its
 *  associated memory. Unlike most other libpkix API functions, this function
 *  does not take a "plContext" parameter.
 *
 * PARAMETERS:
 *  "nssContext"
 *      Address of NssContext to be destroyed. Must be non-NULL.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Context Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
PKIX_PL_NssContext_Destroy(
        void *nssContext);

#ifdef __cplusplus
}
#endif

#endif /* _PKIX_SAMPLEMODULES_H */
