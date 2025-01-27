/**
 * @file orxExtensions.h
 * !! This file is automatically generated by init, do not modify manually. !!
 * @date 27-Jan-2025
 */

#ifndef _orxEXTENSIONS_H_
#define _orxEXTENSIONS_H_

#define orxBUNDLE_IMPL
#include "orxBundle.h"
#undef orxBUNDLE_IMPL

void InitExtensions()
{
}

void ExitExtensions()
{
  // Exit from bundle support
  orxBundle_Exit();

}

void BootstrapExtensions()
{
  // Initialize bundle resource type
  orxBundle_Init();

  // Add config storage to find the initial config file
  orxResource_AddStorage(orxCONFIG_KZ_RESOURCE_GROUP, orxBUNDLE_KZ_RESOURCE_STORAGE, orxFALSE);
  orxResource_AddStorage(orxCONFIG_KZ_RESOURCE_GROUP, orxBUNDLE_KZ_RESOURCE_STORAGE "LargeWorld.obr", orxFALSE);
  orxResource_AddStorage(orxCONFIG_KZ_RESOURCE_GROUP, "../data/config", orxFALSE);
}

#endif // _orxEXTENSIONS_H_
