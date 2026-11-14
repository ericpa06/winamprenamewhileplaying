#include "path_scope.h"
#include <cstdlib>
int main(){
 using namespace fileunlock;
 if(!ReadAccess(0x80000000UL)) return 1;
 if(ReadAccess(0x40000000UL)) return 2;
 if(Apply(1,SharePolicy::RenameReplaceOnly)!=5) return 3;
 if(Apply(1,SharePolicy::EditorFriendly)!=7) return 4;
 if(!LocalPath(L"C:\\Music\\a.ogg")) return 5;
 if(LocalPath(L"\\\\.\\pipe\\x")) return 6;
 return 0;
}
