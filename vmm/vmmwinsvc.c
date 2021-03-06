// vmmwinsvc.c : implementation of functionality related to Windows service manager (SCM).
//
// (c) Ulf Frisk, 2020
// Author: Ulf Frisk, pcileech@frizk.net
//
#include "vmmwinsvc.h"
#include "vmm.h"
#include "vmmwin.h"
#include "pdb.h"
#include "util.h"

typedef struct tdVMMWINSVC_OFFSET_SC19 {
    WORD _Size;
    WORD Tag;
    BOOL fTag;
    DWORD TagV;
    WORD FLink;
    WORD BLink;
    WORD Ordinal;
    WORD SvcTp;
    WORD NmShort;
    WORD NmLong;
    WORD SvcStatus;
    WORD ExtInfo;
} VMMWINSVC_OFFSET_SC19, *PVMMWINSVC_OFFSET_SC19;

typedef struct tdVMMWINSVC_OFFSET_SC16 {
    WORD _Size;
    WORD Tag;
    BOOL fTag;
    WORD StartupPath;
    WORD Pid;
    WORD UserTp;
    WORD UserAcct;
    WORD FLink;
    WORD BLink;
} VMMWINSVC_OFFSET_SC16, *PVMMWINSVC_OFFSET_SC16;

typedef struct tdVMMWINSVC_CONTEXT {
    BOOL fSc19;
    DWORD dwTag;
    VMMWINSVC_OFFSET_SC19 oSc19;
    VMMWINSVC_OFFSET_SC16 oSc16;
} VMMWINSVC_CONTEXT, *PVMMWINSVC_CONTEXT;

/*
* Retrieve required offsets for parsing services by os build version.
* -- ctx
*/
VOID VmmWinSvc_OffsetLocator(_In_ PVMMWINSVC_CONTEXT ctx)
{
    BOOL f32 = ctxVmm->f32;
    DWORD dwBuild = ctxVmm->kernel.dwVersionBuild;
    VMMWINSVC_OFFSET_SC16 o16 = { 0 };
    VMMWINSVC_OFFSET_SC19 o19 = { 0 };
    if(f32) {
        if(dwBuild >= 15063) {          // WIN10 1703/15063 +
            ctx->fSc19 = TRUE;
            ctx->dwTag = 'Sc19';
            o16 = (VMMWINSVC_OFFSET_SC16){ ._Size = 0x40, .fTag = TRUE, .Tag = 0x00, .BLink = 0x04, .FLink = 0x08, .StartupPath = 0x0c, .UserTp = 0x10, .Pid = 0x14, .UserAcct = 0x34 };
            o19 = (VMMWINSVC_OFFSET_SC19){ ._Size = 0xd0, .fTag = TRUE, .TagV = 'Sc19', .Tag = 0x04, .BLink = 0x0c, .FLink = 0x10, .Ordinal = 0x14, .SvcTp = 0x18, .NmShort = 0x2c, .NmLong = 0x30, .SvcStatus = 0x34, .ExtInfo = 0x9c };
            if(dwBuild >= 17763) { o19.ExtInfo = 0xa0; }
            if(dwBuild >= 18362) { o19.ExtInfo = 0xa4; }
            if(dwBuild >= 19041) { o19.ExtInfo = 0xc0; }
        } else if(dwBuild >= 9200) {    // WIN 8.0 +
            ctx->fSc19 = TRUE;
            ctx->dwTag = 'sErv';
            o16 = (VMMWINSVC_OFFSET_SC16){ ._Size = 0x20, .fTag = TRUE, .Tag = 0x00, .BLink = 0x04, .FLink = 0x08, .StartupPath = 0x0c, .UserTp = 0x00, .Pid = 0x10, .UserAcct = 0x00 };
            o19 = (VMMWINSVC_OFFSET_SC19){ ._Size = 0x80, .fTag = TRUE, .TagV = 'sErv', .Tag = 0x00, .BLink = 0x04, .FLink = 0x68, .Ordinal = 0x10, .SvcTp = 0x44, .NmShort = 0x08, .NmLong = 0x0c, .SvcStatus = 0x28, .ExtInfo = 0x24 };
            if(dwBuild >= 9600) { o19.FLink = 0x6c; }
        } else if(dwBuild >= 6000) {    // VISTA, WIN7
            ctx->dwTag = 'serH';
            o16 = (VMMWINSVC_OFFSET_SC16){ ._Size = 0x20, .fTag = FALSE, .Tag = 0x00, .BLink = 0x00, .FLink = 0x04, .StartupPath = 0x08, .UserTp = 0x00, .Pid = 0x0c, .UserAcct = 0x00 };
            o19 = (VMMWINSVC_OFFSET_SC19){ ._Size = 0x80, .fTag = FALSE, .TagV = '----', .Tag = 0x00, .BLink = 0x00, .FLink = 0x60, .Ordinal = 0x0c, .SvcTp = 0x3c, .NmShort = 0x04, .NmLong = 0x08, .SvcStatus = 0x20, .ExtInfo = 0x1c };
        } else if(dwBuild >= 2600) {    // XP
            ctx->fSc19 = TRUE;
            ctx->dwTag = 'sErv';
            o16 = (VMMWINSVC_OFFSET_SC16){ ._Size = 0x20, .fTag = FALSE, .Tag = 0x00, .BLink = 0x00, .FLink = 0x04, .StartupPath = 0x08, .UserTp = 0x00, .Pid = 0x0c, .UserAcct = 0x00 };
            o19 = (VMMWINSVC_OFFSET_SC19){ ._Size = 0x80, .fTag = TRUE, .TagV = 'sErv', .Tag = 0x18, .BLink = 0x00, .FLink = 0x04, .Ordinal = 0x10, .SvcTp = 0x44, .NmShort = 0x08, .NmLong = 0x0c, .SvcStatus = 0x28, .ExtInfo = 0x24 };
        }
    } else {
        if(dwBuild >= 15063) {          // WIN10 1703/15063 +
            ctx->fSc19 = TRUE;
            ctx->dwTag = 'Sc19';
            o16 = (VMMWINSVC_OFFSET_SC16){ ._Size = 0x60, .fTag = TRUE, .Tag = 0x00, .BLink = 0x08, .FLink = 0x10, .StartupPath = 0x18, .UserTp = 0x20, .Pid = 0x28, .UserAcct = 0x58 };
            o19 = (VMMWINSVC_OFFSET_SC19){ ._Size = 0x130, .fTag = TRUE, .TagV = 'Sc19', .Tag = 0x08, .BLink = 0x10, .FLink = 0x18, .Ordinal = 0x20, .SvcTp = 0x24, .NmShort = 0x38, .NmLong = 0x40, .SvcStatus = 0x48, .ExtInfo = 0xe8 };
            if(dwBuild >= 18362) { o19.ExtInfo = 0xf0; }
            if(dwBuild >= 19041) { o19.ExtInfo = 0x128; }
        } else if(dwBuild >= 9200) {    // WIN 8.0 +
            ctx->fSc19 = TRUE;
            ctx->dwTag = 'sErv';
            o16 = (VMMWINSVC_OFFSET_SC16){ ._Size = 0x30, .fTag = TRUE, .Tag = 0x00, .BLink = 0x08, .FLink = 0x10, .StartupPath = 0x18, .UserTp = 0x00, .Pid = 0x28, .UserAcct = 0x00 };
            o19 = (VMMWINSVC_OFFSET_SC19){ ._Size = 0xa0, .fTag = TRUE, .TagV = 'sErv', .Tag = 0x00, .BLink = 0x08, .FLink = 0x90, .Ordinal = 0x20, .SvcTp = 0x5c, .NmShort = 0x10, .NmLong = 0x18, .SvcStatus = 0x40, .ExtInfo = 0x38 };
            if(dwBuild >= 9600) { o19.FLink = 0x98; }
        } else if(dwBuild >= 6000) {    // VISTA, WIN7
            ctx->dwTag = 'serH';
            o16 = (VMMWINSVC_OFFSET_SC16){ ._Size = 0x20, .fTag = FALSE, .Tag = 0x00, .BLink = 0x00, .FLink = 0x08, .StartupPath = 0x10, .UserTp = 0x00, .Pid = 0x18, .UserAcct = 0x00 };
            o19 = (VMMWINSVC_OFFSET_SC19){ ._Size = 0xa0, .fTag = FALSE, .TagV = '----', .Tag = 0x00, .BLink = 0x00, .FLink = 0x80, .Ordinal = 0x18, .SvcTp = 0x4c, .NmShort = 0x08, .NmLong = 0x10, .SvcStatus = 0x30, .ExtInfo = 0x28 };
        }
    }
    memcpy(&ctx->oSc16, &o16, sizeof(VMMWINSVC_OFFSET_SC16));
    memcpy(&ctx->oSc19, &o19, sizeof(VMMWINSVC_OFFSET_SC19));
}

/*
* Retrieve the services.exe process.
* CALLER DECREF: return
* -- return
*/
PVMM_PROCESS VmmWinSvc_GetProcessServices()
{
    BOOL f;
    LPSTR szPProc;
    PVMM_PROCESS pObProcess = NULL, pObProcessParent = NULL;
    while((pObProcess = VmmProcessGetNext(pObProcess, 0))) {
        if(!_stricmp("services.exe", pObProcess->szName)) {
            szPProc = (ctxVmm->kernel.dwVersionBuild == 2600) ? "winlogon.exe" : "wininit.exe";
            f = (pObProcessParent = VmmProcessGet(pObProcess->dwPPID)) &&
                !_stricmp(szPProc, pObProcessParent->szName);
            Ob_DECREF_NULL(&pObProcessParent);
            if(f) { return pObProcess; }
        }
    }
    return NULL;
}

/*
* Retrieve services list start/end from services.exe!ServiceDatabase PDB symbol.
* -- pProcessServices
* -- pvaListHead
*/
VOID VmmWinSvc_ListHeadFromPDB(_In_ PVMM_PROCESS pSvcProcess, _Out_writes_(2) PQWORD pvaListHead)
{
    PDB_HANDLE hPdbServices;
    PVMM_MAP_MODULEENTRY peModuleServices;
    PVMMOB_MAP_MODULE pObModuleMap = NULL;
    BYTE pbSymSvcDB[0x18] = { 0 };
    DWORD dwoSymSvcDB = 0;
    pvaListHead[0] = 0;
    pvaListHead[1] = 0;
    if(!VmmMap_GetModule(pSvcProcess, &pObModuleMap)) { goto fail; }
    if(!(peModuleServices = VmmMap_GetModuleEntry(pObModuleMap, L"services.exe"))) { goto fail; }
    if(!(hPdbServices = PDB_GetHandleFromModuleAddress(pSvcProcess, peModuleServices->vaBase))) { goto fail; }
    if(!PDB_GetSymbolOffset(hPdbServices, "ServiceDatabase", &dwoSymSvcDB)) { goto fail; }
    if(!VmmRead(pSvcProcess, peModuleServices->vaBase + dwoSymSvcDB, pbSymSvcDB, sizeof(pbSymSvcDB))) { goto fail; }
    pvaListHead[0] = (QWORD)VMM_PTR_OFFSET_DUAL(ctxVmm->f32, pbSymSvcDB, 4, 8);
    pvaListHead[1] = (QWORD)VMM_PTR_OFFSET_DUAL(ctxVmm->f32, pbSymSvcDB, 8, 16);
    if(!VMM_UADDR_4_8(pvaListHead[0]) || !VMM_UADDR_4_8(pvaListHead[1])) { goto fail; }
fail:
    Ob_DECREF(pObModuleMap);
}

/*
* Retrieve services objects from vad and prefetch vad into cache.
* -- ctx
* -- pSvcProcess
* -- pvaListHead
*/
VOID VmmWinSvc_ListHeadFromVAD(_In_ PVMMWINSVC_CONTEXT ctx, _In_ PVMM_PROCESS pSvcProcess, _Inout_updates_(2) PQWORD pvaListHead)
{
    BOOL f32 = ctxVmm->f32;
    QWORD va1, va2;
    DWORD i, o, dwTag, cbVad;
    BYTE pb2[0x10], *pb = NULL;
    PVMM_MAP_VADENTRY pe;
    PVMMOB_MAP_VAD pObVadMap = NULL;
    if(!VmmMap_GetVad(pSvcProcess, &pObVadMap, FALSE)) { goto finish; }
    // 1: if address exist -> prefetch vad and finish
    if(pvaListHead[0] && (pe = VmmMap_GetVadEntry(pObVadMap, pvaListHead[0]))) {
        cbVad = (DWORD)(pe->vaEnd + 1 - pe->vaStart);
        if(cbVad <= 0x00200000) {
            VmmCachePrefetchPages4(pSvcProcess, 1, &pe->vaStart, cbVad, 0);
        }
        goto finish;
    }
    // 2: locate vad candidates and scan:
    if(!(pb = LocalAlloc(0, 0x00200000))) { goto finish; }
    for(i = 0; i < pObVadMap->cMap; i++) {
        pe = pObVadMap->pMap + i;
        if(!pe->fPrivateMemory || (pe->CommitCharge < 0x10)) { continue; }
        cbVad = (DWORD)(pe->vaEnd + 1 - pe->vaStart);
        if((cbVad > 0x00200000) || (cbVad < 0x00010000)) { continue; }
        VmmReadEx(pSvcProcess, pe->vaStart, pb, cbVad, NULL, VMM_FLAG_ZEROPAD_ON_FAIL);
        dwTag = _byteswap_ulong(ctx->dwTag);
        for(o = 0x1000; o < 0x00200000; o += 4) {
            if(dwTag == *(PDWORD)(pb + o)) {
                if(ctx->fSc19) {    // sErv / Sc19 entry - Win8.0+
                    va1 = VMM_PTR_OFFSET(f32, pb + o - ctx->oSc19.Tag, ctx->oSc19.BLink);
                    va2 = VMM_PTR_OFFSET(f32, pb + o - ctx->oSc19.Tag, ctx->oSc19.FLink);
                } else {            // serH entry
                    va1 = VMM_PTR_OFFSET_DUAL(f32, pb + o, 0x0c, 0x10); // TODO: verify 32-bit version
                    if(!VMM_UADDR_4_8(va1)) { continue; }
                    if(!VmmRead(pSvcProcess, va1, pb2, 0x10)) { continue; }
                    va1 = VMM_PTR_OFFSET(f32, pb2, 0);
                    va2 = VMM_PTR_OFFSET_DUAL(f32, pb2, 4, 8);
                }
                if((va1 < 0x10000) || !VMM_UADDR_4_8(va1) || (va2 < 0x10000) || !VMM_UADDR_4_8(va2)) { continue; }
                pvaListHead[0] = va1;
                pvaListHead[1] = va2;
                goto finish;
            }
        }
    }
finish:
    Ob_DECREF(pObVadMap);
    LocalFree(pb);
}

/*
* Retrieve services from the service database list structure.
* CALLER DECREF: return
* -- ctx
* -- pProcessSvc
* -- cVaListHead
* -- pvaListHead
* -- return
*/
POB_MAP VmmWinSvc_MainListWalk(_In_ PVMMWINSVC_CONTEXT ctx, _In_ PVMM_PROCESS pProcessSvc, _In_ DWORD cVaListHead, _In_reads_(cVaListHead) PQWORD pvaListHead)
{
    BOOL f32 = ctxVmm->f32;
    DWORD dwOrdinal, dwStartType;
    QWORD i, va, va1, va2, va3;
    BYTE pb[0x200] = { 0 };
    POB_SET psA = NULL;
    POB_MAP pmSvc = NULL;
    PVMM_MAP_SERVICEENTRY pe;
    PVMMWINSVC_OFFSET_SC19 o = &ctx->oSc19;
    if(!(psA = ObSet_New())) { goto fail; }
    if(!(pmSvc = ObMap_New(OB_MAP_FLAGS_OBJECT_LOCALFREE))) { goto fail; }
    for(i = 0; i < cVaListHead; i++) {
        ObSet_Push(psA, pvaListHead[i]);
    }
    while((va = ObSet_Pop(psA))) {
        // read & sanity check
        if(ObMap_ExistsKey(pmSvc, va)) { continue; }
        if(!VmmRead(pProcessSvc, va, pb, o->_Size)) { continue; }
        if(o->fTag && !VMM_POOLTAG(*(PDWORD)(pb + o->Tag), o->TagV)) { continue; }
        if((dwOrdinal = *(PDWORD)(pb + o->Ordinal)) > 0xffff) { continue; }
        if((dwStartType = *(PDWORD)(pb + o->SvcTp)) > SERVICE_TYPE_ALL) { continue; }
        // BLink / FLink
        va1 = VMM_PTR_OFFSET(f32, pb, o->BLink);
        va2 = VMM_PTR_OFFSET(f32, pb, o->FLink);
        if(!VMM_UADDR_4_8(va1)) { va1 = 0; }
        if(!VMM_UADDR_4_8(va2)) { va2 = 0; }
        if(!va1 && !va2) { continue; }
        ObSet_Push(psA, va1);
        ObSet_Push(psA, va2);
        if(!VMM_UADDR(VMM_PTR_OFFSET(f32, pb, o->NmShort))) { continue; }
        // allocate & assign
        if(!(pe = LocalAlloc(LMEM_ZEROINIT, sizeof(VMM_MAP_SERVICEENTRY)))) { continue; }
        ObMap_Push(pmSvc, va, pe);
        pe->vaObj = va;
        pe->dwOrdinal = dwOrdinal;
        pe->dwStartType = dwStartType;
        memcpy(&pe->ServiceStatus, pb + o->SvcStatus, sizeof(SERVICE_STATUS));
        pe->wszServiceName = (LPWSTR)VMM_PTR_OFFSET(f32, pb, o->NmShort);
        pe->wszDisplayName = (LPWSTR)VMM_PTR_OFFSET(f32, pb, o->NmLong);
        if((pe->ServiceStatus.dwServiceType & SERVICE_KERNEL_DRIVER) || (pe->ServiceStatus.dwServiceType & SERVICE_FILE_SYSTEM_DRIVER)) {
            pe->wszPath = (LPWSTR)VMM_PTR_OFFSET(f32, pb, o->ExtInfo);
        } else {
            va3 = VMM_PTR_OFFSET(f32, pb, o->ExtInfo);
            pe->_Reserved = VMM_UADDR_4_8(va3) ? va3 : 0;
        }
    }
    Ob_INCREF(pmSvc);
fail:
    Ob_DECREF(psA);
    return Ob_DECREF(pmSvc);
}

/*
* Retrieve the extended service info such as service process id, service user
* and other data which is found in the 'Sc16' data structure.
* -- ctx
* -- pProcessSvc
* -- pmSvc
*/
VOID VmmWinSvc_GetExtendedInfo(_In_ PVMMWINSVC_CONTEXT ctx, _In_ PVMM_PROCESS pProcessSvc, _In_ POB_MAP pmSvc)
{
    BOOL f32 = ctxVmm->f32;
    QWORD va;
    BYTE pb[0x80] = { 0 };
    POB_SET psObPrefetch = NULL;
    PVMM_MAP_SERVICEENTRY pe = NULL;
    PVMMWINSVC_OFFSET_SC16 o = &ctx->oSc16;
    if((psObPrefetch = ObSet_New())) {
        while((pe = ObMap_GetNext(pmSvc, pe))) {
            ObSet_Push(psObPrefetch, pe->_Reserved);
        }
        VmmCachePrefetchPages3(pProcessSvc, psObPrefetch, o->_Size, 0);
        Ob_DECREF_NULL(&psObPrefetch);
    }
    while((pe = ObMap_GetNext(pmSvc, pe))) {
        if((va = pe->_Reserved)) {
            pe->_Reserved = 0;
            if(!VmmRead2(pProcessSvc, va, pb, o->_Size, VMM_FLAG_FORCECACHE_READ)) { continue; }
            if(o->fTag) {
                if(!VMM_POOLTAG(*(PDWORD)(pb + o->Tag), 'Sc16')) { continue; }
            } else {
                if(!(va = VMM_PTR_OFFSET(f32, pb, o->BLink)) || !VMM_UADDR_4_8(va)) { continue; }
                if(!(va = VMM_PTR_OFFSET(f32, pb, o->FLink)) || !VMM_UADDR_4_8(va)) { continue; }
            }
            pe->dwPID = *(PDWORD)(pb + o->Pid);
            pe->wszPath = (LPWSTR)VMM_PTR_OFFSET(f32, pb, o->StartupPath);
            pe->wszUserTp = o->UserTp ? (LPWSTR)VMM_PTR_OFFSET(f32, pb, o->UserTp) : NULL;
            pe->wszUserAcct = o->UserAcct ? (LPWSTR)VMM_PTR_OFFSET(f32, pb, o->UserAcct) : NULL;
        }
    }
}

#define VMMWINSVC_MULTITEXT_MAX         0x00800000

/*
* Add a string uniquely to the wszMultiText, non-unique strings will be merged.
* -- pProcessSvc
* -- wszMultiText
* -- powszMultiText = current wszMultiText offset
* -- pmStr = map of existing strings
* -- wsz
* -- return
*/
LPWSTR VmmWinSvc_ResolveStrAddSingle(_In_ PVMM_PROCESS pProcessSvc, _In_ LPWSTR wszMultiText, _Inout_ PDWORD powszMultiText, _In_ POB_MAP pmStr, _In_ QWORD qwA, _In_opt_ LPWSTR wszUser)
{
    LPWSTR wszResult;
    QWORD cwsz, qwHash;
    WCHAR wsz[2048] = { 0 };
    if(wszUser) {
        wcsncpy_s(wsz, _countof(wsz), wszUser, _TRUNCATE);
    } else {
        if((qwA < 0x10000) || !VMM_UADDR(qwA)) { goto fail; }
        VmmRead2(pProcessSvc, qwA, (PBYTE)wsz, sizeof(wsz) - 2, VMM_FLAG_FORCECACHE_READ);
    }
    qwHash = Util_HashStringUpperW(wsz);
    if((wszResult = ObMap_GetByKey(pmStr, qwHash))) { return wszResult; }
    if(*powszMultiText > VMMWINSVC_MULTITEXT_MAX - _countof(wsz)) { goto fail; }
    cwsz = wcslen(wsz) + 1;
    if(cwsz == 1) { goto fail; }
    wszResult = wszMultiText + *powszMultiText;
    wcsncpy_s(wszResult, cwsz, wsz, _TRUNCATE);
    ObMap_Push(pmStr, qwHash, wszResult);
    *powszMultiText = (DWORD)(*powszMultiText + cwsz);
    return wszResult;
fail:
    return wszMultiText;
}

/*
* Resolve all "primordial" strings related to the services.
* -- pProcessSvc
* -- pmSvc
* -- pwszMultiText
* -- pcbMultiText
* -- return
*/
_Success_(return)
BOOL VmmWinSvc_ResolveStrAll(_In_ PVMM_PROCESS pProcessSvc, _In_ POB_MAP pmSvc, _Out_ LPWSTR *pwszMultiText, _Out_ PDWORD pcbMultiText)
{
    BOOL fProcessUser;
    WCHAR wszProcessUser[2048];
    PVMM_PROCESS pObProcessUser = NULL;
    POB_SET psObPrefetch = NULL;
    POB_MAP pmObStr = NULL;
    DWORD owszMultiText = 1;
    LPWSTR wszMultiText = NULL;
    PVMM_MAP_SERVICEENTRY pe = NULL;
    // 1: allocate & set-up
    if(!(pmObStr = ObMap_New(0))) { return FALSE; }
    if(!(wszMultiText = LocalAlloc(0, VMMWINSVC_MULTITEXT_MAX * sizeof(WCHAR)))) {
        Ob_DECREF(pmObStr);
        return FALSE;
    }
    // 2: prefetch for performance reasons
    if((psObPrefetch = ObSet_New())) {
        while((pe = ObMap_GetNext(pmSvc, pe))) {
            ObSet_Push(psObPrefetch, (QWORD)pe->wszServiceName);
            ObSet_Push(psObPrefetch, (QWORD)pe->wszDisplayName);
            ObSet_Push(psObPrefetch, (QWORD)pe->wszPath);
            ObSet_Push(psObPrefetch, (QWORD)pe->wszUserTp);
            ObSet_Push(psObPrefetch, (QWORD)pe->wszUserAcct);
        }
        VmmCachePrefetchPages3(pProcessSvc, psObPrefetch, 2 * 2048, 0);
        Ob_DECREF_NULL(&psObPrefetch);
    }
    // 3: fetch strings
    wszMultiText[0] = 0;
    while((pe = ObMap_GetNext(pmSvc, pe))) {
        pe->wszServiceName = VmmWinSvc_ResolveStrAddSingle(pProcessSvc, wszMultiText, &owszMultiText, pmObStr, (QWORD)pe->wszServiceName, NULL);
        pe->wszDisplayName = VmmWinSvc_ResolveStrAddSingle(pProcessSvc, wszMultiText, &owszMultiText, pmObStr, (QWORD)pe->wszDisplayName, NULL);
        pe->wszPath =        VmmWinSvc_ResolveStrAddSingle(pProcessSvc, wszMultiText, &owszMultiText, pmObStr, (QWORD)pe->wszPath, NULL);
        pe->wszUserTp =      VmmWinSvc_ResolveStrAddSingle(pProcessSvc, wszMultiText, &owszMultiText, pmObStr, (QWORD)pe->wszUserTp, NULL);
        pe->wszUserAcct =    VmmWinSvc_ResolveStrAddSingle(pProcessSvc, wszMultiText, &owszMultiText, pmObStr, (QWORD)pe->wszUserAcct, NULL);
        // username from process pid
        fProcessUser =
            !pe->wszUserAcct[0] && pe->dwPID &&
            (pObProcessUser = VmmProcessGetEx(NULL, pe->dwPID, VMM_FLAG_PROCESS_TOKEN)) &&
            pObProcessUser->win.TOKEN.fInitialized && pObProcessUser->win.TOKEN.fSID &&
            VmmWinUser_GetNameW(&pObProcessUser->win.TOKEN.SID, wszProcessUser, MAX_PATH, NULL, NULL);
        if(fProcessUser) {
            pe->wszUserAcct = VmmWinSvc_ResolveStrAddSingle(pProcessSvc, wszMultiText, &owszMultiText, pmObStr, 0, wszProcessUser);
        }
        Ob_DECREF_NULL(&pObProcessUser);

    }
    // 4: return
    *pwszMultiText = LocalReAlloc(wszMultiText, owszMultiText * sizeof(WCHAR), LMEM_FIXED);
    *pcbMultiText = owszMultiText * sizeof(WCHAR);
    Ob_DECREF(pmObStr);
    return TRUE;
}

/*
* Object manager callback function for object cleanup tasks.
* -- pVmmServiceMap
*/
VOID VmmWinSvc_CloseObCallback(_In_ PVOID pVmmServiceMap)
{
    PVMMOB_MAP_SERVICE pOb = (PVMMOB_MAP_SERVICE)pVmmServiceMap;
    LocalFree(pOb->wszMultiText);
}

/*
* qsort compare function for sorting the services list by ordinal.
*/
int VmmWinSvc_CmpSort(PVMM_MAP_SERVICEENTRY a, PVMM_MAP_SERVICEENTRY b)
{
    if(a->dwOrdinal != b->dwOrdinal) {
        return a->dwOrdinal - b->dwOrdinal;
    }
    return (int)(a->vaObj - b->vaObj);
}

/*
* Retrieve services list as a map object manager object.
* CALLER DECREF: return
* -- return
*/
PVMMOB_MAP_SERVICE VmmWinSvc_Initialize_DoWork()
{
    VMMWINSVC_CONTEXT InitCtx = { 0 };
    PVMM_PROCESS pObSvcProcess = NULL;
    QWORD vaSvcDatabase[2] = { 0 };
    POB_MAP pmObSvc = NULL;
    LPWSTR wszMultiText = NULL;
    DWORD i, cSvc, cbMultiText = 0;
    PVMMOB_MAP_SERVICE pObServiceMap = NULL;
    PVMM_MAP_SERVICEENTRY pe = NULL;
    // 1: initialize
    if(!(pObSvcProcess = VmmWinSvc_GetProcessServices())) { goto fail; }
    VmmWinSvc_OffsetLocator(&InitCtx);
    if(ctxVmm->kernel.dwVersionBuild >= 15063) {
        VmmWinSvc_ListHeadFromPDB(pObSvcProcess, vaSvcDatabase);
    }
    VmmWinSvc_ListHeadFromVAD(&InitCtx, pObSvcProcess, vaSvcDatabase);
    if(!vaSvcDatabase[0] && !vaSvcDatabase[1]) { goto fail; }
    // 2: walk services list and resolve extended info and strings
    if(!(pmObSvc = VmmWinSvc_MainListWalk(&InitCtx, pObSvcProcess, 2, vaSvcDatabase))) { goto fail; }
    VmmWinSvc_GetExtendedInfo(&InitCtx, pObSvcProcess, pmObSvc);
    if(!VmmWinSvc_ResolveStrAll(pObSvcProcess, pmObSvc, &wszMultiText, &cbMultiText)) { goto fail; }
    // 3: allocate, assign and sort services map
    cSvc = ObMap_Size(pmObSvc);
    if(!(wszMultiText = LocalReAlloc(wszMultiText, cbMultiText, LMEM_FIXED))) { goto fail; }
    pObServiceMap = Ob_Alloc(OB_TAG_MAP_SERVICE, 0, sizeof(VMMOB_MAP_SERVICE) + cSvc * sizeof(VMM_MAP_SERVICEENTRY), VmmWinSvc_CloseObCallback, NULL);
    if(!pObServiceMap) { goto fail; }
    pObServiceMap->wszMultiText = wszMultiText;
    pObServiceMap->cbMultiText = cbMultiText;
    pObServiceMap->cMap = cSvc;
    for(i = 0; i < cSvc; i++) {
        if((pe = ObMap_GetByIndex(pmObSvc, i))) {
            memcpy(pObServiceMap->pMap + i, pe, sizeof(VMM_MAP_SERVICEENTRY));
        }
    }
    qsort(pObServiceMap->pMap, pObServiceMap->cMap, sizeof(VMM_MAP_SERVICEENTRY), (int(*)(void const*, void const*))VmmWinSvc_CmpSort);
fail:
    Ob_DECREF(pObSvcProcess);
    Ob_DECREF(pmObSvc);
    return pObServiceMap;
}

/*
* Create a service map and assign to the global context upon success.
* CALLER DECREF: return
* -- return
*/
PVMMOB_MAP_SERVICE VmmWinSvc_Initialize()
{
    PVMMOB_MAP_SERVICE pObSvc;
    if((pObSvc = ObContainer_GetOb(ctxVmm->pObCMapService))) { return pObSvc; }
    EnterCriticalSection(&ctxVmm->LockUpdateMap);
    if((pObSvc = ObContainer_GetOb(ctxVmm->pObCMapService))) {
        LeaveCriticalSection(&ctxVmm->LockUpdateMap);
        return pObSvc;
    }
    pObSvc = VmmWinSvc_Initialize_DoWork();
    if(!pObSvc) {
        pObSvc = Ob_Alloc(OB_TAG_MAP_SERVICE, LMEM_ZEROINIT, sizeof(VMMOB_MAP_SERVICE), NULL, NULL);
        if(pObSvc) {
            pObSvc->wszMultiText = (LPWSTR)&pObSvc->cbMultiText;   // NULL CHAR guaranteed.
        }
    }
    ObContainer_SetOb(ctxVmm->pObCMapService, pObSvc);
    LeaveCriticalSection(&ctxVmm->LockUpdateMap);
    return pObSvc;
}

/*
* Refresh the service map.
*/
VOID VmmWinSvc_Refresh()
{
    ObContainer_SetOb(ctxVmm->pObCMapService, NULL);
}
