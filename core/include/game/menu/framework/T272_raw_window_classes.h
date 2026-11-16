/* T272_raw_window_classes.h - the class registry entries of the 16 Raw*
 * window classes (translation unit T272, unit u073), 0x001D2FD0..0x001D3E40.
 *
 * Per class, in alphabetical order, from one macro of the original: the class
 * name (slot 8 of the class, `const char *virtual_8()'), the layout factory
 * (its eight words packed into a RawWindowArgs), the registry's create
 * function (operator new + the constructor, if canCreate(args)) and
 * canCreate(args) (1, the arguments unused).  Those members are declared
 * with their classes: RawCheckBox (T275), RawLabel (T273), RawList* /
 * RawListItem* (T263), RawScrollBar (T274), RawWindow (T264).  Then the
 * static registrars of all 16 (T265's RawWindowRegister, which links them on
 * the registry list).
 *
 * The .sdata per class (24 bytes, in this order, defined in T272 before the
 * class's functions): the name pointer, the RawWindowClass record {name,
 * create}, the registrar (a public object with a constructor); RawList's
 * name string is in .sdata too (8 bytes under -G8), before its pointer.
 * Only T272 uses them. */
#ifndef GAME_T272_RAW_WINDOW_CLASSES_H
#define GAME_T272_RAW_WINDOW_CLASSES_H

#include "game/game.h"
#include "game/menu/framework/T265_raw_window_register.h"  /* gt2k_menu::RawWindowClass, RawWindowRegister (complete:
                                               the registrars are reached $gp-relative) */

/* 0x001D3C50 (.dtors 0x0028F23C) / 0x001D3D28 (.ctors 0x0028F160): egcs's
 * static destructor / initialiser of the 16 registrars
 * (_GLOBAL_$D$g_00352760 / _GLOBAL_$I$g_00352760, T272). */

/* The registry entries, per class: name, record, registrar. */
extern const char *g_00352760;                  /* 0x00352760 gp-0x5290 "RawCheckBox" */
extern gt2k_menu::RawWindowClass    g_00352768; /* 0x00352768 gp-0x5288 {name, RawCheckBox::create} */
extern gt2k_menu::RawWindowRegister g_00352770; /* 0x00352770 gp-0x5280 its registrar */
extern const char *g_00352778;                  /* 0x00352778 gp-0x5278 "RawLabel" */
extern gt2k_menu::RawWindowClass    g_00352780; /* 0x00352780 gp-0x5270 {name, RawLabel::create} */
extern gt2k_menu::RawWindowRegister g_00352788; /* 0x00352788 gp-0x5268 its registrar */
/* 0x00352790 gp-0x5260: "RawList", the string literal itself (.sdata) */
extern const char *g_00352798;                  /* 0x00352798 gp-0x5258 "RawList" (0x00352790) */
extern gt2k_menu::RawWindowClass    g_003527A0; /* 0x003527A0 gp-0x5250 {name, RawList::create} */
extern gt2k_menu::RawWindowRegister g_003527A8; /* 0x003527A8 gp-0x5248 its registrar */
extern const char *g_003527B0;                  /* 0x003527B0 gp-0x5240 "RawListFixed" */
extern gt2k_menu::RawWindowClass    g_003527B8; /* 0x003527B8 gp-0x5238 {name, RawListFixed::create} */
extern gt2k_menu::RawWindowRegister g_003527C0; /* 0x003527C0 gp-0x5230 its registrar */
extern const char *g_003527C8;                  /* 0x003527C8 gp-0x5228 "RawListH" */
extern gt2k_menu::RawWindowClass    g_003527D0; /* 0x003527D0 gp-0x5220 {name, RawListH::create} */
extern gt2k_menu::RawWindowRegister g_003527D8; /* 0x003527D8 gp-0x5218 its registrar */
extern const char *g_003527E0;                  /* 0x003527E0 gp-0x5210 "RawListHFixed" */
extern gt2k_menu::RawWindowClass    g_003527E8; /* 0x003527E8 gp-0x5208 {name, RawListHFixed::create} */
extern gt2k_menu::RawWindowRegister g_003527F0; /* 0x003527F0 gp-0x5200 its registrar */
extern const char *g_003527F8;                  /* 0x003527F8 gp-0x51F8 "RawListHPos" */
extern gt2k_menu::RawWindowClass    g_00352800; /* 0x00352800 gp-0x51F0 {name, RawListHPos::create} */
extern gt2k_menu::RawWindowRegister g_00352808; /* 0x00352808 gp-0x51E8 its registrar */
extern const char *g_00352810;                  /* 0x00352810 gp-0x51E0 "RawListHSize" */
extern gt2k_menu::RawWindowClass    g_00352818; /* 0x00352818 gp-0x51D8 {name, RawListHSize::create} */
extern gt2k_menu::RawWindowRegister g_00352820; /* 0x00352820 gp-0x51D0 its registrar */
extern const char *g_00352828;                  /* 0x00352828 gp-0x51C8 "RawListItem" */
extern gt2k_menu::RawWindowClass    g_00352830; /* 0x00352830 gp-0x51C0 {name, RawListItem::create} */
extern gt2k_menu::RawWindowRegister g_00352838; /* 0x00352838 gp-0x51B8 its registrar */
extern const char *g_00352840;                  /* 0x00352840 gp-0x51B0 "RawListItemFixed" */
extern gt2k_menu::RawWindowClass    g_00352848; /* 0x00352848 gp-0x51A8 {name, RawListItemFixed::create} */
extern gt2k_menu::RawWindowRegister g_00352850; /* 0x00352850 gp-0x51A0 its registrar */
extern const char *g_00352858;                  /* 0x00352858 gp-0x5198 "RawListV" */
extern gt2k_menu::RawWindowClass    g_00352860; /* 0x00352860 gp-0x5190 {name, RawListV::create} */
extern gt2k_menu::RawWindowRegister g_00352868; /* 0x00352868 gp-0x5188 its registrar */
extern const char *g_00352870;                  /* 0x00352870 gp-0x5180 "RawListVFixed" */
extern gt2k_menu::RawWindowClass    g_00352878; /* 0x00352878 gp-0x5178 {name, RawListVFixed::create} */
extern gt2k_menu::RawWindowRegister g_00352880; /* 0x00352880 gp-0x5170 its registrar */
extern const char *g_00352888;                  /* 0x00352888 gp-0x5168 "RawListVPos" */
extern gt2k_menu::RawWindowClass    g_00352890; /* 0x00352890 gp-0x5160 {name, RawListVPos::create} */
extern gt2k_menu::RawWindowRegister g_00352898; /* 0x00352898 gp-0x5158 its registrar */
extern const char *g_003528A0;                  /* 0x003528A0 gp-0x5150 "RawListVSize" */
extern gt2k_menu::RawWindowClass    g_003528A8; /* 0x003528A8 gp-0x5148 {name, RawListVSize::create} */
extern gt2k_menu::RawWindowRegister g_003528B0; /* 0x003528B0 gp-0x5140 its registrar */
extern const char *g_003528B8;                  /* 0x003528B8 gp-0x5138 "RawScrollBar" */
extern gt2k_menu::RawWindowClass    g_003528C0; /* 0x003528C0 gp-0x5130 {name, RawScrollBar::create} */
extern gt2k_menu::RawWindowRegister g_003528C8; /* 0x003528C8 gp-0x5128 its registrar */
extern const char *g_003528D0;                  /* 0x003528D0 gp-0x5120 "RawWindow" */
extern gt2k_menu::RawWindowClass    g_003528D8; /* 0x003528D8 gp-0x5118 {name, RawWindow::create} */
extern gt2k_menu::RawWindowRegister g_003528E0; /* 0x003528E0 gp-0x5110 its registrar */

#endif /* GAME_T272_RAW_WINDOW_CLASSES_H */
