#pragma once
#include <string_view>
#include <string>
namespace fileunlock {
enum class SharePolicy : unsigned char { RenameReplaceOnly=0, EditorFriendly=1 };
inline bool ReadAccess(unsigned long a) noexcept { return (a & (0x80000000UL|1UL)) != 0; }
inline unsigned long Apply(unsigned long s, SharePolicy p) noexcept {
 return s | (p==SharePolicy::EditorFriendly ? 7UL : 4UL);
}
inline bool LocalPath(std::wstring_view p) noexcept {
 if(p.empty() || p.rfind(L"\\\\.\\",0)==0 || p.rfind(L"\\\\?\\GLOBALROOT",0)==0 ||
    p.rfind(L"\\Device\\",0)==0 || p.rfind(L"\\??\\",0)==0) return false;
 if(p.rfind(L"\\\\?\\",0)==0) return p.size()>7;
 if(p.size()>=3 && p[1]==L':' && (p[2]==L'\\'||p[2]==L'/')) return true;
 if(p.size()>=2 && p[0]==L'\\' && p[1]==L'\\') return true;
 return p.find(L'\\')!=std::wstring_view::npos || p.find(L'.')!=std::wstring_view::npos;
}
}
