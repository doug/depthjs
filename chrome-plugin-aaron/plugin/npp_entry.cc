/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
* This code was based on the npsimple.c sample code in Gecko-sdk.
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
* ***** END LICENSE BLOCK ***** */

#include <stdio.h>
#include <iostream>
#include "plugin.h"
#include "nptypes.h"
#include "npapi.h"


NPError NPP_GetValue(NPP instance, NPPVariable variable, void* value) {
  std::cout << "DepthJS Plugin: NPP_GetValue" << "\n";
  switch(variable) {
  default:
    return NPERR_GENERIC_ERROR;
  case NPPVpluginNameString:
    *((char **)value) = "DepthJSPlugin";
    break;
  case NPPVpluginDescriptionString:
    *((char **)value) = "DepthJSPlugin plugin.";
    break;
  case NPPVpluginScriptableNPObject: {
      if(instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;
      CPlugin * pPlugin = (CPlugin *)instance->pdata;
      if(pPlugin == NULL)
        return NPERR_GENERIC_ERROR;
      *(NPObject **)value = (NPObject*)pPlugin->GetScriptableObject();
    }
    break;
  case NPPVpluginNeedsXEmbed:
    *((char *)value) = 1;
    break;
  }
  return NPERR_NO_ERROR;
}

NPError NPP_New(NPMIMEType pluginType, NPP instance,
                uint16_t mode, int16_t argc, char* argn[],
                char* argv[], NPSavedData* saved) {
  std::cout << "DepthJS Plugin: NPP_New" << "\n";
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

#ifdef _WINDOWS
  int bWindowed = 1;
#else
  int bWindowed = 0;
#endif
  npnfuncs->setvalue(instance, NPPVpluginWindowBool, (void *)bWindowed);

  CPlugin * pPlugin = new CPlugin(instance);
  if(pPlugin == NULL)
    return NPERR_OUT_OF_MEMORY_ERROR;

  instance->pdata = (void *)pPlugin;

  return NPERR_NO_ERROR;
}

NPError NPP_Destroy(NPP instance, NPSavedData** save) {
  std::cout << "DepthJS Plugin: NPP_Destroy" << "\n";
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  CPlugin * pPlugin = (CPlugin *)instance->pdata;
  if(pPlugin != NULL)
    delete pPlugin;
  return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP instance, NPWindow* window) {
  std::cout << "DepthJS Plugin: NPP_SetWindow" << "\n";
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  if(window == NULL)
    return NPERR_GENERIC_ERROR;

  CPlugin * pPlugin = (CPlugin *)instance->pdata;
  if(pPlugin == NULL)
    return NPERR_GENERIC_ERROR;

  // window just created
  if(!pPlugin->isInitialized() && (window->window != NULL)) {
    if(!pPlugin->init(window)) {
      delete pPlugin;
      pPlugin = NULL;
      return NPERR_MODULE_LOAD_FAILED_ERROR;
    }
  }

  return NPERR_NO_ERROR;
}

NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream,
                      NPBool seekable, uint16_t* stype) {
  std::cout << "DepthJS Plugin: NPP_NewStream" << "\n";
  return NPERR_GENERIC_ERROR;
}

NPError NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason) {
  std::cout << "DepthJS Plugin: NPP_DestroyStream" << "\n";
  return NPERR_GENERIC_ERROR;
}

int16_t NPP_HandleEvent(NPP instance, void* event) {
  EventRecord* carbonEvent = (EventRecord*)event;
  if (carbonEvent && carbonEvent->what != 0) {
    std::cout << "DepthJS Plugin: NPP_HandleEvent type: " << carbonEvent->what << "\n";
  }
  /*
  NPCocoaEvent* cocoaEvent = (NPCocoaEvent*)event;
  std::cout << "DepthJS: NPP_HandleEvent type: " << cocoaEvent->type << "\n";
  */
  return 0;
}

