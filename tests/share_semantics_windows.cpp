#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
int wmain() {
 wchar_t dir[MAX_PATH]{}, path[MAX_PATH]{}, target[MAX_PATH]{};
 if (!GetTempPathW(MAX_PATH, dir) || !GetTempFileNameW(dir, L"wfu", 0, path)) return 1;
 if (wcscpy_s(target, MAX_PATH, path) != 0 || wcscat_s(target, MAX_PATH, L"-renamed") != 0) { DeleteFileW(path); return 2; }

 // A handle without FILE_SHARE_DELETE must prevent rename.
 HANDLE restrictive=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(restrictive==INVALID_HANDLE_VALUE){DeleteFileW(path);return 3;}
 SetLastError(ERROR_SUCCESS);
 const BOOL wronglyRenamed=MoveFileExW(path,target,MOVEFILE_REPLACE_EXISTING);
 const DWORD blockedError=GetLastError();
 CloseHandle(restrictive);
 if(wronglyRenamed || blockedError!=ERROR_SHARING_VIOLATION){DeleteFileW(path);DeleteFileW(target);return 4;}

 // Adding FILE_SHARE_DELETE allows rename while the old handle stays readable.
 HANDLE permissive=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(permissive==INVALID_HANDLE_VALUE){DeleteFileW(path);return 5;}
 if(!MoveFileExW(path,target,MOVEFILE_REPLACE_EXISTING)){CloseHandle(permissive);DeleteFileW(path);return 6;}
 LARGE_INTEGER size{};
 const bool readable=GetFileSizeEx(permissive,&size)!=FALSE;
 CloseHandle(permissive); DeleteFileW(target);
 return readable?0:7;
}
