/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/* Implement shared vtbl methods for WrappedJS. */

#include "xpcprivate.h"

#if defined(LINUX)

static nsresult
PrepareAndDispatch(nsXPCWrappedJS* self, uint32 methodIndex, uint32* args)
{
#define PARAM_BUFFER_COUNT     32

    nsXPCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPCMiniVariant* dispatchParams = NULL;
    nsXPCWrappedJSClass* clazz;
    nsIInterfaceInfo* iface_info;
    const nsXPTMethodInfo* info;
    uint8 paramCount;
    uint8 i;
    nsresult result = NS_ERROR_FAILURE;

    NS_ASSERTION(self,"no self");

    clazz = self->GetClass();
    NS_ASSERTION(clazz,"no class");

    iface_info = clazz->GetInterfaceInfo();
    NS_ASSERTION(iface_info,"no interface info");

    iface_info->GetMethodInfo(uint16(methodIndex), &info);
    NS_ASSERTION(info,"no interface info");

    paramCount = info->GetParamCount();

    // setup variant array pointer
    if(paramCount > PARAM_BUFFER_COUNT)
        dispatchParams = new nsXPCMiniVariant[paramCount];
    else
        dispatchParams = paramBuffer;
    NS_ASSERTION(dispatchParams,"no place for params");

    uint32* ap = args;
    for(i = 0; i < paramCount; i++, ap++)
    {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPCMiniVariant* dp = &dispatchParams[i];

        if(param.IsOut() || !type.IsArithmetic())
        {
            dp->val.p = (void*) *ap;
            continue;
        }
        // else
        switch(type)
        {
        case nsXPTType::T_I8     : dp->val.i8  = *((int8*)   ap);       break;
        case nsXPTType::T_I16    : dp->val.i16 = *((int16*)  ap);       break;
        case nsXPTType::T_I32    : dp->val.i32 = *((int32*)  ap);       break;
        case nsXPTType::T_I64    : dp->val.i64 = *((int64*)  ap); ap++; break;
        case nsXPTType::T_U8     : dp->val.u8  = *((uint8*)  ap);       break;
        case nsXPTType::T_U16    : dp->val.u16 = *((uint16*) ap);       break;
        case nsXPTType::T_U32    : dp->val.u32 = *((uint32*) ap);       break;
        case nsXPTType::T_U64    : dp->val.u64 = *((uint64*) ap); ap++; break;
        case nsXPTType::T_FLOAT  : dp->val.f   = *((float*)  ap);       break;
        case nsXPTType::T_DOUBLE : dp->val.d   = *((double*) ap); ap++; break;
        case nsXPTType::T_BOOL   : dp->val.b   = *((PRBool*) ap);       break;
        case nsXPTType::T_CHAR   : dp->val.c   = *((char*)   ap);       break;
        case nsXPTType::T_WCHAR  : dp->val.wc  = *((wchar_t*)ap);       break;
        default:
            NS_ASSERTION(0, "bad type");
            break;
        }
    }
    result = clazz->CallMethod(self, (uint16)methodIndex, info, dispatchParams);

    if(dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}

#define STUB_ENTRY(n) \
nsresult nsXPCWrappedJS::Stub##n() \
{ \
  register void* method = PrepareAndDispatch; \
  register nsresult result; \
  __asm__ __volatile__( \
    "leal   0x0c(%%ebp), %%ecx\n\t"    /* args */ \
    "pushl  %%ecx\n\t" \
    "pushl  $"#n"\n\t"                 /* method index */ \
    "movl   0x08(%%ebp), %%ecx\n\t"    /* this */ \
    "pushl  %%ecx\n\t" \
    "call   *%%edx"                    /* PrepareAndDispatch */ \
    : "=a" (result)     /* %0 */ \
    : "d" (method)      /* %1 */ \
    : "ax", "dx", "cx", "memory" ); \
    return result; \
}

#define SENTINEL_ENTRY(n) \
nsresult nsXPCWrappedJS::Sentinel##n() \
{ \
    NS_ASSERTION(0,"nsXPCWrappedJS::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xpcstubsdef.inc"

#endif
