/* T161_race_tachometer.h - the tachometers of the race display (translation
 * unit T161, unit u056), 0x0019D278..0x0019DE30.
 *
 * RaceRoundTachometerBase is a RaceMeterBase that holds a RaceRoundMeter
 * (T160, the dial, at +0x20) and forwards the display object calls to it;
 * it adds the dial image prefix and the transmission mark (one character of
 * the "meter#1" font next to the dial centre: 0x3E when transmission is 0,
 * else 0x3D).  RaceOnboardTachometer ("rev_" dials) and RaceRemoteTachometer
 * ("revb_") only set it up differently.  RaceBattleTachometer does the same
 * around a RaceBarMeter ("meter_mini").  RaceDisplay (T154) holds one of
 * each (+0xE7C, +0x3F3C, +0x6FFC) and points activeTacho at one.
 *
 * T161 also has out-of-line copies of RaceMeterBase's inline members
 * (0x0019DDA0..0x0019DDE0, never called): T160_race_meters.h declares them.
 *
 * Inline in the original: the four destructors (RaceDisplay's destructor
 * 0x00191300 expands them, the derived ones expand ~RaceRoundTachometerBase;
 * none has a caller) and every method but the constructors, setScale,
 * setPrefix and drawMeter (out-of-line copies emitted at the end of the TU,
 * class by class, with the __tf functions).  All of them are written inline
 * in their classes below, in the reverse of the order T161 emits them (the
 * u056 data pass: T161's frame info, i.e. its function order, is retail's
 * only so).  No other unit calls them but the two setTransmission
 * (RaceDisplay::setupCarMeters expands them); the virtual ones are called
 * through the vtables (written in T161, the key function is drawMeter).
 *
 * T154_race_display.h includes it after the meters' header T160 (RaceDisplay
 * holds the tachometers by value); T161's source includes it first
 * (retail's T161 type names: RaceMeterBase's and T161's own last, see
 * T160_race_meters.h).
 */
#ifndef GAME_T161_RACE_TACHOMETER_H
#define GAME_T161_RACE_TACHOMETER_H

#include "game/game.h"
#include "game/race/display/T160_race_meters.h"              /* RaceRoundMeter, RaceBarMeter, base RaceMeterBase */

struct Tex1Image;                               /* G001 (pointers only) */
struct ImagePack;                               /* T506 */
class GSBuffer;                                 /* T514 (references only) */
class type_info;                                /* T548 */

/* The typeinfo functions, declared before the classes for GCC 14 only:
 * egcs generates them itself (in T161, with the vtables).  An earlier
 * extern "C" declaration is the one egcs uses, and then it never defines
 * the function; a later one it rejects (u053 / u056 matching passes).
 * Each builds an __si_type_info on its base's node. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf20RaceBattleTachometer(void);      /* 0x0019DA18 base RaceMeterBase */
const type_info &__tf20RaceRemoteTachometer(void);      /* 0x0019DAC8 base RaceRoundTachometerBase */
const type_info &__tf21RaceOnboardTachometer(void);     /* 0x0019DB78 base RaceRoundTachometerBase */
const type_info &__tf23RaceRoundTachometerBase(void);   /* 0x0019DD50 base RaceMeterBase */
}
#endif

/* vtable 0x0032F778 (13 slots: [0] [5]..[12] its own; index.txt names [5]
 * and [12] RaceRemoteTachometer::drawMeter / _12, they are this class's:
 * all three vtables hold them).  typeinfo __tf23RaceRoundTachometerBase
 * 0x0019DD50 (si, base RaceMeterBase; storage 0x004B2340).  vptr +0x14
 * (RaceDisplayObjectBase's).  Size 0x30C0 (RaceDisplay+0xE7C .. +0x3F3C). */
class RaceRoundTachometerBase : public RaceMeterBase {
public:
    /* RaceMeterBase(), the dial's constructor, setPrefix(""), the dial's
     * scale colours 0x80808068 / 0x800000F0, panel colour 0x80A0A088,
     * enabled. */
    RaceRoundTachometerBase();                  /* 0x0019D278 */
    /* The dial's panel (RaceRoundMeterBase::drawMeter, a direct call), then
     * the transmission mark at the dial centre + (markX, markY). */
    virtual void drawMeter(GSBuffer &buf);      /* [ 5] +0x34 0x0019D428 */
    /* The first entry of the dial table g_0032F5B8 ({maxRpm, step, suffix}:
     * 6000 .. 12000 rpm by 500, 18000 by 1000) the rev limit fits in: the
     * dial's marks (0.03 long, 0.85 filled, the entry's step) and image
     * prefix + suffix ("rev_08"); meter.scale = maxRpm in any case.
     * Caller: RaceDisplay::setupCarMeters. */
    virtual void setScale(f32 maxRpm);          /* [12] +0x6C 0x0019D350 */
    void setPrefix(const char *prefix);         /* 0x0019D408 ? name: strcpy (the three constructors) */

    /* The inline members (see above), declared in the reverse of the order
     * T161 emits them (0x0019DBC8..0x0019DCF0, then __tf): egcs writes a
     * class's in-class inline members last-declared first (u056 data
     * pass: T161's frame info). */
    /* The vptr, ~RaceRoundMeter on the dial (flags 2), RaceMeterBase's
     * vptr, ~RaceDisplayObjectBase. */
    virtual ~RaceRoundTachometerBase() {}       /* [ 0] +0x0C 0x0019DCF0 (out-of-line copy) */
    /* The dial's (RaceRoundMeterBase::render_2nd, direct) when this object
     * (not the dial) is enabled. */
    virtual void render_2nd() {                 /* [10] +0x5C 0x0019DCC8 */
        if (enabled)
            meter.RaceRoundMeterBase::render_2nd();
    }
    virtual void set(f32 rpm) { meter.value = rpm; }    /* [11] +0x64 0x0019DCC0 */
    void setRedZone(f32 rpm) { meter.redZone = rpm; }   /* 0x0019DCB8 ? name */
    /* ? name.  RaceDisplay::setupCarMeters (T154) expands it on its two
     * round tachometers (stores through each one's address, the value
     * loaded as a word: an int parameter). */
    void setTransmission(int t) { transmission = t; }   /* 0x0019DCB0 */
    /* The display object calls go to the dial (setRegion:
     * RaceDisplayObjectBase::setRegion expanded, the dial's setLocation /
     * setSize through its vtable). */
    virtual void setLocation(int x, int y) { meter.setLocation(x, y); }     /* [ 6] +0x3C 0x0019DCA0 */
    virtual void setSize(int w, int h) { meter.setSize(w, h); }             /* [ 7] +0x44 0x0019DC90 */
    virtual void setAlignment(u8 alignX, u8 alignY) {   /* [ 8] +0x4C 0x0019DC80 */
        meter.setAlignment(alignX, alignY);
    }
    virtual void setRegion(int x, int y, int w, int h) {    /* [ 9] +0x54 0x0019DC18 */
        meter.setRegion(x, y, w, h);
    }
    /* ? name.  No callers (RaceDisplay::setupCarMeters calls the dial's
     * RaceRoundMeter::updateScale, 0x0019CA80, directly). */
    void updateScale() { meter.updateScale(); }         /* 0x0019DBF8 */
    /* meter.imagePack (+0x20; index.txt: EngineSound::setMuffler, a code
     * match: one `sw $a1, 0x20($a0)'). */
    void setImagePack(ImagePack *pack) { meter.imagePack = pack; }  /* 0x0019DBF0 */
    void setImage(Tex1Image *img) { meter.image = img; }    /* 0x0019DBE8 ? name: meter.image (no callers) */
    /* ? name.  No callers.  The dial's named image (RaceDisplayObjectBase::
     * setImage, 0x001965A8). */
    void setImage(const char *name) { meter.setImage(name); }   /* 0x0019DBC8 */

    RaceRoundMeter meter;                       /* 0x0020 the dial */
    char prefix[0x20];                          /* 0x309C the dial image name prefix ("rev_", "revb_") */
    /* RaceEntryCar::transmission (1: manual), copied by RaceDisplay::
     * setupCarMeters. */
    u8   transmission;                          /* 0x30BC 0: the 0x3E mark, else 0x3D (lbu) */
    s8   markX;                                 /* 0x30BD the mark's offset from the dial centre */
    s8   markY;                                 /* 0x30BE */
    u8   unk_30BF[1];                           /* 0x30BF */
};
EGCS_SIZE(RaceRoundTachometerBase, 0x30C0);

/* vtable 0x0032F708 (13 slots: [0] its own, [5]..[12]
 * RaceRoundTachometerBase's).  typeinfo __tf21RaceOnboardTachometer
 * 0x0019DB78 (si, storage 0x004B1270).  The "rev_" dials: centre (0, 23),
 * radius 75, angles 72.5..287.5, needle -0.2..0.95, mark at (-10, -15).
 * Size 0x30C0 (RaceDisplay+0xE7C). */
class RaceOnboardTachometer : public RaceRoundTachometerBase {
public:
    RaceOnboardTachometer();                    /* 0x0019D510 */
    virtual ~RaceOnboardTachometer() {}         /* [0] +0x0C 0x0019DB18 (out-of-line copy) */
};
EGCS_SIZE(RaceOnboardTachometer, 0x30C0);

/* vtable 0x0032F698 (13 slots: [0] its own).  typeinfo
 * __tf20RaceRemoteTachometer 0x0019DAC8 (si, storage 0x004B11E0).  The
 * "revb_" dials: centre (3, 4), radius 73, angles 29.85..238.34, needle
 * 0.38..0.9, mark at (-2, 28).  Size 0x30C0 (RaceDisplay+0x3F3C). */
class RaceRemoteTachometer : public RaceRoundTachometerBase {
public:
    RaceRemoteTachometer();                     /* 0x0019D620 */
    virtual ~RaceRemoteTachometer() {}          /* [0] +0x0C 0x0019DA68 (out-of-line copy) */
};
EGCS_SIZE(RaceRemoteTachometer, 0x30C0);

/* vtable 0x0032F628 (13 slots: [0] [5]..[9] [11] [12] its own, [10]
 * RaceMeterBase's).  typeinfo __tf20RaceBattleTachometer 0x0019DA18 (si,
 * base RaceMeterBase; storage 0x004B2330).  Size 0x44 (RaceDisplay+0x6FFC). */
class RaceBattleTachometer : public RaceMeterBase {
public:
    RaceBattleTachometer();                     /* 0x0019D750 transmission 0 */
    /* The bar (RaceBarMeter::drawMeter, direct), then the transmission mark
     * at the bar's aligned corner, 5 pixels down. */
    virtual void drawMeter(GSBuffer &buf);      /* [ 5] +0x34 0x0019D7F0 */
    /* The rev limit is not used: the bar always shows "meter_mini". */
    virtual void setScale(f32 maxRpm);          /* [12] +0x6C 0x0019D7C8 (index: virtual_12) */

    /* The inline members, declared in the reverse of T161's emission order
     * (0x0019D8B0..0x0019D9A8, then __tf), as RaceRoundTachometerBase's. */
    /* The vptr, the bar's destructor expanded, RaceMeterBase's vptr,
     * ~RaceDisplayObjectBase. */
    virtual ~RaceBattleTachometer() {}          /* [ 0] +0x0C 0x0019D9A8 (out-of-line copy) */
    virtual void set(f32 rpm) { bar.value = rpm; }      /* [11] +0x64 0x0019D9A0 */
    void setRedZone(f32 rpm) { bar.scale = rpm; }       /* 0x0019D998 ? name: the bar's full scale */
    /* ? name.  As RaceRoundTachometerBase's (RaceDisplay::setupCarMeters
     * expands it). */
    void setTransmission(int t) { transmission = t; }   /* 0x0019D990 */
    /* The bar's (setRegion: RaceDisplayObjectBase::setRegion expanded, the
     * bar's setLocation / setSize through its vtable). */
    virtual void setLocation(int x, int y) { bar.setLocation(x, y); }       /* [ 6] +0x3C 0x0019D980 */
    virtual void setSize(int w, int h) { bar.setSize(w, h); }               /* [ 7] +0x44 0x0019D970 */
    virtual void setAlignment(u8 alignX, u8 alignY) {   /* [ 8] +0x4C 0x0019D960 */
        bar.setAlignment(alignX, alignY);
    }
    virtual void setRegion(int x, int y, int w, int h) {    /* [ 9] +0x54 0x0019D8F8 */
        bar.setRegion(x, y, w, h);
    }
    void setImagePack(ImagePack *pack) { bar.imagePack = pack; }    /* 0x0019D8F0 */
    void setImage(Tex1Image *img) { bar.setImage(img); }    /* 0x0019D8D0 ? name (no callers; 0x0019D010) */
    void setImage(const char *name) { bar.setImage(name); } /* 0x0019D8B0 ? name (no callers; 0x0019D030) */

    RaceBarMeter bar;                           /* 0x20 */
    u8           transmission;                  /* 0x40 0: the 0x3E mark, else 0x3D (lbu) */
    u8           unk_41[3];                     /* 0x41 */
};
EGCS_SIZE(RaceBattleTachometer, 0x44);

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
/* The __si_type_info nodes (.bss). */
extern "C" u32 __ti20RaceRemoteTachometer[3];   /* 0x004B11E0 */
extern "C" u32 __ti21RaceOnboardTachometer[3];  /* 0x004B1270 */
extern "C" u32 __ti20RaceBattleTachometer[3];   /* 0x004B2330 */
extern "C" u32 __ti23RaceRoundTachometerBase[3];    /* 0x004B2340 */
#endif

#endif /* GAME_T161_RACE_TACHOMETER_H */
