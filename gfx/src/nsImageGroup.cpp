/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsIImageGroup.h"
#include "nsIImageManager.h"
#include "nsIImageObserver.h"
#include "nsIImageRequest.h"
#include "nsImageRequest.h"
#include "ilIImageRenderer.h"
#include "nsImageNet.h"
#include "nsVoidArray.h"
#include "nsCRT.h"
#include "libimg.h"
#include "il_util.h"
#include "nsIDeviceContext.h"

static NS_DEFINE_IID(kIImageGroupIID, NS_IIMAGEGROUP_IID);

static void ns_observer_proc (XP_Observable aSource,
                              XP_ObservableMsg	aMsg, 
                              void* aMsgData, 
                              void* aClosure);

class ImageGroupImpl : public nsIImageGroup
{
public:
  ImageGroupImpl(nsIImageManager *aManager);
  ~ImageGroupImpl();

  nsresult Init(nsIDeviceContext *aDeviceContext);

  void* operator new(size_t sz) {
    void* rv = new char[sz];
    nsCRT::zero(rv, sz);
    return rv;
  }

  NS_DECL_ISUPPORTS

  virtual PRBool AddObserver(nsIImageGroupObserver *aObserver);
  virtual PRBool RemoveObserver(nsIImageGroupObserver *aObserver);

  virtual nsIImageRequest* GetImage(const char* aUrl, 
                                    nsIImageRequestObserver *aObserver,
                                    const nscolor* aBackgroundColor,
                                    PRUint32 aWidth, PRUint32 aHeight,
                                    PRUint32 aFlags);
  
  virtual void Interrupt(void);

  IL_GroupContext *GetGroupContext() { return mGroupContext; }
  nsVoidArray *GetObservers() { return mObservers; }

private:
  nsIImageManager *mManager;
  IL_GroupContext *mGroupContext;
  nsVoidArray *mObservers;
  nsIDeviceContext *mDeviceContext;
  IL_ColorSpace *mColorSpace;
};

ImageGroupImpl::ImageGroupImpl(nsIImageManager *aManager)
{
  NS_INIT_REFCNT();
  mManager = aManager;
  NS_ADDREF(mManager);
}
 
ImageGroupImpl::~ImageGroupImpl()
{
  NS_IF_RELEASE(mDeviceContext);

  if (mObservers != nsnull) {
    PRInt32 i, count = mObservers->Count();
    nsIImageGroupObserver *observer;
    for (i = 0; i < count; i++) {
      observer = (nsIImageGroupObserver *)mObservers->ElementAt(i);
      if (observer != nsnull) {
        NS_RELEASE(observer);
      }
    }

    delete mObservers;
  }

  if (mGroupContext != nsnull) {
    IL_DestroyGroupContext(mGroupContext);
  }
  
  if (mColorSpace != nsnull) {
    IL_ReleaseColorSpace(mColorSpace);
  }

  if (mManager != nsnull) {
    NS_RELEASE(mManager);
  }
}

NS_IMPL_ISUPPORTS(ImageGroupImpl, kIImageGroupIID)

nsresult 
ImageGroupImpl::Init(nsIDeviceContext *aDeviceContext)
{
  ilIImageRenderer *renderer;
  nsresult result;

  if ((result = NS_NewImageRenderer(&renderer)) != NS_OK) {
      return result;
  }

  mGroupContext = IL_NewGroupContext((void *)aDeviceContext,
                                     renderer);

  if (mGroupContext == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mDeviceContext = aDeviceContext;
  NS_ADDREF(mDeviceContext);

  // Ask the device context to create a color space
  mDeviceContext->CreateILColorSpace(mColorSpace);

  // Set the image group context display mode
  IL_DisplayData displayData;
	displayData.dither_mode = IL_Auto;
  displayData.color_space = mColorSpace;
  displayData.progressive_display = PR_TRUE;
  IL_SetDisplayMode(mGroupContext, 
                    IL_COLOR_SPACE | IL_PROGRESSIVE_DISPLAY | IL_DITHER_MODE,
                    &displayData);

  return NS_OK;
}

PRBool 
ImageGroupImpl::AddObserver(nsIImageGroupObserver *aObserver)
{
  if (aObserver == nsnull) {
    return PR_FALSE;
  }

  if (mObservers == nsnull) {
    mObservers = new nsVoidArray();
    if (mObservers == nsnull) {
      return PR_FALSE;
    }
    IL_AddGroupObserver(mGroupContext, ns_observer_proc, (void *)this);
  }
  
  NS_ADDREF(aObserver);
  mObservers->AppendElement((void *)aObserver);
  
  return PR_TRUE;
}
 
PRBool 
ImageGroupImpl::RemoveObserver(nsIImageGroupObserver *aObserver)
{
  PRBool ret;

  if (aObserver == nsnull || mObservers == nsnull) {
    return PR_FALSE;
  }
  
  ret = mObservers->RemoveElement((void *)aObserver);
  
  if (ret == PR_TRUE) {
    NS_RELEASE(aObserver);
  }
  
  return ret;
}

nsIImageRequest* 
ImageGroupImpl::GetImage(const char* aUrl, 
                         nsIImageRequestObserver *aObserver,
                         const nscolor* aBackgroundColor,
                         PRUint32 aWidth, PRUint32 aHeight,
                         PRUint32 aFlags)
{
  NS_PRECONDITION(nsnull != aUrl, "null URL");
  
  ImageRequestImpl *image_req = new ImageRequestImpl();
  if (image_req != nsnull) {
    ilINetContext* net_cx;

    // Create an async net context, and ask the image request object
    // to get the image
    if (NS_SUCCEEDED(NS_NewImageNetContext(&net_cx)) &&
        NS_SUCCEEDED(image_req->Init(mGroupContext, aUrl, aObserver, aBackgroundColor,
                                     aWidth, aHeight, aFlags, net_cx))) {
      NS_ADDREF(image_req);
    } else {
      delete image_req;
      image_req = nsnull;
    }
  }

  return image_req;
}
  
void 
ImageGroupImpl::Interrupt(void)
{
  if (mGroupContext != nsnull) {
    IL_InterruptContext(mGroupContext);
  }
}

extern "C" NS_GFX_(nsresult)
NS_NewImageGroup(nsIImageGroup **aInstancePtrResult)
{
  nsresult result;

  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIImageManager *manager;
  if ((result = NS_NewImageManager(&manager)) != NS_OK) {
      return result;
  }

  nsIImageGroup *group = new ImageGroupImpl(manager);
  if (group == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_RELEASE(manager);

  return group->QueryInterface(kIImageGroupIID, (void **) aInstancePtrResult);
}


static void ns_observer_proc (XP_Observable aSource,
                              XP_ObservableMsg	aMsg, 
                              void* aMsgData, 
                              void* aClosure)
{
  ImageGroupImpl *image_group = (ImageGroupImpl *)aClosure;
  nsVoidArray *observer_list = image_group->GetObservers();

  if (observer_list != nsnull) {
    PRInt32 i, count = observer_list->Count();
    nsIImageGroupObserver *observer;
    for (i = 0; i < count; i++) {
      observer = (nsIImageGroupObserver *)observer_list->ElementAt(i);
      if (observer != nsnull) {
        switch (aMsg) {
          case IL_STARTED_LOADING:
            observer->Notify(image_group,  
                             nsImageGroupNotification_kStartedLoading);
            break;
          case IL_ABORTED_LOADING:
            observer->Notify(image_group, 
                             nsImageGroupNotification_kAbortedLoading);
          case IL_FINISHED_LOADING:
            observer->Notify(image_group, 
                             nsImageGroupNotification_kFinishedLoading);
          case IL_STARTED_LOOPING:
            observer->Notify(image_group, 
                             nsImageGroupNotification_kStartedLooping);
          case IL_FINISHED_LOOPING:
            observer->Notify(image_group, 
                             nsImageGroupNotification_kFinishedLooping);
        }
      }
    }
  }
}

