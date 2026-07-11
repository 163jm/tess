#pragma once

#include <windows.h>
#include <unknwn.h>
#include <restrictederrorinfo.h>
#include <hstring.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Navigation.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Windows.Storage.Pickers.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <microsoft.ui.xaml.window.h> // IWindowNative
#include <shobjidl.h>                 // IInitializeWithWindow (FileOpenPicker 窗口绑定)

// ANGLE / EGL（通过 NuGet ANGLE.WindowsStore 提供，见 README_VS_PROJECT.md）
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

// libmpv 播放器封装（Core 模块）
#include "MpvPlayer.h"

#include <string>
#include <memory>
