/* T511_inflate_loader.h - GZIP::InflateLoader (translation unit T511, unit u110),
 * 0x0023E0F0..0x0023E268. */
#ifndef GAME_T511_INFLATE_LOADER_H
#define GAME_T511_INFLATE_LOADER_H

#include "game/game.h"
#include "engine/pdistd/T537_gzip_inflate.h"             /* base GZIP::InflateBase */

namespace GZIP {

/* An InflateBase whose input is refilled from an IOP file stream (T523
 * F_0025D5B8) into a fixed buffer.  The constructor inflates the whole raw
 * DEFLATE stream into out at once: fetchInput (a direct call), initBitStream
 * on buf, outBase = out = out (setOutputData inline), inflate_main(false).
 * Its user: the volume reader thread (T503 vol_thread_main, on its stack,
 * with a 0x2000-byte 64-aligned buffer).
 *
 * vtable 0x00343B40 (2 slots).  typeinfo __tfQ24GZIP13InflateLoader 0x0023E218
 * (__si_type_info 0x004B2AA0, base InflateBase).  vptr +0x3030 (InflateBase's),
 * size 0x3048. */
class InflateLoader : public InflateBase {
public:
    InflateLoader(int stream, void *out, void *buf, int bufsize);  /* 0x0023E0F0 */
    virtual ~InflateLoader();                   /* [0] +0x0C 0x0023E1B8 */
    /* reads the next bufsize bytes of the stream into buf, inNext = buf
     * (inEnd is left as it is) */
    virtual void fetchInput(void);              /* [1] +0x14 0x0023E1E0 */

    int   stream;                               /* 0x3038 the stream handle (T523) */
    void *buf;                                  /* 0x303C the input buffer */
    int   bufsize;                              /* 0x3040 its size: the bytes read per refill */
};
EGCS_SIZE(GZIP::InflateLoader, 0x3048);

} /* namespace GZIP */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tfQ24GZIP13InflateLoader(void);         /* 0x0023E218 */
extern u32 __tiQ24GZIP13InflateLoader[3];       /* 0x004B2AA0 (.bss, a __si_type_info) */
}
#endif

#endif /* GAME_T511_INFLATE_LOADER_H */
