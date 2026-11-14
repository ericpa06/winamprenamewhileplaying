#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int wmain() {
 wchar_t dir[MAX_PATH]{}, path[MAX_PATH]{}, target[MAX_PATH]{};
 if (!GetTempPathW(MAX_PATH, dir) || !GetTempFileNameW(dir, L"wfu", 0, path)) return 1;
 HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(h==INVALID_HANDLE_VALUE){DeleteFileW(path);return 2;}
 if(swprintf_s(target,L"%s-renamed",path)<0){CloseHandle(h);DeleteFileW(path);return 3;}
 if(!MoveFileExW(path,target,MOVEFILE_REPLACE_EXISTING)){CloseHandle(h);DeleteFileW(path);return 4;}
 LARGE_INTEGER size{};
 const bool readable=GetFileSizeEx(h,&size)!=FALSE;
 CloseHandle(h); DeleteFileW(target);
 return readable?0:5;
}
