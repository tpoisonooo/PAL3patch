#include "common.h"

// file functions
#define MY_REG_FILE "save\\registry.txt"
#define MAX_REG_ITEM 100

struct reg_item {
    char *key1;
    char *key2;
    unsigned val;
};

static struct reg_item reg[MAX_REG_ITEM];
static int nr_reg;

static struct reg_item reg_default[] = {

    { "SOFTWARE\\SOFTSTAR\\PAL3A_MOVIE"   , "MOVIE_A" },

    { "SOFTWARE\\SOFTSTAR\\PAL3A_SAVE"    , "FINISH"  },
    { "SOFTWARE\\SOFTSTAR\\PAL3A_SAVE"    , "SEEK"    },
    { "SOFTWARE\\SOFTSTAR\\PAL3A_SAVE"    , "snap"    },

    { "SOFTWARE\\SOFTSTAR\\PAL3A_SUBGAME" , "LEVEL"   },

    { "SOFTWARE\\SOFTSTAR\\PAL3A_TASK"    , "SEEK"    },

    { NULL, NULL } // EOF
};

static int reg_cmp(const void *a, const void *b)
{
    const struct reg_item *pa = a, *pb = b;
    int ret = strcmp(pa->key1, pb->key1);
    if (ret) return ret;
    return strcmp(pa->key2, pb->key2);
}
static int alloc_reg()
{
    if (nr_reg >= MAX_REG_ITEM) fail("too many registry items.");
    return nr_reg++;
}
static void load_reg()
{
    nr_reg = 0;
    FILE *fp = fopen(MY_REG_FILE, "r");
    if (!fp) return;
    char buf1[MAXLINE], buf2[MAXLINE];
    unsigned val;
    int ret;
    fscanf(fp, UTF8_BOM_STR);
    while (1) {
        // skip comment lines
        char tmp[3];
        while (fscanf(fp, " %2[;#]%*[^\n] ", tmp) == 1);
        
        // read registry tuple
        ret = fscanf(fp, MAXLINEFMT MAXLINEFMT "%x ", buf1, buf2, &val);
        if (ret != 3) break;
        int id = alloc_reg();
        reg[id].key1 = strdup(buf1);
        reg[id].key2 = strdup(buf2);
        reg[id].val = val;
    }
    fclose(fp);
    if (ret != 0 && ret != EOF) fail("can't parse registry file.");
    qsort(reg, nr_reg, sizeof(struct reg_item), reg_cmp);
    int i;
    for (i = 1; i < nr_reg; i++) {
        if (reg_cmp(&reg[i - 1], &reg[i]) == 0) {
            fail("duplicate registry key:\n  '%s'\n  '%s'", reg[i].key1, reg[i].key2);
        }
    }
}
static void save_reg()
{
    PrepareDir(); // call PrepareDir() to create the "./save" Directory
    qsort(reg, nr_reg, sizeof(struct reg_item), reg_cmp);
    FILE *fp = fopen(MY_REG_FILE, "w");
    if (fp) {
        fputs(UTF8_BOM_STR, fp);
        fprintf(fp, "; PAL3A registry save file\n");
        fprintf(fp, "; generated by PAL3Apatch %s (built on %s)\n", patch_version, build_date);
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);
    	fprintf(fp, "; last modification: %04hu-%02hu-%02hu %02hu:%02hu:%02hu.%03hu\n", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
    	fprintf(fp, "\n");
    	fprintf(fp, "%-40s%-15s%s\n", "; HKLM subkey", "name", "value");
        fprintf(fp, "\n");
    
        int i;
        for (i = 0; i < nr_reg; i++) {
            // only write known registry item to file
            struct reg_item *pdef;
            for (pdef = reg_default; pdef->key1 && pdef->key2; pdef++) {
                if (strcmp(reg[i].key1, pdef->key1) == 0 && strcmp(reg[i].key2, pdef->key2) == 0) {
                    fprintf(fp, "%-40s%-15s%08X\n", reg[i].key1, reg[i].key2, reg[i].val);
                    break;
                }
            }
        }
        fclose(fp);
    } else {
        try_goto_desktop();
        MessageBoxW(game_hwnd, wstr_cantsavereg_text, wstr_cantsavereg_title, MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
    }
}
static void assign_reg(const char *key1, const char *key2, unsigned val)
{
    int i;
    for (i = 0; i < nr_reg; i++) {
        if (strcmp(reg[i].key1, key1) == 0 && strcmp(reg[i].key2, key2) == 0) {
            reg[i].val = val;
            goto done;
        }
    }
    int id = alloc_reg();
    reg[id].key1 = strdup(key1);
    reg[id].key2 = strdup(key2);
    reg[id].val = val;
done:
    save_reg();
}
static int query_reg(const char *key1, const char *key2, unsigned *pval)
{
    int i;
    for (i = 0; i < nr_reg; i++) {
        if (strcmp(reg[i].key1, key1) == 0 && strcmp(reg[i].key2, key2) == 0) {
            *pval = reg[i].val;
            return 1;
        }
    }
    return 0;
}

// no clean up functions, just let these strings leak





// registry functions
static void write_winreg(LPCSTR lpSubKey, LPCSTR lpValueName, DWORD Data)
{
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, lpSubKey, 0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, lpValueName, 0, REG_DWORD, (CONST BYTE *) &Data, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}
static void read_winreg(LPCSTR lpSubKey, LPCSTR lpValueName, LPDWORD lpData)
{
    HKEY hKey;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpSubKey, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, lpValueName, NULL, &dwType, (LPBYTE) lpData, &dwSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
        } else {
            *lpData = 0;
        }
    } else {
        *lpData = 0;
    }
}

/*
    0: patch disabled (read and write windows registry only)
    1: read file first, read windows registry when key not exists in file
       write both file and windows registry
    2: read and write file only
*/
#define REGREDIRECT_SYNC 1
#define REGREDIRECT_FILEONLY 2
static int regredirect_flag;


// patch functions
static bool save_dword(LPCSTR lpSubKey, LPCSTR lpValueName, DWORD Data) // WriteReg
{
    // write file
    assign_reg(lpSubKey, lpValueName, Data);
    if (regredirect_flag == REGREDIRECT_SYNC) {
        // always write windows registry is sync is enabled
        write_winreg(lpSubKey, lpValueName, Data);
    }
    return true;
}

static bool query_dword(LPCSTR lpSubKey, LPCSTR lpValueName, LPDWORD lpData) // CheckReg
{
    // read file
    int ret = query_reg(lpSubKey, lpValueName, (unsigned *) lpData);
    if (regredirect_flag == REGREDIRECT_SYNC) {
        if (ret) {
            // if we read data from file
            // sync data to windows registry
            write_winreg(lpSubKey, lpValueName, *lpData);
        } else {
            // if there is no data in file
            // try read registry and sync data to file
            read_winreg(lpSubKey, lpValueName, lpData);
            assign_reg(lpSubKey, lpValueName, *lpData);
        }
    } else {
        if (!ret) *lpData = 0;
    }
    return true;
}




MAKE_PATCHSET(regredirect)
{
    regredirect_flag = flag;
    if (regredirect_flag != REGREDIRECT_SYNC && regredirect_flag != REGREDIRECT_FILEONLY) {
        fail("unknown regredirect_flag %d.", regredirect_flag);
    }
    
    load_reg();
    
    struct reg_item *pdef;
    for (pdef = reg_default; pdef->key1 && pdef->key2; pdef++) {
        DWORD tmp;
        query_dword(pdef->key1, pdef->key2, &tmp);
    }
    
    unsigned i;
    
    const unsigned char save_dword_func_magic[] = "\x55\x8B\xEC\x51\x53\x8B\x5D\x08\x56\x57\x33\xFF\x3B\xDF\x0F\x84";
    const unsigned save_dword_funcs[] = {
        0x00407E10, 0x0041DDD2, 0x00440039, 0x004546D5,
        0x0045A50A, 0x0045C9ED, 0x004A1460, 0x004A6012,
        0x00517C80,
    };
    for (i = 0; i < sizeof(save_dword_funcs) / sizeof(unsigned); i++) {
        check_code(save_dword_funcs[i], save_dword_func_magic, sizeof(save_dword_func_magic) - 1);
        make_jmp(save_dword_funcs[i], save_dword);
    }
    
    const unsigned char query_dword_func_magic[] = "\x55\x8B\xEC\x51\x8D\x45\x08\x50\x6A\x01\x6A\x00\xFF\x75\x08\x68";
    const unsigned query_dword_funcs[] = {
        0x00406C27, 0x0044017A, 0x0045CAA2, 0x0045DF84,
        0x004A1515, 0x00515FD0,
    };
    for (i = 0; i < sizeof(query_dword_funcs) / sizeof(unsigned); i++) {
        check_code(query_dword_funcs[i], query_dword_func_magic, sizeof(query_dword_func_magic) - 1);
        make_jmp(query_dword_funcs[i], query_dword);
    }
    
    add_atexit_hook(save_reg);
}
