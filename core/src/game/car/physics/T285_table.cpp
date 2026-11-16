/* T285 (0x001DE050..0x001DE070): one lookup in a float table of 16 columns
 * (g_002D33A8[5][16], T285's .data); its caller is the race sound code
 * (RaceEntryCar::updateSound, T121). */
#include "game/car/physics/T285_table.h"
#include "game/car/physics/car_param_internal.h"

/* T285's .data (then its frame info at 0x002D34E8): the gain of each sound
 * (column) in each listening mode (row; race_entry_internal.h LISTEN_*, SND_GAIN_*). */
f32 g_002D33A8[5][16] = {                   /* 0x002D33A8 */
    { 3.0f, 2.5f, 0.5f, 0.2f, 0.8f, 0.3f, 1.4f, 0.6f,
      0.0f, 0.9f, 0.9f, 0.6f, 1.0f, 0.8f, 1.4f, 1.0f },
    { 0.3f, 1.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.2f, 0.0f,
      0.3f, 0.3f, 0.6f, 0.3f, 1.0f, 0.6f, 1.0f, 1.0f },
    { 1.0f, 3.0f, 0.0f, 0.0f, 0.3f, 0.5f, 1.0f, 0.0f,
      1.4f, 0.6f, 0.9f, 0.6f, 1.2f, 0.6f, 1.0f, 1.0f },
    { 0.3f, 1.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f,
      0.3f, 0.3f, 0.6f, 0.3f, 0.6f, 0.6f, 1.0f, 1.0f },
    { 0.5f, 3.3f, 0.0f, 0.0f, 0.2f, 0.5f, 0.2f, 0.0f,
      0.8f, 0.9f, 1.0f, 0.5f, 1.0f, 0.3f, 1.0f, 1.0f },
};

// FUNC 0x001DE050 F_001DE050
f32 F_001DE050(int row, int col)
{
    return g_002D33A8[row][col];
}
