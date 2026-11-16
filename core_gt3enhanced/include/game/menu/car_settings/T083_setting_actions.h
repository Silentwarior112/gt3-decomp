/* T083_setting_actions.h - the ExecuteFuncData table of the car setting
 * screen (translation unit T083, unit u036), 0x00166F98..0x001677A0.
 *
 * g_002ACCC8 (77 entries sorted by name, count g_00351978; filled by
 * T083_static_init) maps the exec
 * names of the setting pages' buttons to their actions.  SettingControl::
 * begin (T080, 0x0015CA98) gives it to its PageManager (execute table
 * +0xB18, ctx the SettingControl); PageManager::callExecuteFunc (T211) calls
 * func(ctx, rec, index, layout) for a decided button.  The entries are
 * "commit_slider", "go_load", "go_save", "retrieve_slider" and 73
 * "set_<category>_<part>", all setting_set_part.  The control works on its
 * own copy of the setting car (SettingControl +0xE40); commit_slider and the
 * set_* actions copy it back to GameStatus's MenuStatus::settingCar
 * (GarageCar::assign), retrieve_slider the other way.
 *
 * The actions take the ExecuteFuncData signature (the first pass had
 * (SettingControl *ctl, const PageItem *item)): ctx is the SettingControl,
 * u036's PageItem is T244's ButtonData (its action name at +0xC0 is
 * ButtonData::exec); index and layout are not used.  The first pass's
 * SettingActionEntry (u036) is T211's ExecuteFuncData. */
#ifndef GAME_T083_SETTING_ACTIONS_H
#define GAME_T083_SETTING_ACTIONS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ExecuteFuncData (g_002ACCC8) */

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
class PageLayout;                           /* T229 */
}

/* The first pass's name of the record (u036). */
typedef gt2k_menu::ExecuteFuncData SettingActionEntry;  /* {name, func}, 8 bytes */

/* "commit_slider": the working copy becomes the setting car (the
 * SettingControl's changed = 1); then g_003526B0 = 1, also without a
 * setting car. */
void setting_commit_slider(void *ctx, gt2k_menu::ButtonData *rec, int index,
                           gt2k_menu::PageLayout *layout);      /* 0x00166F98 */
/* "retrieve_slider": the working copy is reset from the setting car. */
void setting_retrieve_slider(void *ctx, gt2k_menu::ButtonData *rec, int index,
                             gt2k_menu::PageLayout *layout);    /* 0x00166FF0 */
/* Every "set_<category>_<part>": the part id of rec->exec (T203
 * 0x001B2C58); if the working copy owns it (CarInformation::hasPart) it is
 * fitted (CarEquipments::fitPart), the copy committed to the setting car and
 * menu sound 7 played; else sound 4.  Nothing without a setting car. */
void setting_set_part(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);           /* 0x00167038 */
/* "go_load" / "go_save": the SettingControl's request = 1 / 2 (its loop
 * loads / saves the settings on the memory card). */
void setting_go_load(void *ctx, gt2k_menu::ButtonData *rec, int index,
                     gt2k_menu::PageLayout *layout);            /* 0x001670F8 */
void setting_go_save(void *ctx, gt2k_menu::ButtonData *rec, int index,
                     gt2k_menu::PageLayout *layout);            /* 0x00167108 */

/* T083 .data 0x002ACCC8..0x002ACF30: {"commit_slider",
 * setting_commit_slider} ..  Filled by T083_static_init (.ctors entry
 * 0x0028F0A8; compiler-generated in the original). */
extern gt2k_menu::ExecuteFuncData g_002ACCC8[77];  /* 0x002ACCC8 */
/* Its entry count (T083 .sdata; SettingControl::begin, T080). */
extern const int g_00351978;                /* 0x00351978 gp-0x6088 (=0x4D) */
void T083_static_init(void);                /* 0x00167118 */

#endif /* GAME_T083_SETTING_ACTIONS_H */
