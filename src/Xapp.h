/*
 * resource set indices for xapp
 *
 * created by ORCS 2.18
 */

/*
 * Number of Strings:        133
 * Number of Bitblks:        1
 * Number of Iconblks:       77
 * Number of Color Iconblks: 3
 * Number of Color Icons:    5
 * Number of Tedinfos:       2
 * Number of Free Strings:   0
 * Number of Free Images:    0
 * Number of Objects:        141
 * Number of Trees:          4
 * Number of Userblks:       0
 * Number of Images:         155
 * Total file size:          14986
 */

#ifdef RSC_NAME
#undef RSC_NAME
#endif
#ifndef __ALCYON__
#define RSC_NAME "xapp"
#endif
#ifdef RSC_ID
#undef RSC_ID
#endif
#ifdef xapp
#define RSC_ID xapp
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 133
#define NUM_FRSTR 0
#define NUM_UD 0
#define NUM_IMAGES 155
#define NUM_BB 1
#define NUM_FRIMG 0
#define NUM_IB 77
#define NUM_CIB 3
#define NUM_TI 2
#define NUM_OBS 141
#define NUM_TREE 4
#endif



#define MENU                               0 /* menu */
#define MENU_CLNT                          5 /* TITLE in tree MENU */
#define MENU_ABOUT                         8 /* STRING in tree MENU */
#define MENU_GWM                          17 /* STRING in tree MENU */
#define MENU_QUIT                         19 /* STRING in tree MENU */
#define MENU_CLNT_FACE                    20 /* BOX in tree MENU */
#define MENU_CLNT_FRST                    21 /* STRING in tree MENU */
#define MENU_CLNT_LAST                    50 /* STRING in tree MENU */

#define ABOUT                              1 /* form/dialog */
#define ABOUT_LOGO                         1 /* IMAGE in tree ABOUT */
#define ABOUT_OK                           3 /* BUTTON in tree ABOUT */
#define ABOUT_VERSN                        6 /* TEXT in tree ABOUT */
#define ABOUT_BUILD                        7 /* TEXT in tree ABOUT */

#define GLYPH                              2 /* form/dialog */
#define GLYPH_X_cursor                     1 /* ICON in tree GLYPH */
#define GLYPH_arrow                        2 /* ICON in tree GLYPH */
#define GLYPH_based_arrow_down             3 /* ICON in tree GLYPH */
#define GLYPH_based_arrow_up               4 /* ICON in tree GLYPH */
#define GLYPH_boat                         5 /* ICON in tree GLYPH */
#define GLYPH_bogosity                     6 /* ICON in tree GLYPH */
#define GLYPH_bottom_left_corner           7 /* ICON in tree GLYPH */
#define GLYPH_bottom_right_corner          8 /* ICON in tree GLYPH */
#define GLYPH_bottom_side                  9 /* ICON in tree GLYPH */
#define GLYPH_bottom_tee                  10 /* ICON in tree GLYPH */
#define GLYPH_box_spiral                  11 /* ICON in tree GLYPH */
#define GLYPH_center_ptr                  12 /* ICON in tree GLYPH */
#define GLYPH_circle                      13 /* ICON in tree GLYPH */
#define GLYPH_clock                       14 /* ICON in tree GLYPH */
#define GLYPH_coffee_mug                  15 /* ICON in tree GLYPH */
#define GLYPH_cross                       16 /* ICON in tree GLYPH */
#define GLYPH_cross_reverse               17 /* ICON in tree GLYPH */
#define GLYPH_crosshair                   18 /* ICON in tree GLYPH */
#define GLYPH_diamond_cross               19 /* ICON in tree GLYPH */
#define GLYPH_dot                         20 /* ICON in tree GLYPH */
#define GLYPH_dotbox                      21 /* ICON in tree GLYPH */
#define GLYPH_double_arrow                22 /* ICON in tree GLYPH */
#define GLYPH_draft_large                 23 /* ICON in tree GLYPH */
#define GLYPH_draft_small                 24 /* ICON in tree GLYPH */
#define GLYPH_draped_box                  25 /* ICON in tree GLYPH */
#define GLYPH_exchange                    26 /* ICON in tree GLYPH */
#define GLYPH_fleur                       27 /* ICON in tree GLYPH */
#define GLYPH_gobbler                     28 /* ICON in tree GLYPH */
#define GLYPH_gumby                       29 /* ICON in tree GLYPH */
#define GLYPH_hand1                       30 /* ICON in tree GLYPH */
#define GLYPH_hand2                       31 /* ICON in tree GLYPH */
#define GLYPH_heart                       32 /* ICON in tree GLYPH */
#define GLYPH_icon                        33 /* ICON in tree GLYPH */
#define GLYPH_iron_cross                  34 /* ICON in tree GLYPH */
#define GLYPH_left_ptr                    35 /* ICON in tree GLYPH */
#define GLYPH_left_side                   36 /* ICON in tree GLYPH */
#define GLYPH_left_tee                    37 /* ICON in tree GLYPH */
#define GLYPH_leftbutton                  38 /* ICON in tree GLYPH */
#define GLYPH_ll_angle                    39 /* ICON in tree GLYPH */
#define GLYPH_lr_angle                    40 /* ICON in tree GLYPH */
#define GLYPH_man                         41 /* ICON in tree GLYPH */
#define GLYPH_middlebutton                42 /* ICON in tree GLYPH */
#define GLYPH_mouse                       43 /* ICON in tree GLYPH */
#define GLYPH_pencil                      44 /* ICON in tree GLYPH */
#define GLYPH_pirate                      45 /* ICON in tree GLYPH */
#define GLYPH_plus                        46 /* ICON in tree GLYPH */
#define GLYPH_question_arrow              47 /* ICON in tree GLYPH */
#define GLYPH_right_ptr                   48 /* ICON in tree GLYPH */
#define GLYPH_right_side                  49 /* ICON in tree GLYPH */
#define GLYPH_right_tee                   50 /* ICON in tree GLYPH */
#define GLYPH_rightbutton                 51 /* ICON in tree GLYPH */
#define GLYPH_rtl_logo                    52 /* ICON in tree GLYPH */
#define GLYPH_sailboat                    53 /* ICON in tree GLYPH */
#define GLYPH_sb_down_arrow               54 /* ICON in tree GLYPH */
#define GLYPH_sb_h_double_arrow           55 /* ICON in tree GLYPH */
#define GLYPH_sb_left_arrow               56 /* ICON in tree GLYPH */
#define GLYPH_sb_right_arrow              57 /* ICON in tree GLYPH */
#define GLYPH_sb_up_arrow                 58 /* ICON in tree GLYPH */
#define GLYPH_sb_v_double_arrow           59 /* ICON in tree GLYPH */
#define GLYPH_shuttle                     60 /* ICON in tree GLYPH */
#define GLYPH_sizing                      61 /* ICON in tree GLYPH */
#define GLYPH_spider                      62 /* ICON in tree GLYPH */
#define GLYPH_spraycan                    63 /* ICON in tree GLYPH */
#define GLYPH_star                        64 /* ICON in tree GLYPH */
#define GLYPH_target                      65 /* ICON in tree GLYPH */
#define GLYPH_tcross                      66 /* ICON in tree GLYPH */
#define GLYPH_top_left_arrow              67 /* ICON in tree GLYPH */
#define GLYPH_top_left_corner             68 /* ICON in tree GLYPH */
#define GLYPH_top_right_corner            69 /* ICON in tree GLYPH */
#define GLYPH_top_side                    70 /* ICON in tree GLYPH */
#define GLYPH_top_tee                     71 /* ICON in tree GLYPH */
#define GLYPH_trek                        72 /* ICON in tree GLYPH */
#define GLYPH_ul_angle                    73 /* ICON in tree GLYPH */
#define GLYPH_umbrella                    74 /* ICON in tree GLYPH */
#define GLYPH_ur_angle                    75 /* ICON in tree GLYPH */
#define GLYPH_watch                       76 /* ICON in tree GLYPH */
#define GLYPH_xterm                       77 /* ICON in tree GLYPH */

#define ICONS                              3 /* form/dialog */
#define ICONS_BIG_X                        1 /* CICON in tree ICONS */
#define ICONS_SMALL_X                      2 /* CICON in tree ICONS */
#define ICONS_NAME_X                       3 /* CICON in tree ICONS */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD xapp_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD xapp_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD xapp_rsc_free(void);
#endif
