// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>
#include <shlobj.h>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/win/scoped_co_mem.h"
#include "base/win/windows_version.h"

// http://blogs.msdn.com/oldnewthing/archive/2004/10/25/247180.aspx
extern "C" IMAGE_DOS_HEADER __ImageBase;

using base::FilePath;

namespace base {

bool PathProviderWin(int key, FilePath* result) {
  // We need to go compute the value. It would be nice to support paths with
  // names longer than MAX_PATH, but the system functions don't seem to be
  // designed for it either, with the exception of GetTempPath (but other
  // things will surely break if the temp path is too long, so we don't bother
  // handling it.
  wchar_t system_buffer[MAX_PATH];
  system_buffer[0] = 0;

  FilePath cur;
  switch (key) {
    case base::FILE_EXE:
      GetModuleFileName(NULL, system_buffer, MAX_PATH);
      cur = FilePath(system_buffer);
      break;
    case base::FILE_MODULE: {
      // the resource containing module is assumed to be the one that
      // this code lives in, whether that's a dll or exe
      HMODULE this_module = reinterpret_cast<HMODULE>(&__ImageBase);
      GetModuleFileName(this_module, system_buffer, MAX_PATH);
      cur = FilePath(system_buffer);
      break;
    }
    case base::DIR_WINDOWS:
      GetWindowsDirectory(system_buffer, MAX_PATH);
      cur = FilePath(system_buffer);
      break;
    case base::DIR_SYSTEM:
      GetSystemDirectory(system_buffer, MAX_PATH);
      cur = FilePath(system_buffer);
      break;
    case base::DIR_PROGRAM_FILESX86:
      if (base::win::OSInfo::GetInstance()->architecture() !=
          base::win::OSInfo::X86_ARCHITECTURE) {
        if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILESX86, NULL,
                                   SHGFP_TYPE_CURRENT, system_buffer)))
          return false;
        cur = FilePath(system_buffer);
        break;
      }
      // Fall through to base::DIR_PROGRAM_FILES if we're on an X86 machine.
    case base::DIR_PROGRAM_FILES:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_IE_INTERNET_CACHE:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_INTERNET_CACHE, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_COMMON_START_MENU:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_COMMON_PROGRAMS, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_START_MENU:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_COMMON_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_LOCAL_APP_DATA_LOW:
      if (win::GetVersion() < win::VERSION_VISTA)
        return false;

      // TODO(nsylvain): We should use SHGetKnownFolderPath instead. Bug 1281128
      if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer)))
        return false;
      cur = FilePath(system_buffer).DirName().AppendASCII("LocalLow");
      break;
    case base::DIR_LOCAL_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_SOURCE_ROOT: {
      FilePath executableDir;
      // On Windows, unit tests execute two levels deep from the source root.
      // For example:  chrome/{Debug|Release}/ui_tests.exe
      PathService::Get(base::DIR_EXE, &executableDir);
      cur = executableDir.DirName().DirName();
      break;
    }
#if defined(PATCH_BY_WUBENQI)
    // FOLDERID_ApplicationShortcuts这个常量的，显示未定义，没法编译。
    // 有可能是在WIN8SDK里定义，忽略此段代码
#else
    case base::DIR_APP_SHORTCUTS: {
      if (win::GetVersion() < win::VERSION_WIN8)
        return false;

      base::win::ScopedCoMem<wchar_t> path_buf;
      if (FAILED(SHGetKnownFolderPath(FOLDERID_ApplicationShortcuts, 0, NULL,
                                      &path_buf)))
        return false;

      cur = FilePath(string16(path_buf));
      break;
    }
#endif
    case base::DIR_USER_DESKTOP:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer))) {
        return false;
      }
      cur = FilePath(system_buffer);
      break;
    case base::DIR_COMMON_DESKTOP:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer))) {
        return false;
      }
      cur = FilePath(system_buffer);
      break;
    case base::DIR_USER_QUICK_LAUNCH:
      if (!PathService::Get(base::DIR_APP_DATA, &cur))
        return false;
      // According to various sources, appending
      // "Microsoft\Internet Explorer\Quick Launch" to %appdata% is the only
      // reliable way to get the quick launch folder across all versions of
      // Windows.
      // http://stackoverflow.com/questions/76080/how-do-you-reliably-get-the-quick-
      // http://www.microsoft.com/technet/scriptcenter/resources/qanda/sept05/hey0901.mspx
      cur = cur.AppendASCII("Microsoft")
                .AppendASCII("Internet Explorer")
                .AppendASCII("Quick Launch");
      break;
    case base::DIR_TASKBAR_PINS:
      if (!PathService::Get(base::DIR_USER_QUICK_LAUNCH, &cur))
        return false;
      cur = cur.AppendASCII("User Pinned");
      cur = cur.AppendASCII("TaskBar");
      break;
    case base::DIR_WINDOWS_FONTS:
      if (FAILED(SHGetFolderPath(
              NULL, CSIDL_FONTS, NULL, SHGFP_TYPE_CURRENT, system_buffer))) {
        return false;
      }
      cur = FilePath(system_buffer);
      break;
    default:
      return false;
  }

  *result = cur;
  return true;
}

}  // namespace base
