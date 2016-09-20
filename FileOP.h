//------------------------------------------------------------------------------
//
//    Copyright (C) Streamlet. All rights reserved.
//
//    File Name:   FileOP.h
//    Author:      Streamlet
//    Create Time: 2016-09-20
//    Description: 
//
//------------------------------------------------------------------------------

#ifndef __FILEOP_H_8DB99F9D_F271_4988_80A8_6EE0C3A9C69F_INCLUDED__
#define __FILEOP_H_8DB99F9D_F271_4988_80A8_6EE0C3A9C69F_INCLUDED__


#include <Windows.h>
#include <string>
#include <assert.h>
#include "External/stdex/string.h"
#include "FileScanner.h"

namespace xl
{
    namespace Win32
    {
        namespace File
        {
            bool MakeDir(LPCWSTR lpszPath)
            {
                assert(lpszPath);
                std::wstring strPath(lpszPath);
                std::vector<stdex::shadow_wstring> vecDir = stdex::str_split(strPath, L'\\');
                for each (auto ss in vecDir)
                {
                    if (ss.length() == 0)
                        continue;
                    size_t i = (ss.c_str() - strPath.c_str()) + ss.length();
                    strPath[i] = L'\0';
                    if (!::CreateDirectoryW(strPath.c_str(), NULL))
                        return false;
                    strPath[i] = L'\\';
                }
                return true;
            }

            bool RMDir(LPCWSTR lpszPath)
            {
                assert(lpszPath && lpszPath[0] != L'\\');
                if (!EnumFiles(lpszPath, L"*", ENUM_RECURSIVELY | ENUM_SUB_FIRST, [](LPCWSTR lpszFile, DWORD dwAttr, LPVOID lpParam)
                {
                    if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
                        return !!::RemoveDirectoryW(lpszFile);
                    else
                        return !!::DeleteFileW(lpszFile);
                }, NULL))
                {
                    return false;
                }

                return !!::RemoveDirectoryW(lpszPath);
            }

            typedef bool(*GET_CONTENT_PROC)(LPCVOID, size_t, LPVOID);
            template <typename Callback>
            bool GetContent(LPCWSTR lpszPath, Callback callback, LPVOID lpParam)
            {
                DWORD dwAttr = ::GetFileAttributesW(lpszPath);
                HANDLE hFile = ::CreateFileW(lpszPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, dwAttr, NULL);
                if (hFile == INVALID_HANDLE_VALUE)
                    return false;
                bool bRet = false;

                LARGE_INTEGER li = {};
                if (!::GetFileSizeEx(hFile, &li) || li.HighPart != 0)
                    goto EXIT;
                HANDLE hMap = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, li.HighPart, li.LowPart, NULL);
                if (hMap == NULL)
                    goto EXIT;
                LPCVOID lpMemory = ::MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, (SIZE_T)li.QuadPart);
                if (lpMemory == NULL)
                    goto EXIT;
                bRet = callback(lpMemory, (size_t)li.QuadPart, lpParam);
            EXIT:
                if (lpMemory)
                    UnmapViewOfFile(lpMemory);
                if (hMap)
                    CloseHandle(hMap);
                if (hFile)
                    CloseHandle(hFile);
                return bRet;
            }

            std::string GetContent(LPCWSTR lpszPath)
            {
                std::string strRet;
                GetContent(lpszPath, [&strRet](LPCVOID lpBuffer, size_t cbSize, LPVOID lpParam)
                {
                    strRet.assign((const char *)lpBuffer, cbSize);
                    return true;
                }, NULL);
                return strRet;
            }

            bool SetContent(LPCWSTR lpszPath, LPCVOID lpBuffer, size_t cbSize, bool bAppend = false)
            {
                DWORD dwAttr = ::GetFileAttributesW(lpszPath);
                HANDLE hFile = ::CreateFileW(lpszPath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, dwAttr == INVALID_FILE_ATTRIBUTES ? 0 : dwAttr, NULL);
                if (hFile == INVALID_HANDLE_VALUE)
                    return false;
                bool bRet = false;

                LARGE_INTEGER li = {};
                if (!SetFilePointerEx(hFile, li, &li, bAppend ? FILE_END : FILE_BEGIN))
                    goto EXIT;
                DWORD dwWritten = 0;
                if (!WriteFile(hFile, lpBuffer, cbSize, &dwWritten, NULL) || dwWritten != cbSize)
                    goto EXIT;
                if (!SetEndOfFile(hFile))
                    goto EXIT;
                bRet = true;
            EXIT:
                if (hFile)
                    CloseHandle(hFile);
                return bRet;
            }

            bool SetContent(LPCWSTR lpszPath, const std::string &strContent, bool bAppend = false)
            {
                return SetContent(lpszPath, strContent.c_str(), strContent.length(), bAppend);
            }
        }
    }
}

#endif // #ifndef __FILEOP_H_8DB99F9D_F271_4988_80A8_6EE0C3A9C69F_INCLUDED__
