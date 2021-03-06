#include "common.h"


static UINT hook_codepage(UINT old_codepage)
{
    return old_codepage == CP_ACP || old_codepage == CP_THREAD_ACP || old_codepage == system_codepage || old_codepage == CODEPAGE_CHS || old_codepage == CODEPAGE_CHT ? target_codepage : old_codepage;
}

static int WINAPI My_MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)
{
    CodePage = hook_codepage(CodePage);
    return MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}

static int WINAPI My_WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar)
{
    CodePage = hook_codepage(CodePage);
    return WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);
}

static void check_badpath()
{
    wchar_t buf[MAXLINE];
    if (GetModuleFileNameW(NULL, buf, MAXLINE) != 0) {
        wchar_t *ptr;
        for (ptr = buf; *ptr; ptr++) {
            if (*ptr < 0x20 || *ptr > 0x7E) {
                break;
            }
        }
        if (*ptr) {
            MessageBoxW(NULL, wstr_badpath_text, wstr_badpath_title, MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
        }
    }
}

MAKE_PATCHSET(setlocale)
{   
    if (flag == GAME_LOCALE_AUTODETECT) { // auto detect
        flag = game_locale;
        
        if (flag == GAME_LOCALE_UNKNOWN) {
            warning("unable to detect game locale, fallback to system default.");
            return;
        }
    }
    
    if (flag == GAME_LOCALE_CHS) target_codepage = CODEPAGE_CHS; // CHS
    else if (flag == GAME_LOCALE_CHT) target_codepage = CODEPAGE_CHT; // CHT
    else fail("unknown language flag %d in setlocale.", flag);
    
    if (system_codepage != target_codepage) {
        
        check_badpath();
        
        // hook GBENGINE.DLL's IAT
        make_pointer(gboffset + 0x100F50A0, My_MultiByteToWideChar);
        make_pointer(gboffset + 0x100F50DC, My_WideCharToMultiByte);
        // hook PAL3.EXE's IAT
        make_pointer(0x0056A128, My_MultiByteToWideChar);
        make_pointer(0x0056A12C, My_WideCharToMultiByte);
        
        if (GET_PATCHSET_FLAG(testcombat)) {
            // patch IAT by replacing known function address (address returned by GetProcAddress())
            // note: this method is not compatible with KernelEx for win9x (also, win9x doesn't support unicode)

            if (is_win9x()) {
                warning("setlocale with testcombat doesn't support win9x.");
                return;
            }
            // hook COMCTL32.DLL's IAT for testcombat
            hook_import_table(GetModuleHandle("COMCTL32.DLL"), "KERNEL32.DLL", "MultiByteToWideChar", My_MultiByteToWideChar);
            hook_import_table(GetModuleHandle("COMCTL32.DLL"), "KERNEL32.DLL", "WideCharToMultiByte", My_WideCharToMultiByte);
        }
        
        // no need (and shouldn't) to hook myself!
    }
}
