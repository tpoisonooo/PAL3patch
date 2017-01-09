#ifndef PAL3PATCH_PATCH_COMMON_H
#define PAL3PATCH_PATCH_COMMON_H

#define MAKE_PATCHSET_NAME(name) CONCAT(patchset_, name)
#define GET_PATCHSET_FLAG(name) (get_int_from_configfile(TOSTR(name)))
#define GET_PATCHSET_CONFIG_STR(name) (get_string_from_configfile(TOSTR(name)))

// patchset based on integer config
#define MAKE_PATCHSET(name) void MAKE_PATCHSET_NAME(name)(int flag)
#define INIT_PATCHSET(name) \
    ({ \
        int __flag = GET_PATCHSET_FLAG(name); \
        if (__flag) MAKE_PATCHSET_NAME(name)(__flag); \
        __flag; \
    })

// patchset based on string config
#define MAKE_PATCHSET_STRCFG(name) void MAKE_PATCHSET_NAME(name)(const char *cfgstr)
#define INIT_PATCHSET_STRCFG(name) MAKE_PATCHSET_NAME(name)(GET_PATCHSET_CONFIG_STR(name))


#define GAME_WIDTH_ORG 800
#define GAME_HEIGHT_ORG 600

// all patchs
MAKE_PATCHSET(testcombat);
MAKE_PATCHSET(cdpatch);
MAKE_PATCHSET(regredirect);
MAKE_PATCHSET(disablekbdhook);
MAKE_PATCHSET(depcompatible);
MAKE_PATCHSET(setlocale);
MAKE_PATCHSET(dpiawareness);
MAKE_PATCHSET(timerresolution);
MAKE_PATCHSET(fixmemfree);
MAKE_PATCHSET(nocpk);
MAKE_PATCHSET(showfps);
MAKE_PATCHSET(console);
MAKE_PATCHSET(relativetimer);

MAKE_PATCHSET(graphicspatch);
    extern int game_width, game_height;
    extern fRECT game_frect, game_frect_43, game_frect_original;
    #define MAX_CUSTOM_GAME_FRECT 4 // NOTE: pay attention to enum uiwnd_rect_type
    extern fRECT game_frect_custom[MAX_CUSTOM_GAME_FRECT];
    extern double game_scalefactor;
    enum scalefactor_index_name {
        // all scalefactor should be set when stage2 is complete
        // uireplacefont will read them after creating D3D device
        
        // these 2 values is initialized by graphicspatch
        SF_IDENTITY, // equals to 1.0
        SF_GAMEFACTOR,
        
        // these values need initinialize by their own init functions
        SF_UI,
        SF_SOFTCURSOR,
        SF_COMBAT,
        
        // NOTE: if you want to modify this enum, pay attention to the size limit in struct uiwnd_ptag
        SCALEFACTOR_COUNT // EOF
    };
    extern double scalefactor_table[SCALEFACTOR_COUNT];
    extern double str2scalefactor(const char *str);
    
    extern void try_goto_desktop();
    
    MAKE_PATCHSET(fixfov);
    MAKE_PATCHSET(fixortho);
        extern float orthofactor;
    MAKE_PATCHSET(nolockablebackbuffer);
    MAKE_PATCHSET(reduceinputlatency);
    MAKE_PATCHSET(fixreset);
    MAKE_PATCHSET(fixui);
        struct fixui_state {
            fRECT src_frect, dst_frect;
            int lr_method, tb_method;
            double len_factor;
            struct fixui_state *prev;
            int no_cursor_virt;
        };
        extern struct fixui_state *fs;

        #define softcursor_scalefactor (scalefactor_table[SF_SOFTCURSOR])
        #define ui_scalefactor (scalefactor_table[SF_UI])
        extern void fixui_update_gamestate();
        
        enum uiwnd_rect_type { // PTR = pos tag rect
            PTR_GAMERECT,          // game_frect
            PTR_GAMERECT_43,       // game_frect_43
            PTR_GAMERECT_ORIGINAL, // game_frect_original
            PTR_GAMERECT_UIAUTO,   // game_frect_ui_auto
            PTR_GAMERECT_CUSTOM0,  // game_frect_custom[0]
            //PTR_GAMERECT_CUSTOM0+1,  // game_frect_custom[1]
            //PTR_GAMERECT_CUSTOM0+2,  // game_frect_custom[2]
            //PTR_GAMERECT_CUSTOM0+3,  // game_frect_custom[3]
            // NOTE: PTR_GAMERECT_CUSTOM0 must be last one, since it occupy MAX_CUSTOM_GAME_FRECT values
            // NOTE: if you want to modify this enum, pay attention to the size limit in struct uiwnd_ptag
        };
        int parse_uiwnd_rect_type(const char *str);
        
        struct fixui_state *fixui_newstate(fRECT *src_frect, fRECT *dst_frect, int lr_method, int tb_method, double len_factor);
        struct fixui_state *fixui_dupstate();
        extern void fixui_pushstate_node(struct fixui_state *cur);
        #define fixui_pushstate(src_frect, dst_frect, lr_method, tb_method, len_factor) \
            fixui_pushstate_node(fixui_newstate(src_frect, dst_frect, lr_method, tb_method, len_factor))
        extern void fixui_popstate();
        #define fixui_pushidentity() fixui_pushstate(&game_frect, &game_frect, TR_LOW, TR_LOW, 1.0)
        
        #define UIWND_PTAG_MAGIC 162u
        struct uiwnd_ptag {
            unsigned scalefactor_index : 3;
            unsigned self_srcrect_type : 3;
            unsigned self_dstrect_type : 3;
            unsigned self_lr_method : 3;
            unsigned self_tb_method : 3;
            unsigned no_cursor_virt : 1; // if set to 1, will disable cursor transform for this UIWnd
            unsigned self_only_ptag : 1; // if set to 1, this ptag apply to this UIWnd only, will not affect it's childrens
            unsigned in_use : 1;
            unsigned enabled : 1;
            unsigned magic : 13;
        };
        #define M_PWND(addr) TOPTR(M_DWORD(addr))
        #define PWND TOPTR
        #define MAKE_PTAG(sf_idx, src_type, dst_type, lr, tb) \
            ((struct uiwnd_ptag) { \
                .scalefactor_index = (sf_idx), \
                .self_srcrect_type = (src_type), \
                .self_dstrect_type = (dst_type), \
                .self_lr_method = (lr), \
                .self_tb_method = (tb), \
                .no_cursor_virt = 0, \
                .self_only_ptag = 0, \
                .in_use = 0, \
                .enabled = 1, \
                .magic = UIWND_PTAG_MAGIC, \
            })
        extern void set_uiwnd_ptag(struct UIWnd *this, struct uiwnd_ptag ptag);
        extern fRECT *get_ptag_frect(int rect_type);
        
        extern void fixui_adjust_fRECT(fRECT *out_frect, fRECT *frect);
        extern void fixui_adjust_gbUIQuad(struct gbUIQuad *out_uiquad, const struct gbUIQuad *uiquad);
        extern void fixui_adjust_RECT(RECT *out_rect, const RECT *rect);
        extern void fixui_adjust_fPOINT(fPOINT *out_fpoint, const fPOINT *fpoint);
        extern void fixui_adjust_POINT(POINT *out_point, const POINT *point);
        
        extern void fixui_scale_fPOINT(fPOINT *out_fpoint, fPOINT *fpoint, fRECT *src_frect, fRECT *dst_frect);
        extern void fixui_scale_POINT_round(POINT *out_point, POINT *point, fRECT *src_frect, fRECT *dst_frect);
        
        extern void push_ptag_state(struct UIWnd *pwnd);
        extern void pop_ptag_state(struct UIWnd *pwnd);
        #define MAKE_UIWND_RENDER_WRAPPER(render_wrapper_name, original_render_address) \
            void __fastcall render_wrapper_name(struct UIWnd *this, int dummy) \
            { \
                push_ptag_state(this); \
                THISCALL_WRAPPER(MAKE_THISCALL_FUNCPTR(original_render_address, void, struct UIWnd *), this); \
                pop_ptag_state(this); \
            }
        #define MAKE_UIWND_UPDATE_WRAPPER(update_wrapper_name, original_wrapper_address) \
            int __fastcall update_wrapper_name(struct UIWnd *this, int dummy, float deltatime, int haveinput) \
            { \
                push_ptag_state(this); \
                int ret = THISCALL_WRAPPER(MAKE_THISCALL_FUNCPTR(original_wrapper_address, int, struct UIWnd *, float, int), this, deltatime, haveinput); \
                pop_ptag_state(this); \
                return ret; \
            }

        MAKE_PATCHSET(uireplacefont);
        MAKE_PATCHSET(fixloadingfrm);
        MAKE_PATCHSET(fixcombatui);
            #define cb_scalefactor (scalefactor_table[SF_COMBAT])
        MAKE_PATCHSET(fixroledialog);
        MAKE_PATCHSET(fixgameover);
        MAKE_PATCHSET(fix3dctrl);
        MAKE_PATCHSET(fixlineupui);
        MAKE_PATCHSET(fixuistaticex);

    MAKE_PATCHSET(replacetexture);

#endif
