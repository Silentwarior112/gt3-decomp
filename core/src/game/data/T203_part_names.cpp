/* T203 - the part name tables: two arrays of {name, part id} sorted by
 * name, built by the static initialiser, and their lookups.
 * 0x001B2C20..0x001B3918.
 *
 * g_0049C638 (0x35 entries) names the parts themselves ("ayc_on",
 * "tir_sports", "tbk_2" ...), g_0049C3F0 (0x49 entries) the "set_*" settings
 * that put a part on or take it off ("set_tcs_off", "set_tur_stage3" ...).
 * Both map to the same part id numbering (0x58 = unknown name).
 *
 * Data (retail): .rodata 0x003322A0..0x003329D6 and .sdata
 * 0x00352450..0x0035250E are the names (those of 8 bytes or less in .sdata,
 * -G8), in the static initialiser's order; no type name (the original saw
 * no class with virtual functions).  .bss 0x0049C3F0..0x0049C7E0: the two
 * tables. */
#include "game/menu/framework/menu_buttons_internal.h"
#include "game/data/T203_part_names.h"

/* The tables: `static' arrays of {name, id} built by the inline constructor
 * at start-up (egcs: cp/init.c build_vec_init, a cursor stepping 8 bytes per
 * element, in _GLOBAL_$I$part_id_of_name__FPCc); statics with a constructor
 * go to .bss in reverse definition order, so the settings (defined second)
 * are at the lower address, as in retail.  The parts table is built first. */
static PartName g_0049C638[] = { /* 0x0049C638 the part names */
    PartName("ayc_on",           0x23),
    PartName("bbl_on",           0x3),
    PartName("bkt_racing",       0x1),
    PartName("clt_high",         0x5),
    PartName("clt_triple",       0x7),
    PartName("clt_twin",         0x6),
    PartName("cpu_on",           0x9),
    PartName("dsp_on",           0xB),
    PartName("egb_on",           0xE),
    PartName("fly_cr_mo",        0x11),
    PartName("fly_light",        0x10),
    PartName("fly_light_cr_mo",  0x12),
    PartName("icl_racing_big",   0x19),
    PartName("icl_sports",       0x18),
    PartName("lsd_1",            0x1F),
    PartName("lsd_15",           0x21),
    PartName("lsd_2",            0x20),
    PartName("lsd_variable",     0x22),
    PartName("muf_racing",       0x27),
    PartName("muf_semi_racing",  0x26),
    PartName("muf_sports",       0x25),
    PartName("nat_1",            0x29),
    PartName("nat_2",            0x2A),
    PartName("nat_3",            0x2B),
    PartName("prp_on",           0x2F),
    PartName("ptp_on",           0x2D),
    PartName("sms_on",           0x54),
    PartName("sus_racing",       0x35),
    PartName("sus_semi_racing",  0x34),
    PartName("sus_sports",       0x33),
    PartName("tbk_1",            0x4F),
    PartName("tbk_2",            0x50),
    PartName("tbk_3",            0x51),
    PartName("tbk_4",            0x52),
    PartName("tcs_on",           0x37),
    PartName("tir_all",          0xBC),
    PartName("tir_control",      0x41),
    PartName("tir_dirt",         0x42),
    PartName("tir_hard",         0x3B),
    PartName("tir_medium",       0x3D),
    PartName("tir_medium_hard",  0x3C),
    PartName("tir_medium_soft",  0x3E),
    PartName("tir_soft",         0x3F),
    PartName("tir_sports",       0x39),
    PartName("tir_super_hard",   0x3A),
    PartName("tir_super_soft",   0x40),
    PartName("tms_close",        0x14),
    PartName("tms_super_close",  0x15),
    PartName("tms_variable",     0x16),
    PartName("vcd_on",           0x56),
    PartName("wtr_1",            0x1B),
    PartName("wtr_2",            0x1C),
    PartName("wtr_3",            0x1D)
};
static PartName g_0049C3F0[] = { /* 0x0049C3F0 the "set_*" settings */
    PartName("set_asm_off",          0x53),
    PartName("set_asm_on",           0x54),
    PartName("set_ayc_off",          0x1E),
    PartName("set_ayc_on",           0x23),
    PartName("set_brc_off",          0x2),
    PartName("set_brc_on",           0x3),
    PartName("set_brk_normal",       0x0),
    PartName("set_brk_racing",       0x1),
    PartName("set_clt_big",          0x5),
    PartName("set_clt_normal",       0x4),
    PartName("set_clt_triple",       0x7),
    PartName("set_clt_twin",         0x6),
    PartName("set_coo_1",            0x18),
    PartName("set_coo_2",            0x19),
    PartName("set_coo_normal",       0x17),
    PartName("set_cpu_off",          0x8),
    PartName("set_cpu_on",           0x9),
    PartName("set_fly_crmo",         0x11),
    PartName("set_fly_light",        0x10),
    PartName("set_fly_light_crmo",   0x12),
    PartName("set_fly_normal",       0xF),
    PartName("set_lsd_15way",        0x21),
    PartName("set_lsd_1way",         0x1F),
    PartName("set_lsd_2way",         0x20),
    PartName("set_lsd_normal",       0x1E),
    PartName("set_lsd_variable",     0x22),
    PartName("set_msn_close",        0x14),
    PartName("set_msn_normal",       0x13),
    PartName("set_msn_super_close",  0x15),
    PartName("set_msn_variable",     0x16),
    PartName("set_muf_normal",       0x24),
    PartName("set_muf_racing",       0x27),
    PartName("set_muf_semi_racing",  0x26),
    PartName("set_muf_sports",       0x25),
    PartName("set_nat_off",          0x28),
    PartName("set_nat_stage1",       0x29),
    PartName("set_nat_stage2",       0x2A),
    PartName("set_nat_stage3",       0x2B),
    PartName("set_shf_normal",       0x2E),
    PartName("set_shf_propeller",    0x2F),
    PartName("set_sus_normal",       0x32),
    PartName("set_sus_racing",       0x35),
    PartName("set_sus_semi_racing",  0x34),
    PartName("set_sus_sports",       0x33),
    PartName("set_tcs_off",          0x36),
    PartName("set_tcs_on",           0x37),
    PartName("set_tir_f_control",    0x41),
    PartName("set_tir_f_normal",     0x38),
    PartName("set_tir_f_racing_h",   0x3B),
    PartName("set_tir_f_racing_m",   0x3D),
    PartName("set_tir_f_racing_mh",  0x3C),
    PartName("set_tir_f_racing_ms",  0x3E),
    PartName("set_tir_f_racing_s",   0x3F),
    PartName("set_tir_f_racing_sh",  0x3A),
    PartName("set_tir_f_racing_ss",  0x40),
    PartName("set_tir_f_sports",     0x39),
    PartName("set_tir_r_control",    0x4C),
    PartName("set_tir_r_normal",     0x43),
    PartName("set_tir_r_racing_h",   0x46),
    PartName("set_tir_r_racing_m",   0x48),
    PartName("set_tir_r_racing_mh",  0x47),
    PartName("set_tir_r_racing_ms",  0x49),
    PartName("set_tir_r_racing_s",   0x4A),
    PartName("set_tir_r_racing_sh",  0x45),
    PartName("set_tir_r_racing_ss",  0x4B),
    PartName("set_tir_r_sports",     0x44),
    PartName("set_tur_off",          0x4E),
    PartName("set_tur_stage1",       0x4F),
    PartName("set_tur_stage2",       0x50),
    PartName("set_tur_stage3",       0x51),
    PartName("set_tur_stage4",       0x52),
    PartName("set_vcd_off",          0x55),
    PartName("set_vcd_on",           0x56)
};

/* Part id of a part name, 0x58 if unknown. */
// FUNC 0x001B2C20 part_id_of_name
int part_id_of_name(const char *name)
{
    PartName *entry = find_part_name(g_0049C638, PART_NAME_COUNT, name);
    if (entry != 0)
        return entry->id;
    return PART_ID_UNKNOWN;
}

/* Part id of a "set_*" setting name, 0x58 if unknown. */
// FUNC 0x001B2C58 setting_id_of_name
int setting_id_of_name(const char *name)
{
    PartName *entry = find_part_name(g_0049C3F0, SETTING_NAME_COUNT, name);
    if (entry != 0)
        return entry->id;
    return PART_ID_UNKNOWN;
}

/* Binary search of `name` in table[0..count) (sorted by name, compared as
 * signed chars like strcmp); the entry or NULL.  The project's name-search
 * shape (ref/matching/u064/notes.md): tail-recursive, each half's count in
 * its own variable; egcs turns the self-calls into jumps. */
// FUNC 0x001B2C90 find_part_name
PartName *find_part_name(PartName *table, int count, const char *name)
{
    if (count <= 0)
        return 0;
    PartName *entry = &table[count / 2];
    const char *key = entry->name;
    for (int i = 0; ; i++) {
        if ((s8)name[i] != (s8)key[i]) {
            if ((s8)name[i] < (s8)key[i]) {
                int n = count / 2;              /* the lower half */
                if (n <= 0)
                    return 0;
                return find_part_name(table, n, name);
            }
            int n = count - count / 2 - 1;      /* the upper half */
            if (n <= 0)
                return 0;
            return find_part_name(entry + 1, n, name);
        }
        if (name[i] == '\0')
            break;                              /* a full match */
    }
    return entry;
}

/* Compiler-generated: egcs's static initialiser of the two tables (.ctors
 * entry 0x0028F150), emitted after the last function: the parts table, then
 * the settings, each element through PartName's inline constructor. */
// FUNC 0x001B2D28 T203_static_init (_GLOBAL_$I$part_id_of_name__FPCc)
/* implicit: PartName (the static initialiser of g_0049C638 / g_0049C3F0) */
