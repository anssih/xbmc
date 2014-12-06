/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include <EGL/egl.h>
#include "EGLNativeTypeAndroid.h"
#include "utils/log.h"
#include "guilib/gui3d.h"
#if defined(TARGET_ANDROID)
  #include "android/activity/XBMCApp.h"
  #include "android/activity/EventDispatcher.h"
  #include "android/jni/Display.h"
  #include "android/jni/View.h"
  #include "android/jni/Window.h"
  #include "android/jni/WindowManager.h"
  #if defined(HAS_AMLPLAYER) || defined(HAS_LIBAMCODEC)
    #include "utils/AMLUtils.h"
  #endif
#endif
#include "utils/StringUtils.h"

CEGLNativeTypeAndroid::CEGLNativeTypeAndroid()
{
}

CEGLNativeTypeAndroid::~CEGLNativeTypeAndroid()
{
} 

bool CEGLNativeTypeAndroid::CheckCompatibility()
{
#if defined(TARGET_ANDROID)
  return true;
#endif
  return false;
}

void CEGLNativeTypeAndroid::Initialize()
{
#if defined(TARGET_ANDROID) && (defined(HAS_AMLPLAYER) || defined(HAS_LIBAMCODEC))
  aml_permissions();
  aml_cpufreq_min(true);
  aml_cpufreq_max(true);
#endif

  return;
}
void CEGLNativeTypeAndroid::Destroy()
{
#if defined(TARGET_ANDROID) && (defined(HAS_AMLPLAYER) || defined(HAS_LIBAMCODEC))
  aml_cpufreq_min(false);
  aml_cpufreq_max(false);
#endif

  return;
}

bool CEGLNativeTypeAndroid::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeAndroid::CreateNativeWindow()
{
#if defined(TARGET_ANDROID)
  // Android hands us a window, we don't have to create it
  return true;
#else
  return false;
#endif
}  

bool CEGLNativeTypeAndroid::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeAndroid::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
#if defined(TARGET_ANDROID)
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) CXBMCApp::GetNativeWindow(30000);
  CLog::Log(LOGNOTICE, "%s: XYZ Native display 0x%x.",__FUNCTION__, (long)*nativeWindow);
  return (*nativeWindow != NULL);
#else
  return false;
#endif
}

bool CEGLNativeTypeAndroid::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeAndroid::DestroyNativeWindow()
{
  return true;
}

static float lastRate = 60;

#include <pthread.h>

static void currentattributeprint(CJNIWindow& window)
{
    CLog::Log(LOGNOTICE, "XYZ - cur preferred rate %f", window.getAttributes().getpreferredRefreshRate());
}

static float currentRate()
{
  CJNIWindow window = CXBMCApp::getWindow();
  if (window)
  {
      
    float preferredRate = window.getAttributes().getpreferredRefreshRate();
    if (preferredRate > 1.0)
        return preferredRate;
//     CLog::Log(LOGNOTICE, "XYZ - YES WINDOW");
//     currentattributeprint(window);
    CJNIView view(window.getDecorView());
    if (view) {
        CJNIDisplay display(view.getDisplay());
        if (display)
        {
            float reportedRate = display.getRefreshRate();
//             CLog::Log(LOGNOTICE, "XYZ - YES RATE %f", reportedRate);
            return reportedRate;
        }
    }

  }
  return 60.0;
}

bool CEGLNativeTypeAndroid::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(TARGET_ANDROID)
  EGLNativeWindowType *nativeWindow = (EGLNativeWindowType*)CXBMCApp::GetNativeWindow(30000);
  if (!nativeWindow)
    return false;

  ANativeWindow_acquire(*nativeWindow);
  res->iWidth = ANativeWindow_getWidth(*nativeWindow);
  res->iHeight= ANativeWindow_getHeight(*nativeWindow);
  ANativeWindow_release(*nativeWindow);

// CLog::Log(LOGNOTICE,"IS NOT WINDOW: %d\n", !CXBMCApp::getWindow().get_raw() ? 1 : 0);

  CLog::Log(LOGNOTICE, "CALL FROM THREAD 0x%x", (unsigned int)pthread_self());
//   float reportedRate = -1.0;
//   CJNIWindow window = CXBMCApp::getWindow();
//   if (window)
//   {
//     CLog::Log(LOGNOTICE, "XYZ - YES WINDOW");
//     currentattributeprint(window);
//     CJNIView view(window.getDecorView());
//     if (view) {
//         CJNIDisplay display(view.getDisplay());
//         if (display)
//         {
//             reportedRate = display.getRefreshRate();
//             CLog::Log(LOGNOTICE, "XYZ - YES RATE %f", reportedRate);
//         }
//     }
//     
// // //     CJNIWindowManagerLayoutParams params = window.getAttributes();
// // //   
// // //     params.setpreferredRefreshRate(50.000000f);
// // //     if (params.getpreferredRefreshRate() > 0.0)
// // //     {
// // //       window.setAttributes(params);
// // //       CLog::Log(LOGNOTICE, "XYZ - TRY TO SET RATE IN GET - OK RATE %f", params.getpreferredRefreshRate());
// // // //       lastRate = res.fRefreshRate;
// // // //       return true;
// // //       currentattributeprint(window);
// // //     }
// 
//   }
//   else
//   {
//     CLog::Log(LOGNOTICE, "XYZ - NO WINDOW, last rate %f", lastRate);
// //     for (int x = 0; x < 600; x++)
// //     {
// //         (void)(bool)CXBMCApp::getWindow();
// //     }
//   }

//   if (reportedRate > 0.0)
//     res->fRefreshRate = lastRate = reportedRate;
//   else
    res->fRefreshRate = currentRate() /*60*/;
  res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode       = StringUtils::Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
  res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  CLog::Log(LOGNOTICE,"Current resolution: %s\n",res->strMode.c_str());
//   if (res->iScreenWidth < 0 || res->iScreenHeight < 0) {
//       res->iScreenWidth = 1920;
//       res->iScreenHeight = 1080;
//       return false;
//   }
//   if (res->iScreenHeight < 0)
//       return false;
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeAndroid::SetNativeResolution(const RESOLUTION_INFO &res)
{
#if defined(TARGET_ANDROID)
//    res.fRefreshRate = 24.000025;
    CLog::Log(LOGNOTICE, "CALL FROM THREAD 0x%x", (unsigned int)pthread_self());
  if (res.fRefreshRate < 1.0)
      return false;

//   RESOLUTION_INFO curres;
//   GetNativeResolution(&curres);
  if (abs(currentRate() - res.fRefreshRate) < 0.0001) {
      CLog::Log(LOGNOTICE, "XYZ SKIPPING CALL AS RES ALL OK");
      return true;
  }

//   g_Windowing.DestroyWindow();
  CXBMCApp::SetRefreshRate(res.fRefreshRate);

//   sleep(5);
//   CEventDispatcher::SubmitSetRefreshRate(res.fRefreshRate);
  return true;
//   CLog::Log(LOGNOTICE, "XYZ - TRYING RATE %f", res.fRefreshRate);
//   CJNIWindow window = CXBMCApp::getWindow();
//   if (window)
//   {
//   currentattributeprint(window);
//     CJNIWindowManagerLayoutParams params = window.getAttributes();
//   
//     params.setpreferredRefreshRate(res.fRefreshRate);
//     if (params.getpreferredRefreshRate() > 0.0)
//     {
//       window.setAttributes(params);
//       CLog::Log(LOGNOTICE, "XYZ - OK RATE %f", res.fRefreshRate);
//       lastRate = res.fRefreshRate;
//       currentattributeprint(window);
//       return true;
//     }
//   }
#endif
  return false;
}

bool CEGLNativeTypeAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
    CLog::Log(LOGNOTICE, "CALL FROM THREAD 0x%x", (unsigned int)pthread_self());
  RESOLUTION_INFO res;
  bool ret = false;
  ret = GetNativeResolution(&res);
  if (ret && res.iWidth > 1 && res.iHeight > 1)
  {
    std::vector<float> refreshRates;
#if defined(TARGET_ANDROID)
    CJNIWindow window = CXBMCApp::getWindow();
    if (window)
    {
        CLog::Log(LOGNOTICE, "XYZ OK 1");
        CJNIView view = window.getDecorView();
        if (view)
        {
            CLog::Log(LOGNOTICE, "XYZ OK 2");
            CJNIDisplay display = view.getDisplay();
            if (display)
            {
                CLog::Log(LOGNOTICE, "XYZ OK 3");
                refreshRates = display.getSupportedRefreshRates();
            }
        }
    }
#endif

    if (refreshRates.size())
    {
      for (unsigned int i = 0; i < refreshRates.size(); i++)
      {
        CLog::Log(LOGNOTICE, "XYZ - SUPPORTED RATE %f", refreshRates[i]);
        res.fRefreshRate = refreshRates[i];
        resolutions.push_back(res);
      }
    }
    else
    {
      /* No refresh rate list available, just provide the current one */
      resolutions.push_back(res);
    }
    return true;
  }
  return false;
}

bool CEGLNativeTypeAndroid::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return false;
}

bool CEGLNativeTypeAndroid::ShowWindow(bool show)
{
  return false;
}
