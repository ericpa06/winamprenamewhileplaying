#pragma once
#include <windows.h>
#define GPPHDR_VER 0x10
#define GEN_INIT_SUCCESS 0
#define GEN_INIT_FAILURE 1
typedef struct winampGeneralPurposePlugin {
 int version; char *description;
 int (__cdecl *init)(); void (__cdecl *config)(); void (__cdecl *quit)();
 HWND hwndParent; HINSTANCE hDllInstance;
} winampGeneralPurposePlugin;
