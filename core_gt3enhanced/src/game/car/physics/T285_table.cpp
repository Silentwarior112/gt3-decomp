/* T285 (0x001DE050..0x001DE070): one lookup in a float table of 16 columns
 * (g_002D33A8[5][16], T285's .data); its caller is the race sound code
 * (RaceEntryCar::updateSound, T121). */
#include "game/car/physics/T285_table.h"
#include "game/car/physics/car_param_internal.h"

/* T285's .data (then its frame info at 0x002D34E8): the gain of each sound
 * (column) in each listening mode (row; race_entry_internal.h LISTEN_*, SND_GAIN_*). */
/* The enhanced engine: ENH_SOUND_MIX_TABLE (cameras_sound.md S3; Lupo 2's
 * table L2 0x002DE7E0): in the car the engine 2.8, the exhaust 2.3, the gear
 * whine 0.7, the road 0.5; outside the exhaust 3.0 (GT3 3.0, 2.5, 0.8, 0.6;
 * 3.3).  Asked for by the cameras + sound area. */
f32 g_002D33A8[5][16] = {                   /* 0x002D33A8 */
#if ENH_SOUND_MIX_TABLE
    { 2.8f, 2.3f, 0.5f, 0.2f, 0.7f, 0.3f, 1.4f, 0.5f,     /* Lupo 2 (L2 0x002DE7E0) */
      0.0f, 0.9f, 0.9f, 0.6f, 1.0f, 0.8f, 1.4f, 1.0f },
#else
    { 3.0f, 2.5f, 0.5f, 0.2f, 0.8f, 0.3f, 1.4f, 0.6f,
      0.0f, 0.9f, 0.9f, 0.6f, 1.0f, 0.8f, 1.4f, 1.0f },
#endif
    { 0.3f, 1.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.2f, 0.0f,
      0.3f, 0.3f, 0.6f, 0.3f, 1.0f, 0.6f, 1.0f, 1.0f },
    { 1.0f, 3.0f, 0.0f, 0.0f, 0.3f, 0.5f, 1.0f, 0.0f,
      1.4f, 0.6f, 0.9f, 0.6f, 1.2f, 0.6f, 1.0f, 1.0f },
    { 0.3f, 1.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f,
      0.3f, 0.3f, 0.6f, 0.3f, 0.6f, 0.6f, 1.0f, 1.0f },
#if ENH_SOUND_MIX_TABLE
    { 0.5f, 3.0f, 0.0f, 0.0f, 0.2f, 0.5f, 0.2f, 0.0f,     /* Lupo 2: outside, the exhaust 3.0 */
      0.8f, 0.9f, 1.0f, 0.5f, 1.0f, 0.3f, 1.0f, 1.0f },
#else
    { 0.5f, 3.3f, 0.0f, 0.0f, 0.2f, 0.5f, 0.2f, 0.0f,
      0.8f, 0.9f, 1.0f, 0.5f, 1.0f, 0.3f, 1.0f, 1.0f },
#endif
};

// FUNC 0x001DE050 F_001DE050
f32 F_001DE050(int row, int col)
{
    return g_002D33A8[row][col];
}
