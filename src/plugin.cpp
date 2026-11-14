#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include <atomic>
#include "gen.h"
#include "path_scope.h"
using CreateFileWFn=HANDLE (WINAPI*)(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
using CreateFileAFn=HANDLE (WINAPI*)(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
static CreateFileWFn realW=nullptr; static CreateFileAFn realA=nullptr;
static std::atomic<unsigned long> adjusted{0};
static HANDLE WINAPI HookW(LPCWSTR p,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD disp,DWORD flags,HANDLE tmpl) {
 if(disp==OPEN_EXISTING && fileunlock::ReadAccess(access) && !(flags&(FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_DELETE_ON_CLOSE)) && p && fileunlock::LocalPath(p)) {
  DWORD n=share|FILE_SHARE_DELETE; if(n!=share){share=n;adjusted.fetch_add(1);}
 }
 return realW(p,access,share,sa,disp,flags,tmpl);
}
static HANDLE WINAPI HookA(LPCSTR p,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD disp,DWORD flags,HANDLE tmpl) {
 if(disp==OPEN_EXISTING && fileunlock::ReadAccess(access) && !(flags&(FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_DELETE_ON_CLOSE)) && p && p[0] && p[1]==':' ) {
  DWORD n=share|FILE_SHARE_DELETE; if(n!=share){share=n;adjusted.fetch_add(1);}
 }
 return realA(p,access,share,sa,disp,flags,tmpl);
}
static int __cdecl Init() {
 if(MH_Initialize()!=MH_OK) return GEN_INIT_FAILURE;
 HMODULE k=GetModuleHandleW(L"KernelBase.dll"); if(!k) k=GetModuleHandleW(L"kernel32.dll");
 if(!k) return GEN_INIT_FAILURE;
 FARPROC w=GetProcAddress(k,"CreateFileW"), a=GetProcAddress(k,"CreateFileA");
 if(w && MH_CreateHook((LPVOID)w,(LPVOID)HookW,(LPVOID*)&realW)==MH_OK) MH_EnableHook((LPVOID)w);
 if(a && MH_CreateHook((LPVOID)a,(LPVOID)HookA,(LPVOID*)&realA)==MH_OK) MH_EnableHook((LPVOID)a);
 return (realW||realA)?GEN_INIT_SUCCESS:GEN_INIT_FAILURE;
}
static void __cdecl Config() {
 wchar_t msg[160]; wsprintfW(msg,L"Adjusted file opens: %lu\nMode: rename/replace (FILE_SHARE_DELETE)",(unsigned long)adjusted.load());
 MessageBoxW(nullptr,msg,L"Winamp File Share Unlocker",MB_OK|MB_ICONINFORMATION);
}
static void __cdecl Quit() { MH_DisableHook(MH_ALL_HOOKS); MH_Uninitialize(); }
static char pluginDescription[] = "Winamp File Share Unlocker";
static winampGeneralPurposePlugin plugin={GPPHDR_VER,pluginDescription,Init,Config,Quit,nullptr,nullptr};
extern "C" __declspec(dllexport) winampGeneralPurposePlugin *winampGetGeneralPurposePlugin(){return &plugin;}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID) { if(reason==DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(h); return TRUE; }
