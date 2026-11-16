/* T160_race_meters.h - the meters of the race display: RaceRoundMeterBase,
 * RaceRoundMeter (round dials) and RaceBarMeter (translation unit T160,
 * unit u055), 0x0019C368..0x0019D278.
 *
 * Also here: RaceMeterBase, the meters' base (T154's class: its vtable and
 * inline copies are emitted with RaceDisplay's, 0x00195AB8..0x00195B48).
 * T160 defines its constructor (0x0019C368) and has weak copies of its
 * inline members (0x0019D1E8..0x0019D228, unreferenced).
 *
 * A round dial is drawn in two parts: drawMeter the panel image over the
 * rectangle in the panel colour; render_2nd (when enabled) the scale and
 * the needle in the panel's coordinates (translated to the dial centre,
 * scaled by the radius), each drawn 8 times at 1/8 alpha with the sub-pixel
 * offsets g_0032F568 (an anti-aliasing pass).  Angles are in degrees,
 * stored + 90.  The scale marks of a RaceRoundMeter are prebuilt by a
 * TStripBuilder (T448) into one of two packet buffers, alternately.
 *
 * The meters' header of the original: T161 / T162 / T163 include it and
 * T154_race_display.h includes it after the display parts (T157) and
 * UnitArena<64> / RaceCameraBase.  Retail's type names show the order
 * (egcs writes those of the classes without a key function in the reverse
 * of their definition order): RaceMeterBase's comes after the display's in
 * the objects that include the display first (T155, T157, T160, T164) and
 * first in those that include their meter header first (T161, T162, T163;
 * the T154 merge, ref/matching/u051/notes.md).
 *
 * Inline in the original: ~RaceBarMeter (expanded by RaceBattleTachometer's
 * destructor, T161, and RaceDisplay's, T154; no callers) and the small
 * accessors (out-of-line copies emitted at the end of the TU with the __tf
 * functions; expanded by T161 / T162: "setPanelColor, inline", ...): both
 * are defined inline below, u055 keeps the markers.  So is
 * RaceRoundMeterBase::draw_scale, for T160 only (GAME_T160_INLINE_DRAW_SCALE,
 * after the class: u055's data pass).
 */
#ifndef GAME_T160_RACE_METERS_H
#define GAME_T160_RACE_METERS_H

#include "game/game.h"
#include "game/race/display/T156_race_display_object.h"      /* base RaceDisplayObjectBase */

struct Tex1Image;                               /* G001 (pointers only) */
struct VifModel;                                /* G001 (TStripBuilder::finish's packet; pointers only) */
class GSBuffer;                                 /* T514 (references only) */
class type_info;                                /* T548 */

/* The colours of the tachometers', the speedometer's and the boost meter's
 * dials (T161 / T162 / T163 store them with the inline setPanelColor /
 * setScaleColor).  GS RGBA, 0x80 = 1.0.  (u056 proposal; moved from race_meters_internal.h.) */
#define METER_PANEL_COLOR       0x80A0A088u /* the panel image (and the transmission mark) */
#define METER_SCALE_COLOR       0x80808068u /* the scale marks */
#define METER_SCALE_RED_COLOR   0x800000F0u /* 0x800000F0u. The marks of the red zone */

/* Declared before the classes, for GCC 14 only: egcs declares a polymorphic
 * class's __tf<mangled> itself (C++ linkage) when the class is complete and
 * rejects a later extern "C" declaration; an earlier one would be the
 * declaration it uses, and it would then never synthesize the function
 * (u042 / u055: T160's three typeinfo functions).  Each builds an
 * __si_type_info on its base's node. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf13RaceMeterBase(void);     /* 0x00195AF8 (T154; copies T160 0x0019D228, T161 0x0019DDE0) */
const type_info &__tf12RaceBarMeter(void);      /* 0x0019D090 (__ti12RaceBarMeter 0x004B2320, base
                                                   __ti13RaceMeterBase) */
const type_info &__tf14RaceRoundMeter(void);    /* 0x0019D118 (__ti14RaceRoundMeter 0x004B2310, base
                                                   __ti18RaceRoundMeterBase) */
const type_info &__tf18RaceRoundMeterBase(void);    /* 0x0019D198 (__ti18RaceRoundMeterBase 0x004B2300,
                                                       base __ti13RaceMeterBase) */
}
#endif

/* The base of the meters (RaceRoundMeterBase / RaceBarMeter here,
 * RaceRoundTachometerBase / RaceBattleTachometer T161).  vtables 0x0032E368
 * (T154; the constructor stores it), 0x0032F3F0, 0x0032F7E8 (13 slots: [0]
 * [10..12] its own), typeinfo __tf13RaceMeterBase 0x00195AF8 (si, base
 * RaceDisplayObjectBase).  vptr +0x14; 0x20 bytes.  Its virtuals are inline
 * in the original: T154 0x00195AB8..0x00195AD0, T160 0x0019D1E8..
 * 0x0019D200, T161 0x0019DDA0..0x0019DDB8, T162 0x0019E078 / 0x0019E080,
 * T163 0x0019E220 / 0x0019E228.  (In T154_race_display.h before the T154
 * merge.) */
class RaceMeterBase : public RaceDisplayObjectBase {
public:
    RaceMeterBase();                        /* 0x0019C368 (T160) value 0, scale 1, enabled */
    virtual ~RaceMeterBase() {}             /* [ 0] +0x0C 0x00195AD0 */
    /* The second draw pass (the round dials' scale and needle). */
    virtual void render_2nd() {}            /* [10] +0x5C 0x00195AC8 */
    virtual void set(f32 v) { value = v; }  /* [11] +0x64 0x00195AC0 */
    virtual void setScale(f32 v) { scale = v; }     /* [12] +0x6C 0x00195AB8 */

    f32 value;                              /* 0x18 the value shown */
    f32 scale;                              /* 0x1C the full-scale value */
};
EGCS_SIZE(RaceMeterBase, 0x20);

/* vtable 0x0032F370 (15 slots: [0] [5] [10] its own, [13] [14] new; the
 * others RaceDisplayObjectBase's / RaceMeterBase's; index.txt names [5]
 * [10] [14] RaceRoundMeter::virtual_N, they are this class's).  typeinfo
 * __tf18RaceRoundMeterBase 0x0019D198 (si, base RaceMeterBase).  vptr +0x14
 * (RaceDisplayObjectBase's).  0x4C bytes (RaceRoundMeter's data and
 * RaceBoostmeter's, T163, start at +0x4C). */
class RaceRoundMeterBase : public RaceMeterBase {
public:
    /* lastAngle FLT_MAX, centre (0.5, 0.5), radius 150 (aspect 1), angles
     * 65..295, scale 1, needle -0.1..0.95, panel colour 0x80FFFFFF. */
    RaceRoundMeterBase();                       /* 0x0019C3B8 */
    virtual ~RaceRoundMeterBase();              /* [ 0] +0x0C 0x0019C4A0 (index: virtual_0) */
    /* The panel image (image) over the rectangle in panelColor; the
     * argument is not used.  Also called directly by
     * RaceRoundTachometerBase::drawMeter and RaceBoostmeter::drawMeter. */
    virtual void drawMeter(GSBuffer &buf);      /* [ 5] +0x34 0x0019C4C8 */
    /* When enabled: the identity VU0 matrix translated to the dial centre
     * and scaled by the radius (both in panel image pixels, scaled to the
     * rectangle), then draw_scale and draw_needle (virtual calls). */
    virtual void render_2nd();                  /* [10] +0x5C 0x0019C540 */
    /* Empty; inline in T160 only (GAME_T160_INLINE_DRAW_SCALE, below the
     * class). */
    virtual void draw_scale();                  /* [13] +0x74 0x0019D168 */
    /* The angle of value / scale (value clamped to 0..scale + 8) between
     * the start and end angles; the needle (g_0032F250) 8 times at alpha
     * 1/8, each rotated a further (angle - lastAngle) / 8. */
    virtual void draw_needle();                 /* [14] +0x7C 0x0019C738 */

    void setCenter(f32 x, f32 y);               /* 0x0019C8E8 */
    void setRadius(f32 r, f32 aspect);          /* 0x0019C8F8 radiusX = r * aspect, radiusY = r */
    void setAngles(f32 start, f32 end);         /* 0x0019C908 stored + 90 */
    /* The inline accessors (out-of-line copies in T160, no callers),
     * declared in the reverse of their copies' order: egcs writes a class's
     * inline members last-declared first (u055's data pass: T160's frame
     * info). */
    void setNeedle(f32 inner, f32 outer) { needleInner = inner; needleOuter = outer; } /* 0x0019D188 */
    void setPanelColor(u32 color) { panelColor = color; } /* 0x0019D180 */
    f32 getCenterX() const { return centerX; } /* 0x0019D178 */
    f32 getCenterY() const { return centerY; } /* 0x0019D170 */

    u32 panelColor;                             /* 0x20 */
    f32 lastAngle;                              /* 0x24 the needle angle drawn last (FLT_MAX: none yet) */
    f32 centerX;                                /* 0x28 in panel image pixels, from the rectangle's centre */
    f32 centerY;                                /* 0x2C */
    f32 radiusX;                                /* 0x30 r * aspect */
    f32 radiusY;                                /* 0x34 r */
    f32 startAngle;                             /* 0x38 degrees + 90 */
    f32 endAngle;                               /* 0x3C degrees + 90 */
    f32 needleInner;                            /* 0x40 */
    f32 needleOuter;                            /* 0x44 */
    u8  unk_48[4];                              /* 0x48 */
};
EGCS_SIZE(RaceRoundMeterBase, 0x4C);

/* RaceRoundMeterBase::draw_scale, inline for T160 alone (u055's data pass).
 * Retail's T160 writes its copy (0x0019D168) among RaceRoundMeterBase's
 * inline members, after RaceRoundMeter's __tf and before getCenterY: egcs
 * saw an inline body after this class and before RaceRoundMeter.  But
 * T163's RaceBoostmeter vtable, which inherits the slot, uses T160's copy
 * and T163 has none of its own (it has copies of RaceMeterBase's inline
 * set / setScale for the same vtable): T163 saw no inline body.  The other
 * includers see the declaration only.  An exact form, not necessarily the
 * original's. */
#ifdef GAME_T160_INLINE_DRAW_SCALE
inline void RaceRoundMeterBase::draw_scale()
{
}
#endif

/* vtable 0x0032F2F0 (15 slots: [0] and [13] its own; index.txt names them
 * RaceRoundMeter::virtual_0 / _13).  typeinfo __tf14RaceRoundMeter
 * 0x0019D118 (si, base RaceRoundMeterBase).  Size 0x307C (RaceDisplay's
 * speedometer is one, RaceRoundTachometerBase::meter at +0x20). */
class RaceRoundMeter : public RaceRoundMeterBase {
public:
    /* No scale packet yet; the two packet buffers 16-byte aligned in
     * dialData; the dial defaults again; the scale: radius 0.95, marks 0.03
     * long, 3/4 filled, 0.1 apart, red from 0.75 (0x80D0D0D0 / 0x800000FF). */
    RaceRoundMeter();                           /* 0x0019C928 */
    virtual ~RaceRoundMeter();                  /* [ 0] +0x0C 0x0019CA58 (11 callers) */
    /* The scale packet (once there is one) 8 times at alpha 1/8. */
    virtual void draw_scale();                  /* [13] +0x74 0x0019CB30 */

    void updateScale();                         /* 0x0019CA80 buildScale().  Callers:
                                                   RaceDisplay::setupCarMeters, T161 */
    /* scale / scaleStep marks over the angle range into the other packet
     * buffer, each a strip over scaleFill of its angle; from redZone /
     * scaleStep on (when not 0) in scaleColor[1].  A TStripBuilder on the
     * stack. */
    void buildScale();                          /* 0x0019CBD0 */
    /* The inline accessors (out-of-line copies in T160, no callers), in the
     * reverse of their copies' order (as RaceRoundMeterBase's). */
    void setScaleRadius(f32 r) { scaleRadius = r; } /* 0x0019D110 */
    void setScaleLine(f32 length, f32 fill, f32 step) { scaleLength = length; scaleFill = fill; scaleStep = step; } /* 0x0019D100 */
    void setRedZone(f32 v) { redZone = v; } /* 0x0019D0F8 */
    f32 getRedZone() const { return redZone; } /* 0x0019D0F0 */
    void setScaleColor(u32 normal, u32 red) { scaleColor[0] = normal; scaleColor[1] = red; } /* 0x0019D0E0 */

    int   dialIndex;                            /* 0x4C the packet buffer of the scale, -1: none yet */
    f32   redZone;                              /* 0x50 the value the red marks start at */
    f32   scaleRadius;                          /* 0x54 the inner radius of the marks */
    f32   scaleLength;                          /* 0x58 the length of the marks */
    f32   scaleFill;                            /* 0x5C the part of a mark's angle drawn */
    u8    unk_60[4];                            /* 0x60 */
    f32   scaleStep;                            /* 0x64 the value per mark */
    u32   scaleColor[2];                        /* 0x68 normal, red zone */
    VifModel     *dialPacket;                   /* 0x70 the scale packet (TStripBuilder::finish; model_draw) */
    void *dialBuf[2];                           /* 0x74 16-byte aligned, inside dialData */
    u8    dialData[0x3000];                     /* 0x7C */
};
EGCS_SIZE(RaceRoundMeter, 0x307C);

/* vtable 0x0032F280 (13 slots: [0] and [5] its own).  typeinfo
 * __tf12RaceBarMeter 0x0019D090 (si, base RaceMeterBase).  No data of its
 * own: 0x20 bytes (RaceBattleTachometer::bar at +0x20). */
class RaceBarMeter : public RaceMeterBase {
public:
    RaceBarMeter();                             /* 0x0019CEE0 */
    /* Inline in the original: RaceBattleTachometer's destructor (T161) and
     * RaceDisplay's (T154) expand it (RaceMeterBase's vptr, then
     * ~RaceDisplayObjectBase); 0x0019D068 is the out-of-line copy egcs
     * emitted with the vtable. */
    virtual ~RaceBarMeter() {}                  /* [0] +0x0C 0x0019D068 */
    /* The image over the rectangle with the alpha test reference at
     * value * 56 / scale (clamped to 4..64): its alpha gradient shows the
     * bar up to the value.  Also called directly by
     * RaceBattleTachometer::drawMeter. */
    virtual void drawMeter(GSBuffer &buf);      /* [5] +0x34 0x0019CF18 */

    void setImage(Tex1Image *img);              /* 0x0019D010 image = img, its TEX1 MMAG / MMIN
                                                   cleared (point sampling) */
    void setImage(const char *name);            /* 0x0019D030 the pack's named image */
};
EGCS_SIZE(RaceBarMeter, 0x20);

/* No callers.  Fills count words at dst with value: first ((dst >> 2) & 3)
 * single words (not taken off count), then 4 at a time (sq), then the rest.
 * ? name: a static helper of T160 or an inline library one, unknown. */
void T160_fill_words(u32 *dst, int count, u32 value);  /* 0x0019CAA0 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
/* The __si_type_infos {vptr, name, base} the three __tf functions fill.
 * (The plan had put RaceRoundMeterBase's in T158 and RaceRoundMeter's in
 * T154; u055 / u056: g_004B2300 / g_004B2310.) */
extern "C" {
extern u32 __ti13RaceMeterBase[3];              /* 0x004B11D0 */
extern u32 __ti18RaceRoundMeterBase[3];         /* 0x004B2300 */
extern u32 __ti14RaceRoundMeter[3];             /* 0x004B2310 */
extern u32 __ti12RaceBarMeter[3];               /* 0x004B2320 */
}
#endif

#endif /* GAME_T160_RACE_METERS_H */
