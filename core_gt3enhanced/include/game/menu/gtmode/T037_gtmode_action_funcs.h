/* T037_gtmode_action_funcs.h - the ExecuteFuncData actions of the GT-mode
 * menu (translation unit T037, unit u013), 0x00125B50..0x00126C30: their
 * prototypes only.
 *
 * Split from T037_gtmode_actions.h (which includes it where they were):
 * T028 (u005) puts action_set_garage_car into the arcade menu's table, and
 * retail T028 saw none of that header's classes (TumbleCar, T212's Car and
 * its OperatorHolder bases: no type names of them in its .rodata,
 * 0x0030BD38..0x0030BF30). */
#ifndef GAME_T037_GTMODE_ACTION_FUNCS_H
#define GAME_T037_GTMODE_ACTION_FUNCS_H

namespace gt2k_menu {
class PageLayout;                           /* T229 (pointers only) */
struct ButtonData;                          /* T244 */
}

/* The ExecuteFuncData actions ("<name>": action_<name>; "the shop car" is
 * MenuStatus::car, "the ridden car" GameStatus::getRunningCar(), "the
 * current list" GtmodeControl::getButtonList()). */
/* MenuStatus::setFocusCar(rec->name), the menu car spec of it (T232). */
void action_set_car_code(void *ctx, gt2k_menu::ButtonData *rec, int index,
                         gt2k_menu::PageLayout *layout);    /* 0x00125B50 */
/* MenuStatus::setEvent: rec->name, raceMode 4 / rec->name with its last
 * character + 1, raceMode 2 / rec->name, raceMode 3 / rec->name cut at its
 * last '_' (raceMode kept) / "". */
void action_set_race_code(void *ctx, gt2k_menu::ButtonData *rec, int index,
                          gt2k_menu::PageLayout *layout);   /* 0x00125BA0 */
void action_set_race_code_champ(void *ctx, gt2k_menu::ButtonData *rec, int index,
                                gt2k_menu::PageLayout *layout);     /* 0x00125BF8 */
void action_set_race_code_spot(void *ctx, gt2k_menu::ButtonData *rec, int index,
                               gt2k_menu::PageLayout *layout);      /* 0x00125C68 */
void action_set_race_code_info(void *ctx, gt2k_menu::ButtonData *rec, int index,
                               gt2k_menu::PageLayout *layout);      /* 0x00125CC0 */
void action_clear_race_code(void *ctx, gt2k_menu::ButtonData *rec, int index,
                            gt2k_menu::PageLayout *layout); /* 0x00125CF8 */
void action_clear_focus_car(void *ctx, gt2k_menu::ButtonData *rec, int index,
                            gt2k_menu::PageLayout *layout); /* 0x00125D30 MenuStatus::clearFocusCar() */
/* The garage car at the current list's cursor becomes the shop car and
 * MenuStatus::garageIndex / the same in the guest garage (the
 * GameStatusHolder's status, T004), garageIndex kept. */
void action_set_garage_car(void *ctx, gt2k_menu::ButtonData *rec, int index,
                           gt2k_menu::PageLayout *layout);  /* 0x00125D60 */
void action_set_garage_car_trade(void *ctx, gt2k_menu::ButtonData *rec, int index,
                                 gt2k_menu::PageLayout *layout);    /* 0x00125DF0 */
/* Buys the shop car in MenuStatus::color (menu sound 0 after 30 frames,
 * T202) / a copy of the guest garage's shop car; garageIndex 0. */
void action_buy_car(void *ctx, gt2k_menu::ButtonData *rec, int index,
                    gt2k_menu::PageLayout *layout);         /* 0x00125E68 */
void action_buy_guest_car(void *ctx, gt2k_menu::ButtonData *rec, int index,
                          gt2k_menu::PageLayout *layout);   /* 0x00125F08 */
void action_ride_car(void *ctx, gt2k_menu::ButtonData *rec, int index,
                     gt2k_menu::PageLayout *layout);        /* 0x00125F60 rides garage car garageIndex */
void action_sell_car(void *ctx, gt2k_menu::ButtonData *rec, int index,
                     gt2k_menu::PageLayout *layout);        /* 0x00125FA8 sells garage car garageIndex */
void action_set_focus_wheel(void *ctx, gt2k_menu::ButtonData *rec, int index,
                            gt2k_menu::PageLayout *layout); /* 0x00125FF0 MenuStatus::wheelIndex: the
                                               current list's cursor */
/* Fits garage wheel wheelIndex to the shop car (an unnamed one: the car's
 * own); menu sound 0xC after 30 frames. */
void action_change_wheel(void *ctx, gt2k_menu::ButtonData *rec, int index,
                         gt2k_menu::PageLayout *layout);    /* 0x00126048 */
void action_set_focus_wheel_code(void *ctx, gt2k_menu::ButtonData *rec, int index,
                                 gt2k_menu::PageLayout *layout);    /* 0x001260E8 MenuStatus::wheelName:
                                               rec->name cut at its last '_' */
void action_buy_wheel(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);       /* 0x00126120 buys MenuStatus::wheelName */
/* Fits MenuStatus::wheelName to the ridden car (sound 0xC after 30
 * frames) and shows it on MenuStatus::carItem. */
void action_fit_wheel(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);       /* 0x00126160 */
/* MenuStatus::nextMode: 1 arcade menu; go_race: raceMode (a day passes);
 * 5 preview; 6 licence test / 7 machine test (event rec->name), 8 free run
 * (event "ATAF0001", str84 the course rec->name), each a day; 0xB replay
 * theater, 0xC demonstration, 0xD logger, 0x1A load, 0x19 save, 0x18 file
 * operation, 9 option, 0xA setting, 0x1D trade, 0x1E save at home (both
 * handled by gtmode_main, T038).  go_sitemap / go_top: GtmodeControl::
 * mapRequest / rootRequest = 1. */
void action_go_arcade_menu(void *ctx, gt2k_menu::ButtonData *rec, int index,
                           gt2k_menu::PageLayout *layout);  /* 0x001261F0 */
void action_go_sitemap(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x00126220 */
void action_go_top(void *ctx, gt2k_menu::ButtonData *rec, int index,
                   gt2k_menu::PageLayout *layout);          /* 0x00126230 */
void action_go_race(void *ctx, gt2k_menu::ButtonData *rec, int index,
                    gt2k_menu::PageLayout *layout);         /* 0x00126240 */
void action_go_preview(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x00126280 */
void action_go_license(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x001262B0 */
void action_go_machine_test(void *ctx, gt2k_menu::ButtonData *rec, int index,
                            gt2k_menu::PageLayout *layout); /* 0x00126308 */
void action_go_free_run(void *ctx, gt2k_menu::ButtonData *rec, int index,
                        gt2k_menu::PageLayout *layout);     /* 0x00126360 */
void action_go_replay_theater(void *ctx, gt2k_menu::ButtonData *rec, int index,
                              gt2k_menu::PageLayout *layout);       /* 0x001263D0 */
void action_go_demonstration(void *ctx, gt2k_menu::ButtonData *rec, int index,
                             gt2k_menu::PageLayout *layout);        /* 0x00126400 */
void action_go_logger(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);       /* 0x00126430 */
void action_go_load_game_menu(void *ctx, gt2k_menu::ButtonData *rec, int index,
                              gt2k_menu::PageLayout *layout);       /* 0x00126460 */
void action_go_save_game_menu(void *ctx, gt2k_menu::ButtonData *rec, int index,
                              gt2k_menu::PageLayout *layout);       /* 0x00126490 */
void action_go_file_operation(void *ctx, gt2k_menu::ButtonData *rec, int index,
                              gt2k_menu::PageLayout *layout);       /* 0x001264C0 */
void action_go_option(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);       /* 0x001264F0 */
void action_go_setting(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x00126520 */
void action_go_trade(void *ctx, gt2k_menu::ButtonData *rec, int index,
                     gt2k_menu::PageLayout *layout);        /* 0x00126550 */
void action_go_save_in_home(void *ctx, gt2k_menu::ButtonData *rec, int index,
                            gt2k_menu::PageLayout *layout); /* 0x00126580 */
/* The tuning shop: MenuStatus::parts = the part rec->name (T203); buys it
 * for the ridden car (not fitted) / every tyre 0x3A..0x40 the car can have
 * and lacks / fits it (a front tyre 0x38..0x41 also its rear tyre
 * 0x43..0x4C) / buys and fits it (not the rear tyre); the last two keep the
 * car's power in GtmodeControl::prevPower first. */
void action_set_focus_parts(void *ctx, gt2k_menu::ButtonData *rec, int index,
                            gt2k_menu::PageLayout *layout); /* 0x001265B0 */
void action_buy_parts(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);       /* 0x001265F0 */
void action_buy_all_tires(void *ctx, gt2k_menu::ButtonData *rec, int index,
                          gt2k_menu::PageLayout *layout);   /* 0x00126680 */
void action_fit_parts(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);       /* 0x00126730 */
void action_buy_fit_parts(void *ctx, gt2k_menu::ButtonData *rec, int index,
                          gt2k_menu::PageLayout *layout);   /* 0x001268B8 */
/* GtmodeControl::sortType (in parentheses) and, for some, infoMode.set. */
void action_sort_color(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x00126968 (3) */
void action_sort_date(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);       /* 0x00126978 (0) */
void action_sort_distance(void *ctx, gt2k_menu::ButtonData *rec, int index,
                          gt2k_menu::PageLayout *layout);   /* 0x00126980 (5), infoMode 3 */
void action_sort_maker(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x001269A8 (2), infoMode 2 */
void action_sort_name(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);       /* 0x001269D0 (1) */
void action_sort_power(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x001269E0 (6), infoMode 0 */
void action_sort_price(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x00126A08 (4), infoMode 1 */
void action_sort_drive(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x00126A30 (7), infoMode 4 */
void action_sort_aspiration(void *ctx, gt2k_menu::ButtonData *rec, int index,
                            gt2k_menu::PageLayout *layout); /* 0x00126A58 (8), infoMode 5 */
/* A TumbleCar under the PageManager's root, pays the T435 price
 * "car_wash", the shop car's dirt 0 / starts GtmodeControl::oilCylinder (if
 * any), pays "oil_change", changes the shop car's oil. */
void action_wash_car(void *ctx, gt2k_menu::ButtonData *rec, int index,
                     gt2k_menu::PageLayout *layout);        /* 0x00126A80 */
void action_oil_change(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);      /* 0x00126B98 */
void action_set_focus_replay_theater(void *ctx, gt2k_menu::ButtonData *rec, int index,
                                     gt2k_menu::PageLayout *layout);    /* 0x00126C18 GtmodeControl::mode
                                               = 2 */
void action_check_ride_car(void *ctx, gt2k_menu::ButtonData *rec, int index,
                           gt2k_menu::PageLayout *layout);  /* 0x00126C28 empty (T045's check of the
                                               name does the work) */

#endif /* GAME_T037_GTMODE_ACTION_FUNCS_H */
