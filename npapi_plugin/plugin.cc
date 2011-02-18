/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

//////////////////////////////////////////////////
//
// CPlugin class implementation
//

#include <string.h>
#include <iostream>
#include "plugin.h"
#include "depthjs.h"

/*const char* kSaveScreenshot = "SaveScreenshot";
const char* kAutoSave = "AutoSave";
const char* kSetSavePath = "SetSavePath";
const char* kOpenSavePath = "OpenSavePath";
const char* kGetDefaultSavePath = "GetDefaultSavePath";
const char* kSaveToClipboard = "SaveToClipboard";*/

const char* kInitDepthJS = "InitDepthJS";
const char* kShutdownDepthJS = "ShutdownDepthJS";

static NPClass plugin_ref_obj = {
  NP_CLASS_STRUCT_VERSION,
  ScriptablePluginObject::Allocate,
  ScriptablePluginObject::Deallocate,
  NULL,
  ScriptablePluginObject::HasMethod,
  ScriptablePluginObject::Invoke,
  ScriptablePluginObject::InvokeDefault,
  ScriptablePluginObject::HasProperty,
  ScriptablePluginObject::GetProperty,
  NULL,
  NULL,
};

ScriptablePluginObject::ScriptablePluginObject(NPP instance)
    : npp(instance) {
  std::cout << "DepthJS: ScriptablePluginObject constructor" << "\n";
}

NPObject* ScriptablePluginObject::Allocate(NPP instance, NPClass* npclass) {
  std::cout << "DepthJS: ScriptablePluginObject::Allocate" << "\n";
  return (NPObject*)(new ScriptablePluginObject(instance));
}

void ScriptablePluginObject::Deallocate(NPObject* obj) {
  std::cout << "DepthJS: ScriptablePluginObject::Deallocate" << "\n";
  ShutdownDepthJS();
  delete (ScriptablePluginObject*)obj;
}

bool ScriptablePluginObject::HasMethod(NPObject* obj, NPIdentifier methodName) {
  char* name = npnfuncs->utf8fromidentifier(methodName);
  std::cout << "DepthJS: ScriptablePluginObject::HasMethod: " << name << "\n";
  return true;
}

bool ScriptablePluginObject::InvokeDefault(NPObject* obj, const NPVariant* args,
                            uint32_t argCount, NPVariant* result) {
  std::cout << "DepthJS: ScriptablePluginObject::InvokeDefault" << "\n";
  return true;
}

bool ScriptablePluginObject::Invoke(NPObject* obj, NPIdentifier methodName,
                     const NPVariant* args, uint32_t argCount,
                     NPVariant* result) {
  ScriptablePluginObject *thisObj = (ScriptablePluginObject*)obj;
  char* name = npnfuncs->utf8fromidentifier(methodName);
  std::cout << "DepthJS: ScriptablePluginObject::Invoke: " << name << "\n";
  bool ret_val = false;
  if (!name) {
    return ret_val;
  }
  if (!strcmp(name, kInitDepthJS)) {
    ret_val = InitDepthJS(thisObj, args, argCount, result);
  } else if (!strcmp(name, kShutdownDepthJS)) {
    ret_val = ShutdownDepthJS(thisObj, args, argCount, result);
  } else {
    // Exception handling.
    npnfuncs->setexception(obj, "Unknown method");
  }
  npnfuncs->memfree(name);
  return ret_val;
}

bool ScriptablePluginObject::HasProperty(NPObject* obj, NPIdentifier propertyName) {
  char* name = npnfuncs->utf8fromidentifier(propertyName);
  std::cout << "DepthJS: ScriptablePluginObject::HasProperty: " << name << "\n";
  return false;
}

bool ScriptablePluginObject::GetProperty(NPObject* obj, NPIdentifier propertyName,
                          NPVariant* result) {
  char* name = npnfuncs->utf8fromidentifier(propertyName);
  std::cout << "DepthJS: ScriptablePluginObject::GetProperty: " << name << "\n";
  return false;
}

CPlugin::CPlugin(NPP pNPInstance) :
  m_pNPInstance(pNPInstance),
  m_bInitialized(false),
  m_pScriptableObject(NULL) {
#ifdef _WINDOWS
  m_hWnd = NULL;
#endif
  std::cout << "DepthJS Plugin: Constructing CPlugin" << "\n";
}

CPlugin::~CPlugin() {
  if (m_pScriptableObject)
    npnfuncs->releaseobject((NPObject*)m_pScriptableObject);
#ifdef _WINDOWS
  m_hWnd = NULL;
#endif
  m_bInitialized = false;
  std::cout << "DepthJS Plugin: Destructing CPlugin" << "\n";
}

NPBool CPlugin::init(NPWindow* pNPWindow) {
  if(pNPWindow == NULL)
    return false;
#ifdef _WINDOWS
  m_hWnd = (HWND)pNPWindow->window;
  if(m_hWnd == NULL)
    return false;
#endif
  m_Window = pNPWindow;
  m_bInitialized = true;
  std::cout << "DepthJS Plugin: Finished initalizating" << "\n";
  return true;
}

NPBool CPlugin::isInitialized() {
  std::cout << "DepthJS Plugin: isInitialized: " << (m_bInitialized ? "yes" : "no") << "\n";
  return m_bInitialized;
}

ScriptablePluginObject * CPlugin::GetScriptableObject() {
  std::cout << "DepthJS Plugin: GetScriptableObject" << "\n";
  if (!m_pScriptableObject) {
    m_pScriptableObject = (ScriptablePluginObject*)npnfuncs->createobject(m_pNPInstance, &plugin_ref_obj);

    // Retain the object since we keep it in plugin code
    // so that it won't be freed by browser.
    npnfuncs->retainobject((NPObject*)m_pScriptableObject);
  }

  return m_pScriptableObject;
}

#ifdef _WINDOWS
HWND CPlugin::GetHWnd() {
  return m_hWnd;
}
#endif
