/* T469 - the sound effects: the system / race effect banks, the road noise
 * bank and sound/se.inf.  0x0022C3A0..0x0022C900.
 *
 * The code of an effect (se.inf): bank << 24 | program A << 12 | program B.
 *
 * The enhanced engine (Lupo 2, cameras_sound.md S1): the Toyota Pod's voice.
 * The effect table has Lupo 2's 11 pod_* names after GT3's 40 (L2 se_init
 * 0x0023B788 registers 0x33), and a fourth bank holds sound/gt3pod.ins (L2
 * loads it into bank 3, 0x0023B820..0x0023B830).  GT3 has neither the file
 * nor the names: the bank is loaded only when the file exists and fits, and
 * a pod_* name se.inf lacks keeps the index SE_INDEX_NONE; se_find_program
 * then gives no program, and the Pod is silent.  Log lines: "snd: ...". */
#include "game/sound/gtsound_internal.h"
#include "game/sound/T469_sound_effects.h"

/* The enhanced engine: the Pod's bank file, its header buffer (its own: the
 * 0x800 bytes of g_seBankHeaders are sized for GT3's banks and rn_01, and
 * nothing checks their end) and the SPU address where the music and the race
 * cars' samples start (music_init, car_sound_reset_spu: 0xAC000), which the
 * effect banks must stay below. */
#define SE_POD_BANK_PATH        "sound/gt3pod.ins"
#define SE_POD_HEADER_SIZE      0x800
#define SE_SPU_END              0xAC000

/* The banks.  egcs puts a public object that has a constructor in .data
 * (zero-filled) and a static one in .bss, as retail has them; their
 * constructors and destructors run in the compiler's _GLOBAL_$I$ / $D$
 * functions (T469_static_init / T469_static_dtor below). */
GTSOUNDINSTRUMENT g_roadNoiseBank[1];                   /* 0x002E9360 sound/rn_01.es */
static GTSOUNDINSTRUMENT g_seBanks[SE_BANK_COUNT];      /* 0x0049ECC0 gt3sys, gt3se01, gt3se02 */

SeInf *g_seInf = NULL;                                  /* 0x00352B98 sound/se.inf */
/* The effect table: the se.inf name of each effect (SoundEffect); se_init
 * sets the indexes.  Its names are written just before it, in its order:
 * those of 8 bytes or less to .sdata (-G8, 0x00352BA0.., after g_seInf), the
 * others open T469's .rodata (0x003415D0, before the functions' literals;
 * only the table points at them). */
// DATA_START 0x003415D0
SeName g_seNames[SE_TABLE_COUNT] = {                    /* 0x002E9380 */
    { "buy",          0 },
    { "cancel",       0 },
    { "car_wash",     0 },
    { "cursor",       0 },
    { "disable",      0 },
    { "next",         0 },
    { "oil",          0 },
    { "ok",           0 },
    { "param",        0 },
    { "return",       0 },
    { "select",       0 },
    { "slide",        0 },
    { "wheel",        0 },
    { "b_off",        0 },
    { "body_noiz",    0 },
    { "clash",        0 },
    { "clash2",       0 },
    { "clash3",       0 },
    { "clash4",       0 },
    { "clash5",       0 },
    { "count_d",      0 },
    { "count_s",      0 },
    { "heri",         0 },
    { "in_car",       0 },
    { "off_bound",    0 },
    { "off_roadnoiz", 0 },
    { "off_skill",    0 },
    { "on_bound",     0 },
    { "on_road",      0 },
    { "on_skill",     0 },
    { "rain_noiz",    0 },
    { "rumble",       0 },
    { "turbine",      0 },
    { "turbine2",     0 },
    { "air",          0 },
    { "city",         0 },
    { "crowds",       0 },
    { "sea",          0 },
    { "tunnel",       0 },
    { "tunnel2",      0 },
    /* The enhanced engine: the Pod's (Lupo 2's 40..50); SE_INDEX_NONE until
     * se.inf names them (retail's default 0 is a real entry of se.inf). */
    { "pod_genki",    SE_INDEX_NONE },
    { "pod_iya",      SE_INDEX_NONE },
    { "pod_kowai1",   SE_INDEX_NONE },
    { "pod_kowai2",   SE_INDEX_NONE },
    { "pod_kyoro",    SE_INDEX_NONE },
    { "pod_naku",     SE_INDEX_NONE },
    { "pod_nemui",    SE_INDEX_NONE },
    { "pod_odoroku",  SE_INDEX_NONE },
    { "pod_okoru",    SE_INDEX_NONE },
    { "pod_sabishii", SE_INDEX_NONE },
    { "pod_wink",     SE_INDEX_NONE },
};

/* The enhanced engine: 1 once gt3pod.ins is in bank SE_BANK_POD. */
static int g_sePodBankLoaded = 0;

// FUNC 0x0022C3A0 se_inf_relocate
void se_inf_relocate(SeInf *inf)
{
    SeInf *built = inf->base;
    u32 count = inf->count;
    u32 i;
    int delta;

    inf->base = inf;
    delta = (u8 *)inf - (u8 *)built;
    for (i = 0; i < count; i++)
        u104_reloc(inf->entry[i].name, delta);
}

/* The enhanced engine (Lupo 2 L2 0x0023B820, cameras_sound.md S1): sound/
 * gt3pod.ins into bank SE_BANK_POD, at the SPU allocation pointer, when the
 * file exists (the file layer retries a missing file forever: file_resolve
 * first), its samples end below SE_SPU_END (checked with the whole file's
 * size before anything is read, so the music samples at SE_SPU_END are never
 * written over) and its header fits its buffer.  A fixed bank (setFixed):
 * the header goes to that buffer, then the SPU allocator moves past the
 * samples.  Caller: se_init, after rn_01.es (Lupo 2 loads it before rn_01;
 * after it, GT3's banks keep their SPU places).  Lupo 2's own files fit:
 * gt3pod.ins is 81,760 bytes. */
static void se_load_pod_bank(void)
{
    static u8 g_sePodHeader[SE_POD_HEADER_SIZE] __attribute__((aligned(64)));
    GTSOUNDINSTRUMENT *bank = &g_seBanks[SE_BANK_POD];
    int spu = sound_get_spu_addr();
    SoundBankHeader *file;
    FileLoc loc;

    if (file_resolve(SE_POD_BANK_PATH, &loc, &vol_device_object) != 0 || !(loc.flags & 1)) {
        printf("snd: %s not found: the Pod's voice is off\n", SE_POD_BANK_PATH);
        return;
    }
    if (spu + (int)loc.size > SE_SPU_END) {
        printf("snd: %s (%d bytes) does not fit below SPU %05X (the effects end at %05X): "
               "the Pod's voice is off\n", SE_POD_BANK_PATH, loc.size, SE_SPU_END, spu);
        return;
    }
    bank->setFixed((SoundBankHeader *)g_sePodHeader, sizeof(g_sePodHeader), spu, SE_SPU_END - spu);
    file = (SoundBankHeader *)file_read_sound_bank(SE_POD_BANK_PATH, bank->nextSpuAddr(), NULL, 0);
    if (file == NULL) {
        printf("snd: %s is not a sound bank: the Pod's voice is off\n", SE_POD_BANK_PATH);
        return;
    }
    if (file->size > (int)sizeof(g_sePodHeader)) {
        printf("snd: %s: its header (%d bytes) is larger than %d: the Pod's voice is off\n",
               SE_POD_BANK_PATH, file->size, (int)sizeof(g_sePodHeader));
        free(file);
        return;
    }
    bank->load(file, 0);                    /* fixed: transfer only */
    sound_set_spu_addr(spu + bank->spuSize);
    g_sePodBankLoaded = 1;
    printf("snd: %s -> bank %d, SPU %05X..%05X (limit %05X), %d programs\n", SE_POD_BANK_PATH,
           SE_BANK_POD, spu, spu + bank->spuSize, SE_SPU_END, bank->header->nPrograms);
}

/* The enhanced engine: which of the Pod's effects se.inf names on the Pod's
 * bank (one line each that does not, one line in all).  Caller: se_init. */
static void se_report_pod_effects(void)
{
    int ready = 0;
    int se;

    if (!g_sePodBankLoaded) {
        for (se = SE_POD_FIRST; se < SE_TABLE_COUNT; se++)
            if (g_seNames[se].index >= 0)
                ready++;
        printf("snd: no Pod bank: its %d sounds are silent (se.inf names %d)\n", SE_POD_COUNT, ready);
        return;
    }
    for (se = SE_POD_FIRST; se < SE_TABLE_COUNT; se++) {
        int index = g_seNames[se].index;

        if (index < 0) {
            printf("snd: %s is not in se.inf: silent\n", g_seNames[se].name);
        } else if (SE_CODE_BANK(g_seInf->entry[index].code) != SE_BANK_POD) {
            printf("snd: %s (se.inf %d) is on bank %d, not %d: silent\n", g_seNames[se].name,
                   index, SE_CODE_BANK(g_seInf->entry[index].code), SE_BANK_POD);
        } else if (se_find_program(se, SE_BANK_POD) == NULL) {
            printf("snd: %s (se.inf %d, program %d): no such program in bank %d: silent\n",
                   g_seNames[se].name, index, SE_CODE_A(g_seInf->entry[index].code), SE_BANK_POD);
        } else {
            ready++;
        }
    }
    printf("snd: %d of %d Pod sounds ready\n", ready, SE_POD_COUNT);
}

/* Loads the banks: gt3sys.ins at SPU 0x1E0000, gt3se01 / gt3se02.ins from
 * 0x5040 on, rn_01.es after them (headers in the 0x800-byte
 * g_seBankHeaders), then se.inf, and links the effect table to it.
 * The enhanced engine: the Pod's bank after rn_01.es (se_load_pod_bank) and
 * its effects reported once se.inf is in. */
// FUNC 0x0022C3E8 se_init
void se_init(void)
{
    /* The banks' headers (and rn_01's).  A static of this function: retail's
     * .bss has it first (64-aligned: 0x0049E490..0x0049E4C0 is padding),
     * before g_seBanks, which egcs writes right after the last function; a
     * file static would follow them (egcs writes those at the very end). */
    static u8 g_seBankHeaders[0x800] __attribute__((aligned(64)));    /* 0x0049E4C0 */
    SeInf *inf;
    u32 entry;
    int se;

    if (g_seInf != NULL)
        return;

    sound_set_header_buffer(g_seBankHeaders, sizeof(g_seBankHeaders));
    sound_set_spu_addr(0x1E0000);
    g_seBanks[0].loadFile("sound/gt3sys.ins", 0);
    sound_set_spu_addr(0x5040);
    g_seBanks[1].loadFile("sound/gt3se01.ins", 0);
    g_seBanks[2].loadFile("sound/gt3se02.ins", 0);
    g_roadNoiseBank[0].loadFile("sound/rn_01.es", 1);
    se_load_pod_bank();                     /* the enhanced engine: Lupo 2's 4th bank */
    inf = (SeInf *)file_read_blocking("sound/se.inf", NULL, 0, 1, &vol_device_object);
    se_inf_relocate(inf);
    g_seInf = inf;
    for (entry = 0; entry < inf->count; entry++) {
        const char *name = inf->entry[entry].name;
        for (se = 0; se < SE_TABLE_COUNT; se++) {
            if (strcmp(name, g_seNames[se].name) == 0) {
                g_seNames[se].index = entry;
                break;
            }
        }
    }
    se_report_pod_effects();                /* the enhanced engine */
}

// FUNC 0x0022C560 se_term
void se_term(void)
{
    if (g_seInf != NULL) {
        free(g_seInf);
        g_seInf = NULL;
    }
}

// FUNC 0x0022C588 se_code
u32 se_code(int se)
{
    return g_seInf->entry[g_seNames[se].index].code;
}

/* The program of effect `se': B for which == 2, else A (retail's switch
 * range-tests 0..1 before the 2: cases 0 and 1 share the default). */
// FUNC 0x0022C5B0 se_program
SoundProgram *se_program(int se, int which)
{
    u32 code = se_code(se);
    int bank = SE_CODE_BANK(code);
    int a = SE_CODE_A(code);
    int b = SE_CODE_B(code);

    switch (which) {
    case 0:
    case 1:
    default:
        return g_seBanks[bank].program(a);
    case 2:
        return g_seBanks[bank].program(b);
    }
}

/* The enhanced engine (cameras_sound.md S1; not in Lupo 2, whose files
 * always have its effects): program A of effect `se', or NULL when se.inf
 * does not name it, names another bank than `bank' (>= 0), or its bank is
 * not loaded or has no such program.  Callers: CarSound::updatePodVoice
 * (T176), se_report_pod_effects. */
SoundProgram *se_find_program(int se, int bank)
{
    GTSOUNDINSTRUMENT *b;
    int index, a;
    u32 code;

    if (g_seInf == NULL || se < 0 || se >= SE_TABLE_COUNT)
        return NULL;
    index = g_seNames[se].index;
    if (index < 0 || (u32)index >= g_seInf->count)
        return NULL;
    code = g_seInf->entry[index].code;
    if (bank >= 0 && (int)SE_CODE_BANK(code) != bank)
        return NULL;
    if (SE_CODE_BANK(code) >= SE_BANK_COUNT)
        return NULL;
    b = &g_seBanks[SE_CODE_BANK(code)];
    if (b->header == NULL || b->headerSize == 0)
        return NULL;
    if (SE_CODE_BANK(code) == SE_BANK_POD && !g_sePodBankLoaded)
        return NULL;
    a = SE_CODE_A(code);
    if ((u32)a >= b->header->nPrograms)
        return NULL;
    return b->program(a);
}

/* Sets the effect volume (interrupts off); returns the old one. */
// FUNC 0x0022C620 se_set_volume
f32 se_set_volume(f32 volume)
{
    int intr = intr_suspend();
    f32 old = g_seVolume;

    g_seVolume = volume;
    intr_resume(intr);
    return old;
}

/* Plays effect `se' (pan 0x40 = centre, prio -1 = the program's).  When its
 * programs A and B are the same, A is played twice, hard left and hard
 * right, and `pan' is ignored; else only A is played. */
// FUNC 0x0022C658 se_play
void se_play(int se, int pan, int prio, f32 volume)
{
    u32 code = se_code(se);
    GTSOUNDINSTRUMENT *bank = &g_seBanks[SE_CODE_BANK(code)];
    int progA = SE_CODE_A(code);

    if (progA != (int)SE_CODE_B(code)) {
        bank->play(progA, pan, volume, prio);
        return;
    }
    bank->play(progA, SE_PAN_LEFT, volume, prio);
    bank->play(progA, SE_PAN_RIGHT, volume, prio);
}

/* Compiler-generated: egcs's static destructor and initialiser of the two
 * bank arrays above (named after the unit's first global), emitted at the
 * end of the unit, the destructor first. */
// FUNC 0x0022C718 T469_static_dtor (_GLOBAL_$D$g_roadNoiseBank)
//   (.dtors entry 0x0028F24C) the banks' virtual destructors (flags 0),
//   g_seBanks then g_roadNoiseBank, last first (the enhanced engine: 4 banks,
//   Lupo 2's L2 0x0023BAF0).
/* implicit: ~GTSOUNDINSTRUMENT */
// FUNC 0x0022C7C8 T469_static_init (_GLOBAL_$I$g_roadNoiseBank)
//   (.ctors entry 0x0028F178) GTSOUNDINSTRUMENT() of g_roadNoiseBank, then
//   of g_seBanks (the cleanups destroy the ones built if one throws).
/* implicit: GTSOUNDINSTRUMENT */
