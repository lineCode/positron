/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
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
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsIOService.h"
#include "nsIProtocolHandler.h"
#include "nscore.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIFileTransportService.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "prprf.h"
#include "nsLoadGroup.h"
#include "nsInputStreamChannel.h"
#include "nsXPIDLString.h" 
#include "nsReadableUtils.h"
#include "nsIErrorService.h" 
#include "netCore.h"
#include "nsIObserverService.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIPrefService.h"
#include "nsIPrefBranchInternal.h"
#include "nsIPrefLocalizedString.h"
#include "nsICategoryManager.h"
#include "nsIURLParser.h"
#include "nsISupportsPrimitives.h"
#include "nsIProxiedProtocolHandler.h"
#include "nsIProxyInfo.h"
#include "nsITimelineService.h"
#include "nsEscape.h"
#include "nsNetCID.h"

#define PORT_PREF_PREFIX     "network.security.ports."
#define PORT_PREF(x)         PORT_PREF_PREFIX x

static NS_DEFINE_CID(kFileTransportService, NS_FILETRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kDNSServiceCID, NS_DNSSERVICE_CID);
static NS_DEFINE_CID(kErrorServiceCID, NS_ERRORSERVICE_CID);
static NS_DEFINE_CID(kProtocolProxyServiceCID, NS_PROTOCOLPROXYSERVICE_CID);
static NS_DEFINE_CID(kStdURLParserCID, NS_STDURLPARSER_CID);

// A general port blacklist.  Connections to these ports will not be avoided unless 
// the protocol overrides.
//
// TODO: I am sure that there are more ports to be added.  
//       This cut is based on the classic mozilla codebase

PRInt16 gBadPortList[] = { 
  1,    // tcpmux          
  7,    // echo     
  9,    // discard          
  11,   // systat   
  13,   // daytime          
  15,   // netstat  
  17,   // qotd             
  19,   // chargen  
  20,   // ftp-data         
  21,   // ftp-cntl 
  22,   // ssh              
  23,   // telnet   
  25,   // smtp     
  37,   // time     
  42,   // name     
  43,   // nicname  
  53,   // domain  
  77,   // priv-rjs 
  79,   // finger   
  87,   // ttylink  
  95,   // supdup   
  101,  // hostriame
  102,  // iso-tsap 
  103,  // gppitnp  
  104,  // acr-nema 
  109,  // pop2     
  110,  // pop3     
  111,  // sunrpc   
  113,  // auth     
  115,  // sftp     
  117,  // uucp-path
  119,  // nntp     
  123,  // NTP
  135,  // loc-srv / epmap         
  139,  // netbios
  143,  // imap2  
  179,  // BGP
  389,  // ldap        
  512,  // print / exec          
  513,  // login         
  514,  // shell         
  515,  // printer         
  526,  // tempo         
  530,  // courier        
  531,  // Chat         
  532,  // netnews        
  540,  // uucp       
  556,  // remotefs    
  587,  //
  601,  //       
  1080, // SOCKS
  2049, // nfs
  4045, // lockd
  6000, // x11        
  0,    // This MUST be zero so that we can populating the array
};

static const char kProfileChangeNetTeardownTopic[] = "profile-change-net-teardown";
static const char kProfileChangeNetRestoreTopic[] = "profile-change-net-restore";

////////////////////////////////////////////////////////////////////////////////

nsIOService::nsIOService()
    : mOffline(PR_FALSE),
      mOfflineForProfileChange(PR_FALSE)
{
    NS_INIT_REFCNT();
}

nsresult
nsIOService::Init()
{
    nsresult rv = NS_OK;
    
    // Hold onto the eventQueue service.  We do not want any eventqueues to go away
    // when we shutdown until we process all remaining transports

    if (NS_SUCCEEDED(rv))
        mEventQueueService = do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &rv);
  
    
    // We need to get references to these services so that we can shut them
    // down later. If we wait until the nsIOService is being shut down,
    // GetService will fail at that point.
    rv = nsServiceManager::GetService(kSocketTransportServiceCID,
                                      NS_GET_IID(nsISocketTransportService),
                                      getter_AddRefs(mSocketTransportService));

    if (NS_FAILED(rv)) return rv;

    rv = nsServiceManager::GetService(kFileTransportService,
                                      NS_GET_IID(nsIFileTransportService),
                                      getter_AddRefs(mFileTransportService));
    if (NS_FAILED(rv)) return rv;

    rv = nsServiceManager::GetService(kDNSServiceCID,
                                      NS_GET_IID(nsIDNSService),
                                      getter_AddRefs(mDNSService));
    if (NS_FAILED(rv)) return rv;

    rv = nsServiceManager::GetService(kProtocolProxyServiceCID,
                                      NS_GET_IID(nsIProtocolProxyService),
                                      getter_AddRefs(mProxyService));
    if (NS_FAILED(rv)) return rv;

    // XXX hack until xpidl supports error info directly (http://bugzilla.mozilla.org/show_bug.cgi?id=13423)
    nsCOMPtr<nsIErrorService> errorService = do_GetService(kErrorServiceCID, &rv);
    if (NS_FAILED(rv)) 
        return rv;
    
    rv = errorService->RegisterErrorStringBundle(NS_ERROR_MODULE_NETWORK, NECKO_MSGS_URL);
    if (NS_FAILED(rv)) return rv;
    rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_READ_FROM, "ReadFrom");
    if (NS_FAILED(rv)) return rv;
    rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_WROTE_TO, "WroteTo");
    if (NS_FAILED(rv)) return rv;
    rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_RESOLVING_HOST, "ResolvingHost");
    if (NS_FAILED(rv)) return rv;
    rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_CONNECTED_TO, "ConnectedTo");
    if (NS_FAILED(rv)) return rv;
    rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_SENDING_TO, "SendingTo");
    if (NS_FAILED(rv)) return rv;
    rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_RECEIVING_FROM, "ReceivingFrom");
    if (NS_FAILED(rv)) return rv;
    rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_CONNECTING_TO, "ConnectingTo");
    if (NS_FAILED(rv)) return rv;
    
    // setup our bad port list stuff
    for(int i=0; gBadPortList[i]; i++)
    {
        mRestrictedPortList.AppendElement((void*)gBadPortList[i]);
    }

    // Further modifications to the port list come from prefs
    nsCOMPtr<nsIPrefBranch> prefBranch;
    GetPrefBranch(getter_AddRefs(prefBranch));
    if (prefBranch) {
        nsCOMPtr<nsIPrefBranchInternal> pbi = do_QueryInterface(prefBranch);
        if (pbi)
            pbi->AddObserver(PORT_PREF_PREFIX, this, PR_TRUE);
        PrefsChanged(prefBranch);
    }
    
    // Register for profile change notifications
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1");
    NS_ASSERTION(observerService, "Failed to get observer service");
    if (observerService) {
        observerService->AddObserver(this, kProfileChangeNetTeardownTopic, PR_TRUE);
        observerService->AddObserver(this, kProfileChangeNetRestoreTopic, PR_TRUE);
    }
    
    return NS_OK;
}


nsIOService::~nsIOService()
{
    // mURLParsers is a voidarray; we must release ourselves
    for (PRInt32 i = 0; i < mURLParsers.Count(); i++)
    {
        nsISupports *temp;
        temp = NS_STATIC_CAST(nsISupports*, mURLParsers[i]);
        NS_IF_RELEASE(temp);
    }
    (void)SetOffline(PR_TRUE);
    if (mFileTransportService)
        (void)mFileTransportService->Shutdown();
}   

NS_METHOD
nsIOService::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    static  nsISupports *_rValue = nsnull;

    nsresult rv;
    NS_ENSURE_NO_AGGREGATION(aOuter);

    if (_rValue)
    {
        NS_ADDREF (_rValue);
        *aResult = _rValue;
        return NS_OK;
    }

    nsIOService* _ios = new nsIOService();
    if (_ios == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(_ios);
    rv = _ios->Init();
    if (NS_FAILED(rv))
    {
        delete _ios;
        return rv;
    }

    rv = _ios->QueryInterface(aIID, aResult);

    if (NS_FAILED(rv))
    {
        delete _ios;
        return rv;
    }
    
    _rValue = NS_STATIC_CAST (nsISupports*, *aResult);
    NS_RELEASE (_rValue);
    _rValue = nsnull;

    return rv;
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsIOService,
                              nsIIOService,
                              nsIObserver,
                              nsISupportsWeakReference);

////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsIOService::CacheProtocolHandler(const char *scheme, nsIProtocolHandler *handler)
{
    for (unsigned int i=0; i<NS_N(gScheme); i++)
    {
        if (!nsCRT::strcasecmp(scheme, gScheme[i]))
        {
            nsresult rv;
            NS_ASSERTION(!mWeakHandler[i], "Protocol handler already cached");
            // Make sure the handler supports weak references.
            nsCOMPtr<nsISupportsWeakReference> factoryPtr = do_QueryInterface(handler, &rv);
            if (!factoryPtr)
            {
                // Dont cache handlers that dont support weak reference as
                // there is real danger of a circular reference.
#ifdef DEBUG_dp
                printf("DEBUG: %s protcol handler doesn't support weak ref. Not cached.\n", scheme);
#endif /* DEBUG_dp */
                return NS_ERROR_FAILURE;
            }
            mWeakHandler[i] = getter_AddRefs(NS_GetWeakReference(handler));
            return NS_OK;
        }
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsIOService::GetCachedProtocolHandler(const char *scheme, nsIProtocolHandler **result, PRUint32 start, PRUint32 end)
{
    PRUint32 len = end - start - 1;
    for (unsigned int i=0; i<NS_N(gScheme); i++)
    {
        if (!mWeakHandler[i])
            continue;

        // handle unterminated strings
        // start is inclusive, end is exclusive, len = end - start - 1
        if (end ? (!nsCRT::strncasecmp(scheme + start, gScheme[i], len)
                   && gScheme[i][len] == '\0')
                : (!nsCRT::strcasecmp(scheme, gScheme[i])))
        {
            nsCOMPtr<nsIProtocolHandler> temp = do_QueryReferent(mWeakHandler[i]);
            if (temp)
            {
                *result = temp.get();
                NS_ADDREF(*result);
                return NS_OK;
            }
        }
    }
    return NS_ERROR_FAILURE;
}
 
NS_IMETHODIMP
nsIOService::CacheURLParser(const char *scheme, nsIURLParser *parser)
{
    NS_ENSURE_ARG_POINTER(scheme);
    NS_ENSURE_ARG_POINTER(parser);
    for (PRInt32 i=0; i< (PRInt32) NS_N(gScheme); i++)
    {
        if (!nsCRT::strcasecmp(scheme, gScheme[i]))
        {
            // we're going to store this in an nsVoidArray, which extends,
            // unlike nsSupportsArray, which doesn't.  We must release
            // them on delete!
            // grab this before overwriting it
            nsIURLParser *old_parser;
            old_parser = NS_STATIC_CAST(nsIURLParser*,mURLParsers.SafeElementAt(i));

            NS_ADDREF(parser);
            mURLParsers.ReplaceElementAt(parser, i);
            // release any old entry, if any, AFTER adding new entry in
            // case they were the same (paranoia)
            NS_IF_RELEASE(old_parser);
            return NS_OK;
        }
    }
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsIOService::GetCachedURLParser(const char *scheme, nsIURLParser **result)
{
    PRInt32 count = mURLParsers.Count();
    for (PRInt32 i=0; i < count; i++)
    {
        if (!nsCRT::strcasecmp(scheme, gScheme[i]))
        {
            *result = NS_STATIC_CAST(nsIURLParser*, mURLParsers[i]);
            NS_IF_ADDREF(*result);
            return *result ? NS_OK : NS_ERROR_FAILURE;
        }
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsIOService::GetProtocolHandler(const char* scheme, nsIProtocolHandler* *result)
{
    nsresult rv;

    NS_ASSERTION(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX_LENGTH
                 == nsCRT::strlen(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX),
                 "need to fix NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX_LENGTH");

    NS_ENSURE_ARG_POINTER(scheme);
    // XXX we may want to speed this up by introducing our own protocol 
    // scheme -> protocol handler mapping, avoiding the string manipulation
    // and service manager stuff

    rv = GetCachedProtocolHandler(scheme, result);
    if (NS_SUCCEEDED(rv)) return NS_OK;

    nsCAutoString contractID(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX);
    contractID += scheme;
    ToLowerCase(contractID);

    rv = CallGetService(contractID.get(), result);
    if (NS_FAILED(rv)) 
    {
      // okay we don't have a protocol handler to handle this url type, so use the default protocol handler.
      // this will cause urls to get dispatched out to the OS ('cause we can't do anything with them) when 
      // we try to read from a channel created by the default protocol handler.

      rv = CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX"default",
                          result);
      if (NS_FAILED(rv)) return NS_ERROR_UNKNOWN_PROTOCOL;
    }

    CacheProtocolHandler(scheme, *result);

    return NS_OK;
}

NS_IMETHODIMP
nsIOService::ExtractScheme(const char* inURI, PRUint32 *startPos, 
                           PRUint32 *endPos, char* *scheme)
{
    return ExtractURLScheme(inURI, startPos, endPos, scheme);
}

/* nsIURLParser getParserForScheme (in string scheme); */
NS_IMETHODIMP 
nsIOService::GetParserForScheme(const char *scheme, nsIURLParser **_retval)
{
    nsresult rv;
    
    rv = GetCachedURLParser(scheme, _retval);
    if (NS_SUCCEEDED(rv) && *_retval) return NS_OK;


    if (!scheme) {
        if (!mDefaultURLParser) {
            rv = nsServiceManager::GetService(kStdURLParserCID, 
                                              NS_GET_IID(nsIURLParser), 
                                              getter_AddRefs(mDefaultURLParser));
            if (NS_FAILED(rv)) return rv;
        }
        
        *_retval = mDefaultURLParser;
        NS_ADDREF(*_retval);
        return NS_OK;
    }

    nsCOMPtr<nsICategoryManager> catmgr(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISimpleEnumerator> parserList;
    rv = catmgr->EnumerateCategory(NS_IURLPARSER_KEY, getter_AddRefs(parserList));
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsISupportsString> entry;

    // Walk the list of parsers...
    while (1) {
        rv = parserList->GetNext(getter_AddRefs(entry));
        if (NS_FAILED(rv)) break;

        // get the entry string
        nsXPIDLCString entryString;
        rv = entry->GetData(getter_Copies(entryString));
        if (NS_FAILED(rv)) break;

        if (nsCRT::strcmp(entryString, scheme) == 0) {
            nsXPIDLCString contractID;
            rv = catmgr->GetCategoryEntry(NS_IURLPARSER_KEY,(const char *)entryString, getter_Copies(contractID));
            if (NS_FAILED(rv)) break;

            rv = nsServiceManager::GetService(contractID, NS_GET_IID(nsIURLParser), (nsISupports **)_retval);
            if (NS_FAILED(rv))
                return rv;
            if (*_retval)
                CacheURLParser(scheme, *_retval);
            return *_retval ? NS_OK : NS_ERROR_FAILURE;
        }
    }

    // no registered url parser. Just use the default...
    if (!mDefaultURLParser) {
        rv = nsServiceManager::GetService(kStdURLParserCID, 
                                          NS_GET_IID(nsIURLParser), 
                                          getter_AddRefs(mDefaultURLParser));
        if (NS_FAILED(rv)) return rv;
    }

    *_retval = mDefaultURLParser;
    NS_ADDREF(*_retval);
    CacheURLParser(scheme, *_retval);
    return NS_OK;
}

static void
ExtractUrlPart_Helper(const char *src, PRUint32 pos, PRInt32 len,
                      PRUint32 *startPos, PRUint32 *endPos, char **urlPart)
{
    if (len < 0) {
        if (startPos)
            *startPos = 0;
        if (endPos)
            *endPos = 0;
        if (urlPart)
            *urlPart = nsnull;
    }
    else {
        if (startPos)
            *startPos = pos;
        if (endPos)
            *endPos = pos + len;
        if (urlPart)
            *urlPart = PL_strndup(src + pos, len);
    }
}

// Crap.  How do I ensure that startPos and endPos are correct.
NS_IMETHODIMP 
nsIOService::ExtractUrlPart(const char *urlString, PRInt16 flag, PRUint32 *startPos, PRUint32 *endPos, char **urlPart)
{
    nsresult rv;
    nsXPIDLCString scheme;

    if (urlPart)
        *urlPart = nsnull;

    ExtractScheme(urlString, startPos, endPos, getter_Copies(scheme));

    if (flag == url_Scheme) {
        if (urlPart)
            *urlPart = nsCRT::strdup(scheme);
        return NS_OK;
    }

    nsCOMPtr<nsIURLParser> parser;
    rv = GetParserForScheme(scheme, getter_AddRefs(parser));
    if (NS_FAILED(rv)) return rv;
    
    PRUint32 authPos, pathPos;
    PRInt32 authLen, pathLen;

    rv = parser->ParseURL(urlString, -1,
                          nsnull, nsnull,
                          &authPos, &authLen,
                          &pathPos, &pathLen);
    if (NS_FAILED(rv)) return rv;

    if (flag == url_Path) {
        ExtractUrlPart_Helper(urlString, pathPos, pathLen, startPos, endPos, urlPart);
    }
    else if (flag < url_Directory || flag & url_Port) {
        PRUint32 usernamePos, passwordPos, hostnamePos;
        PRInt32 usernameLen, passwordLen, hostnameLen;
        PRInt32 port;

        if (authLen < 0)
            return NS_OK;

        rv = parser->ParseAuthority(urlString + authPos, authLen,
                                    &usernamePos, &usernameLen,
                                    &passwordPos, &passwordLen,
                                    &hostnamePos, &hostnameLen,
                                    &port);
        if (NS_FAILED(rv)) return rv;

        usernamePos += authPos;
        passwordPos += authPos;
        hostnamePos += authPos;

        switch (flag) {
        case url_Username:
            ExtractUrlPart_Helper(urlString, usernamePos, usernameLen, startPos, endPos, urlPart);
            break;
        case url_Password:
            ExtractUrlPart_Helper(urlString, passwordPos, passwordLen, startPos, endPos, urlPart);
            break;
        case url_Host:
            ExtractUrlPart_Helper(urlString, hostnamePos, hostnameLen, startPos, endPos, urlPart);
            break;
        case url_Port:
            if (port != -1) {
                PRInt32 pos = hostnamePos + hostnameLen + 1;
                ExtractUrlPart_Helper(urlString, pos, pathPos - pos, startPos, endPos, urlPart);
            }
            else
                ExtractUrlPart_Helper(nsnull, 0, -1, startPos, endPos, urlPart);
            break;
        case url_Host | url_Port:
            if (port != -1 && hostnameLen > 0) {
                nsCAutoString buf(urlString + hostnamePos, hostnameLen);
                buf.Append(':');
                buf.AppendInt(port);
                ExtractUrlPart_Helper(buf.get(), 0, buf.Length(), startPos, endPos, urlPart);
            }
            else
                ExtractUrlPart_Helper(urlString, hostnamePos, hostnameLen, startPos, endPos, urlPart);
            break;
        default:
            NS_NOTREACHED("unexpected flag");
            return NS_ERROR_UNEXPECTED;
        }
    }
    else {
        PRUint32 filepathPos, paramPos, queryPos, refPos;
        PRInt32 filepathLen, paramLen, queryLen, refLen;

        if (pathLen < 0)
            return NS_OK;

        rv = parser->ParsePath(urlString + pathPos, pathLen,
                               &filepathPos, &filepathLen,
                               &paramPos, &paramLen,
                               &queryPos, &queryLen,
                               &refPos, &refLen);
        if (NS_FAILED(rv)) return rv;

        filepathPos += pathPos;
        paramPos += pathPos;
        queryPos += pathPos;
        refPos += pathPos;

        if (flag < url_Param) {
            PRUint32 directoryPos, basenamePos, extensionPos;
            PRInt32 directoryLen, basenameLen, extensionLen;

            rv = parser->ParseFilePath(urlString + filepathPos, filepathLen,
                                       &directoryPos, &directoryLen,
                                       &basenamePos, &basenameLen,
                                       &extensionPos, &extensionLen);

            directoryPos += filepathPos;
            basenamePos += filepathPos;
            extensionPos += filepathPos;

            switch (flag) {
            case url_Directory:
                ExtractUrlPart_Helper(urlString, directoryPos, directoryLen, startPos, endPos, urlPart);
                break;
            case url_FileBaseName:
                ExtractUrlPart_Helper(urlString, basenamePos, basenameLen, startPos, endPos, urlPart);
                break;
            case url_FileExtension:
                ExtractUrlPart_Helper(urlString, extensionPos, extensionLen, startPos, endPos, urlPart);
                break;
            default:
                NS_NOTREACHED("unexpected flag");
                return NS_ERROR_UNEXPECTED;
            }
        }
        else {
            switch (flag) {
            case url_Param:
                ExtractUrlPart_Helper(urlString, paramPos, paramLen, startPos, endPos, urlPart);
                break;
            case url_Query:
                ExtractUrlPart_Helper(urlString, queryPos, queryLen, startPos, endPos, urlPart);
                break;
            case url_Ref:
                ExtractUrlPart_Helper(urlString, refPos, refLen, startPos, endPos, urlPart);
                break;
            default:
                NS_NOTREACHED("unexpected flag");
                return NS_ERROR_UNEXPECTED;
            }
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsIOService::GetProtocolFlags(const char* scheme, PRUint32 *flags)
{
    nsCOMPtr<nsIProtocolHandler> handler;
    nsresult rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
    if (NS_FAILED(rv)) {
        return rv;
    }
    rv = handler->GetProtocolFlags(flags);
    return rv;
}


nsresult
nsIOService::NewURI(const char* aSpec, nsIURI* aBaseURI, nsIURI* *result)
{
    nsresult rv;
    nsIURI* base;
    NS_ENSURE_ARG_POINTER(aSpec);
    nsXPIDLCString scheme;
    nsCOMPtr<nsIProtocolHandler> handler;
    PRUint32 start,end;

    rv = ExtractScheme(aSpec, &start, &end, nsnull);
    if (NS_SUCCEEDED(rv))
    {
        // then aSpec is absolute
        // ignore aBaseURI in this case
        base = nsnull;
        rv = GetCachedProtocolHandler(aSpec, getter_AddRefs(handler),
                                      start,end);
        if (NS_FAILED(rv))
        {
            // not cached; we'll do an allocation
            rv = ExtractScheme(aSpec, nsnull, nsnull, getter_Copies(scheme));
        }
        // else scheme == nsnull, and we succeeded
    }
    else
    {
        // then aSpec is relative
        if (!aBaseURI)
            return NS_ERROR_MALFORMED_URI;
        rv = aBaseURI->GetScheme(getter_Copies(scheme));
        base = aBaseURI;
    }
    // scheme is non-null if we allocated it.
    // base is null if absolute, set if relative
    // rv is success if all is ok, failure if we need to cleanup and exit
    if (NS_SUCCEEDED(rv) && scheme.get())
    {
        // we allocated scheme; get the handler for it
        rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
    }
    if (NS_FAILED(rv)) return rv;

    return handler->NewURI(aSpec, base, result);
}


NS_IMETHODIMP 
nsIOService::NewFileURI(nsIFile *aSpec, nsIURI **_retval)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(aSpec);
    
    nsXPIDLCString urlString;
    rv = GetURLSpecFromFile(aSpec, getter_Copies(urlString));
    if (NS_FAILED(rv)) return rv;

    return NewURI(urlString, nsnull, _retval);
}

NS_IMETHODIMP
nsIOService::NewChannelFromURI(nsIURI *aURI, nsIChannel **result)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(aURI);
    NS_TIMELINE_MARK_URI("nsIOService::NewChannelFromURI(%s)", aURI);

    nsXPIDLCString scheme;
    rv = aURI->GetScheme(getter_Copies(scheme));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProxyInfo> pi;
    rv = mProxyService->ExamineForProxy(aURI, getter_AddRefs(pi));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProtocolHandler> handler;

    if (pi && !nsCRT::strcmp(pi->Type(),"http")) {
        // we are going to proxy this channel using an http proxy
        rv = GetProtocolHandler("http", getter_AddRefs(handler));
        if (NS_FAILED(rv)) return rv;
    } else {
        rv = GetProtocolHandler(scheme.get(), getter_AddRefs(handler));
        if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsIProxiedProtocolHandler> pph = do_QueryInterface(handler);

    if (pph)
        rv = pph->NewProxiedChannel(aURI, pi, result);
    else
        rv = handler->NewChannel(aURI, result);

    return rv;
}

NS_IMETHODIMP
nsIOService::NewChannel(const char *aSpec, nsIURI *aBaseURI, nsIChannel **result)
{
    nsresult rv;
    nsCOMPtr<nsIURI> uri;
    rv = NewURI(aSpec, aBaseURI, getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    return NewChannelFromURI(uri, result);
}

NS_IMETHODIMP
nsIOService::GetOffline(PRBool *offline)
{
    *offline = mOffline;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::SetOffline(PRBool offline)
{
    nsCOMPtr<nsIObserverService>
        observerService(do_GetService("@mozilla.org/observer-service;1"));
    
    nsresult rv1 = NS_OK;
    nsresult rv2 = NS_OK;
    if (offline) {
    	mOffline = PR_TRUE;		// indicate we're trying to shutdown
        // be sure to try and shutdown both (even if the first fails)
        if (mDNSService)
            rv1 = mDNSService->Shutdown();  // shutdown dns service first, because it has callbacks for socket transport
        if (mSocketTransportService)
            rv2 = mSocketTransportService->Shutdown();
        if (NS_FAILED(rv1)) return rv1;
        if (NS_FAILED(rv2)) return rv2;

        // don't care if notification fails
        if (observerService)
            (void)observerService->NotifyObservers(NS_STATIC_CAST(nsIIOService *, this),
                                                   "network:offline-status-changed",
                                                   NS_LITERAL_STRING("offline").get());
    }
    else if (!offline && mOffline) {
        // go online
        if (mDNSService)
            rv1 = mDNSService->Init();
        if (NS_FAILED(rv2)) return rv1;

        if (mSocketTransportService)
            rv2 = mSocketTransportService->Init();		//XXX should we shutdown the dns service?
        if (NS_FAILED(rv2)) return rv1;        
        mOffline = PR_FALSE;    // indicate success only AFTER we've
                                // brought up the services
        // don't care if notification fails
        if (observerService)
            (void)observerService->NotifyObservers(NS_STATIC_CAST(nsIIOService *, this),
                                                   "network:offline-status-changed",
                                                   NS_LITERAL_STRING("online").get());
    }
    return NS_OK;
}


NS_IMETHODIMP
nsIOService::AllowPort(PRInt32 inPort, const char *scheme, PRBool *_retval)
{
    PRInt16 port = inPort;
    if (port == -1) {
        *_retval = PR_TRUE;
        return NS_OK;
    }
        
    // first check to see if the port is in our blacklist:
    PRInt32 badPortListCnt = mRestrictedPortList.Count();
    for (int i=0; i<badPortListCnt; i++)
    {
        if (port == (PRInt32) NS_PTR_TO_INT32(mRestrictedPortList[i]))
        {
            *_retval = PR_FALSE;

            // check to see if the protocol wants to override
            if (!scheme)
                return NS_OK;
            
            nsCOMPtr<nsIProtocolHandler> handler;
            nsresult rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
            if (NS_FAILED(rv)) return rv;

            // let the protocol handler decide
            return handler->AllowPort(port, scheme, _retval);
        }
    }

    *_retval = PR_TRUE;
    return NS_OK;
}
////////////////////////////////////////////////////////////////////////////////
// URL parsing utilities

NS_IMETHODIMP
nsIOService::ExtractPort(const char *str, PRInt32 *result)
{
    *result = ExtractPortFrom(str);
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::ResolveRelativePath(const char *relativePath, const char* basePath,
                                 char **result)
{
    nsCAutoString name;
    nsCAutoString path(basePath);
	PRBool needsDelim = PR_FALSE;

	if ( !path.IsEmpty() ) {
		PRUnichar last = path.Last();
		needsDelim = !(last == '/' || last == '\\' );
	}

    PRBool end = PR_FALSE;
    char c;
    while (!end) {
        c = *relativePath++;
        switch (c) {
          case '\0':
          case '#':
          case ';':
          case '?':
            end = PR_TRUE;
            // fall through...
          case '/':
          case '\\':
            // delimiter found
            if (name.Equals("..")) {
                // pop path
                // If we already have the delim at end, then
                //  skip over that when searching for next one to the left
                PRInt32 offset = path.Length() - (needsDelim ? 1 : 2);
                PRInt32 pos = path.RFind("/", PR_FALSE, offset);
                if (pos > 0) {
                    path.Truncate(pos + 1);
                }
                else {
                    return NS_ERROR_MALFORMED_URI;
                }
            }
            else if (name.Equals(".") || name.Equals("")) {
                // do nothing
            }
            else {
                // append name to path
                if (needsDelim)
                    path += "/";
                path += name;
                needsDelim = PR_TRUE;
            }
            name = "";
            break;

          default:
            // append char to name
            name += c;
        }
    }
    // append anything left on relativePath (e.g. #..., ;..., ?...)
    if (c != '\0')
        path += --relativePath;

    *result = ToNewCString(path);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

void
nsIOService::PrefsChanged(nsIPrefBranch *prefs, const char *pref)
{
    if (!prefs) return;

    // Look for extra ports to block
    if (!pref || PL_strcmp(pref, PORT_PREF("banned")) == 0)
        ParsePortList(prefs, PORT_PREF("banned"), PR_FALSE);

    // ...as well as previous blocks to remove.
    if (!pref || PL_strcmp(pref, PORT_PREF("banned.override")) == 0)
        ParsePortList(prefs, PORT_PREF("banned.override"), PR_TRUE);
}

void
nsIOService::ParsePortList(nsIPrefBranch *prefBranch, const char *pref, PRBool remove)
{
    nsXPIDLCString portList;

    // Get a pref string and chop it up into a list of ports.
    prefBranch->GetCharPref(pref, getter_Copies(portList));
    if (portList) {
        char* tokp;
        char* currentPos = (char *)portList.get();
        while ((tokp = nsCRT::strtok(currentPos, ",", &currentPos))) {
            nsCAutoString tmp(tokp);
            tmp.StripWhitespace();

            PRInt32 aErrorCode;
            PRInt32 value = tmp.ToInteger(&aErrorCode);
            if (remove)
                mRestrictedPortList.RemoveElement((void*)value);
            else
                mRestrictedPortList.AppendElement((void*)value);
        }
    }
}

void
nsIOService::GetPrefBranch(nsIPrefBranch **result)
{
    *result = nsnull;
    nsCOMPtr<nsIPrefService> prefService =
        do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefService)
        prefService->GetBranch(nsnull, result);
}

// nsIObserver interface
NS_IMETHODIMP
nsIOService::Observe(nsISupports *subject,
                     const char *topic,
                     const PRUnichar *data)
{
    if (!nsCRT::strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(subject);
        if (prefBranch)
            PrefsChanged(prefBranch, NS_ConvertUCS2toUTF8(data).get());
    }
    else if (!nsCRT::strcmp(topic, kProfileChangeNetTeardownTopic)) {
        if (!mOffline) {
            SetOffline(PR_TRUE);
            mOfflineForProfileChange = PR_TRUE;
        }
    }
    else if (!nsCRT::strcmp(topic, kProfileChangeNetRestoreTopic)) {
        if (mOfflineForProfileChange) {
            SetOffline(PR_FALSE);
            mOfflineForProfileChange = PR_FALSE;
        }    
    }
    return NS_OK;
}

nsresult
nsIOService::ParseFileURL(const char* inURL,
                          char **outHost, char **outDirectory,
                          char **outFileBaseName, char **outFileExtension)
{
    nsresult rv = NS_ERROR_NOT_IMPLEMENTED;

    NS_ENSURE_ARG(inURL);
    NS_ENSURE_ARG_POINTER(outHost);
    *outHost = nsnull;
    NS_ENSURE_ARG_POINTER(outDirectory);
    *outDirectory = nsnull;
    NS_ENSURE_ARG_POINTER(outFileBaseName);
    *outFileBaseName = nsnull;
    NS_ENSURE_ARG_POINTER(outFileExtension);
    *outFileExtension = nsnull;

	nsXPIDLCString scheme;
    rv = ExtractScheme(inURL, nsnull, nsnull, getter_Copies(scheme));
    if (NS_FAILED(rv)) return rv;
    if (nsCRT::strcmp(scheme.get(), "file") != 0) {
        NS_ERROR("must be a file:// url");
        return NS_ERROR_UNEXPECTED;
    }
    
    nsCOMPtr<nsIURLParser> parser;
    rv = GetParserForScheme(scheme, getter_AddRefs(parser));
    if (NS_FAILED(rv)) return rv;

    PRUint32 pathPos, filepathPos, directoryPos, basenamePos, extensionPos;
    PRInt32 pathLen, filepathLen, directoryLen, basenameLen, extensionLen;

    // invoke the parser to extract the URL path
    rv = parser->ParseURL(inURL, -1,
                          nsnull, nsnull, // dont care about scheme
                          nsnull, nsnull, // dont care about authority
                          &pathPos, &pathLen);
    if (NS_FAILED(rv)) return rv;

    // invoke the parser to extract filepath from the path
    rv = parser->ParsePath(inURL + pathPos, pathLen,
                           &filepathPos, &filepathLen,
                           nsnull, nsnull,  // dont care about param
                           nsnull, nsnull,  // dont care about query
                           nsnull, nsnull); // dont care about ref
    if (NS_FAILED(rv)) return rv;

    filepathPos += pathPos;

    // invoke the parser to extract the directory and filename from filepath
    rv = parser->ParseFilePath(inURL + filepathPos, filepathLen,
                           &directoryPos, &directoryLen,
                           &basenamePos, &basenameLen,
                           &extensionPos, &extensionLen);
    if (NS_FAILED(rv)) return rv;

    if (directoryLen > 0)
        *outDirectory = PL_strndup(inURL + filepathPos + directoryPos, directoryLen);
    if (basenameLen > 0)
        *outFileBaseName = PL_strndup(inURL + filepathPos + basenamePos, basenameLen);
    if (extensionLen > 0)
        *outFileExtension = PL_strndup(inURL + filepathPos + extensionPos, extensionLen);
    // since we are using a no-auth url parser, there will never be a host

    return NS_OK;
}
 
