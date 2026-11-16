/* T108_memorycard_file.h - the memory card files of the gt2k_menu save /
 * load screens: gt2k_menu::MemorycardFile and its subclasses,
 * MemorycardFileList and the GT3 save names (translation unit T108, units
 * u023 0x00142790..0x001447D8 and u024 0x001447D8..0x00147870).
 *
 * A MemorycardFile is one save directory "/<name>" of a card (or a
 * placeholder), a cell of the file manager's List<MemorycardFile> (T109):
 *   pCell (T495) - Cell<MemorycardFile> - MemorycardFile (0x68, abstract)
 *     +- MemorycardPS1File (0x270)        a PS1 save: its header block
 *     +- MemorycardPS2File (0x230)        a PS2 save: its icon.sys
 *     |    +- MemorycardOtherPS2File (0x238)   another game's: its own icon
 *     |    +- MemorycardGT3File (0x298, abstract)
 *     |         +- MemorycardGameFile 'G', MemorycardReplayFile 'R',
 *     |            MemorycardBestlapReplayFile 'B', MemorycardSettingFile 'S'
 *     +- MemorycardDummyFile (0x68) - MemorycardDeletedFile (0x68)
 *     +- MemorycardDamageFile (0x68)      unreadable, or an unknown GT3 kind
 *   pList (T495) - List<MemorycardFile> - MemorycardFileList
 * (sizes: the table 0x0031B780 MemorycardFileList uses for its cells and the
 * clones' operator new).
 *
 * The GT3 save names: "BASCUS-97102" + a type letter + 7 digits of a
 * base-64 alphabet ("A".."Z" "a".."z" "0".."9" "+" "-", g_00351754) holding
 * a 42-bit code, low digit first: bits 0..11 car index, 12..20 course index
 * (T489), 21..25 category, 26..29 sub-category, 30..41 a 12-bit number that
 * makes the name unique on the card (makeUniqueName).  The game data is
 * always "BASCUS-97102GAMEDATA".  A GT3 directory holds the data file (named
 * like the directory), the 0x60-byte "thumbnail" file, icon.sys (from T108's
 * template g_0049BEF0, menu/icon/icon.sys) and up to three icons
 * (menu/icon/ICON_*.ico).  Texts are EUC-JP in memory (the menu font),
 * Shift-JIS on the card.
 *
 * Destructors: MemorycardFile's is declared and empty, defined out of line
 * (its own vptr store is dead, and every subclass destructor calls the copy
 * 0x00142B18 instead of expanding it: an implicit one would be inline);
 * those of MemorycardPS1File, MemorycardPS2File, MemorycardDummyFile,
 * MemorycardDeletedFile, MemorycardDamageFile and the four GT3 kinds store
 * no vptr and only call their base's, so they were implicit (the slot
 * comments give the out-of-line copies); MemorycardOtherPS2File's,
 * MemorycardGT3File's and MemorycardFileList's are declared.  T108 also holds the copies of the
 * inline template destructors List<MemorycardFile>::~List 0x00146AD8 and
 * Cell<MemorycardFile>::~Cell 0x00146B00 (menu_list.h) and their typeinfo
 * functions.
 *
 * The non-virtual accessors (MemorycardFile::retry .. getId,
 * MemorycardGT3File::setThumbnail / getThumbnail, MemorycardFileList::
 * setUnk24) are inline: Lupo 2 has them in .gnu.linkonce sections, and T109
 * expands retry, isSizeError, setSizeError and setUnk24.  T108 holds
 * out-of-line copies of them with no callers (0x00146B28, 0x001470B0,
 * 0x001470F8, 0x001476C0..0x001477C0, among the vtable functions).  So are
 * the one-line virtual functions (the clones, MemorycardFile's defaults,
 * the PS1 / PS2 / other-PS2 / dummy / damaged / GT3 ones): egcs emits them
 * at the end of T108, class by class in reverse definition order, each
 * class's inline members in reverse declaration order, then its typeinfo
 * function and implicit destructor.  That is retail's order
 * 0x00146AD8..0x00147870 only with the classes defined in the order below
 * (MemorycardDamageFile, MemorycardDummyFile and MemorycardDeletedFile
 * before MemorycardGT3File) and MemorycardFile's members declared as they
 * are; the same class order gives retail's vtables (0x0031B798..) and type
 * names (0x0031BFB8..). */
#ifndef GAME_T108_MEMORYCARD_FILE_H
#define GAME_T108_MEMORYCARD_FILE_H

#include "game/game.h"
#include "game/menu/framework/menu_list.h"                 /* bases gt2k_menu::Cell<T>, gt2k_menu::List<T> */
#include "engine/io/T501_memorycard_manager.h"   /* gt2k_menu::McDateTime, ::McDirEntry (mc_get_dir's entries) */

struct McIconModel;                         /* T077 (pointers only) */

namespace gt2k_menu {
class MemorycardIconSprites;                /* T072 (pointers only) */

/* McDateTime and ::McDirEntry (T501): the entries of mc_get_dir. */
typedef ::McDirEntry McFileEntry;

/* A save directory as the file manager knows it (MemorycardFile::dir). */
struct MemorycardDir {
    int  setTimeNow();                      /* 0x00142910 both dates of the entry := now (the RTC, BCD) */
    void setOldId();                        /* 0x00142998 a valid id becomes -0x400 - id */
    int  getOldId();                        /* 0x001429B8 -0x400 - id */

    int          id;                        /* 0x00 >= 0 the entry number of the last listing;
                                               -1 new (not on the card yet, isNewFile); -2 not
                                               reading (a dummy); <= -0x400 an old id (setOldId) */
    /*                                         0x04 (alignment: entry is 8-aligned) */
    ::McDirEntry entry;                     /* 0x08 the directory's own entry (attr +0x1C, name +0x24) */
};
EGCS_SIZE(gt2k_menu::MemorycardDir, 0x48);

/* icon.sys (sceMcIconSys, 0x3C4 bytes) without Reserve3: 0x1C4 bytes.  The
 * title is EUC-JP in memory; writeIconSys converts it to Shift-JIS. */
struct MemorycardIconSys {
    char head[4];                           /* 0x000 "PS2D" */
    u16  reserv1;                           /* 0x004 */
    u16  offsLF;                            /* 0x006 the byte offset of the title's second line */
    u32  reserv2;                           /* 0x008 */
    u32  transRate;                         /* 0x00C the background transparency (the icon's alpha) */
    int  bgColor[4][4];                     /* 0x010 the background corner colours (r, g, b, -) */
    f32  lightDir[3][4];                    /* 0x050 */
    f32  lightColor[3][4];                  /* 0x080 */
    f32  ambient[4];                        /* 0x0B0 */
    char titleName[0x44];                   /* 0x0C0 */
    char iconFile[3][0x40];                 /* 0x104 FnameView, FnameCopy, FnameDel: the list / copy /
                                               delete icons ("icon0.ico" ..) */
};
EGCS_SIZE(gt2k_menu::MemorycardIconSys, 0x1C4);

/* The first 0x200 bytes of a PS1 save (its header block). */
struct McPs1Header {
    char magic[2];                          /* 0x000 "SC" */
    u8   iconFlag;                          /* 0x002 0x11..0x13: the low nibble is the icon's frames */
    u8   blocks;                            /* 0x003 */
    char title[0x40];                       /* 0x004 Shift-JIS on the card, EUC-JP once read */
    u32  titleEnd;                          /* 0x044 zeroed after reading */
    u8   unk_48[0x18];                      /* 0x048 */
    u16  clut[16];                          /* 0x060 */
    u8   icon[3][0x80];                     /* 0x080 16 x 16 4-bit frames */
};
EGCS_SIZE(gt2k_menu::McPs1Header, 0x200);

} /* namespace gt2k_menu */

#include "game/menu/memorycard/T108_game_thumbnail.h"         /* gt2k_game::GameThumbnail (defined here before) */

namespace gt2k_menu {

/* What MemorycardFile::render draws with.  The built-in icons are drawn by
 * copying x .. a into sprites (+0xE00) and calling
 * MemorycardIconSprites::draw(n) (T072; inline in every render). */
struct MemorycardIconRenderInfo {
    f32 x;                                  /* 0x00 centre */
    f32 y;                                  /* 0x04 */
    f32 w;                                  /* 0x08 full size */
    f32 h;                                  /* 0x0C */
    f32 r;                                  /* 0x10 colour */
    f32 g;                                  /* 0x14 */
    f32 b;                                  /* 0x18 */
    f32 a;                                  /* 0x1C */
    MemorycardIconSprites *sprites;         /* 0x20 */
    f32 time;                               /* 0x24 the animation time (s) */
    int useSlot0;                           /* 0x28 > 0: T077's slot-0 model if there is one */
};

/* The progress callback of the long operations: rate 0..1, ctx as given. */
typedef void (*MemorycardProgressFunc)(f32 rate, void *ctx);

/* vtable 0x0031BEF8 (19 slots: [1] [2] pCell's).  typeinfo
 * __tfQ29gt2k_menu14MemorycardFile 0x001477D0 (si, base
 * Cell<MemorycardFile>; storage 0x004B1910).  vptr +0x08 (pCell's); size
 * 0x68. */
class MemorycardFile : public Cell<MemorycardFile> {
public:
    MemorycardFile();                       /* 0x001429C8 a file not on the card yet */
    MemorycardFile(const MemorycardDir &d); /* 0x00142A08 a directory found on the card */
    MemorycardFile(const MemorycardFile &f);    /* 0x00142A88 the copy is not linked */
    /* Empty: Cell<MemorycardFile>'s vptr, then pCell::~pCell. */
    virtual ~MemorycardFile();              /* [0] +0x0C 0x00142B18 */
    /* The inline members (see the top of the file; the addresses are T108's
     * copies) in the order of declaration that gives retail's order of the
     * copies: prepareRelease, getId .. isSizeInvalid, infoValid, isNewFile
     * .. isSelectable, isPrepareIcon .. loadFromVolume, retry. */
    virtual void prepareRelease() {}        /* [3] +0x24 0x001477C8 */
    int  getId() { return dir.id; }                             /* 0x001477C0 */
    MemorycardDir *getDir() { return &dir; }                    /* 0x001477B8 */
    int  getSize() { return sizeKB; }                           /* 0x001477B0 */
    void invalidateSize() { sizeKB = -1; }                      /* 0x001477A0 */
    void setSizeError() { sizeKB = -2; }                        /* 0x00147790 */
    int  isSizeError() { return sizeKB == -2; }                 /* 0x00147778 */
    void setSizeInvalid() { sizeKB = -3; }                      /* 0x00147768 */
    int  isSizeInvalid() { return sizeKB == -3; }               /* 0x00147750 */
    virtual MemorycardFile *clone() = 0;    /* [4] +0x2C */
    virtual int  infoValid() { return infoRead; }               /* [5] +0x34 0x00147748 */
    int  isNewFile() { return dir.id == -1; }                   /* 0x00147738 */
    int  isNotReading() { return dir.id == -2; }                /* 0x00147720 */
    int  isSelectable() { return selectable; }                  /* 0x00147718 */
    /* selectable = the file passes the screen's filter: its type letter
     * (filter 0: any) and course / car / category / sub-category (0: any).
     * Here: filter == 0. */
    virtual void checkSelectable(int filter, u64 course, u64 car, int category,
                                 int subCategory);  /* [6] +0x3C 0x00142BC0 */
    virtual char *getTitle() = 0;           /* [7] +0x44 */
    virtual int  getTitleFirstLineBytes() = 0;  /* [8] +0x4C */
    virtual int  readInfo(int port) = 0;    /* [9] +0x54 */
    virtual int  isPrepareIcon() { return 1; }                  /* [10] +0x5C 0x00147710 */
    virtual int  isAllocIconMemory() { return 0; }              /* [11] +0x64 0x00147708 */
    /* which: 0 the list icon, 1 the copy, 2 the delete icon of icon.sys. */
    virtual int  readIcon(int port, int which) { return 1; }    /* [12] +0x6C 0x00147700 */
    virtual void releaseIcon(bool noWait) {}                    /* [13] +0x74 0x001476F8 */
    virtual void render(MemorycardIconRenderInfo &info) {}      /* [14] +0x7C 0x001476F0 */
    virtual int  save(int port, void *data, int size, bool format,
                      MemorycardProgressFunc progress, void *ctx) { return 1; }   /* [15] +0x84 0x001476E8 */
    /* The data file in a new buffer (memalign 64), NULL on failure. */
    virtual void *load(int port, MemorycardProgressFunc progress, void *ctx) { return 0; }    /* [16] +0x8C 0x001476E0 */
    /* The same from the disc volume (file_read_blocking; the demo replays). */
    virtual void *loadFromVolume(MemorycardProgressFunc progress, void *ctx) { return 0; } /* [17] +0x94 0x001476D8 */
    /* The layer of the file's info panel. */
    virtual const char *getInfoName();      /* [18] +0x9C 0x00142BB0 "info_other" */

    char *getMainFilePath(char *buf);       /* 0x00142B40 "/NAME/NAME"; returns buf */
    /* The files of the directory without "." and ".." (at most 20 asked
     * for); their count, -1 if it cannot be listed. */
    int  getFileList(int port, McFileEntry *list);  /* 0x00142BD8 */
    int  updateSize(int port);              /* 0x00142D30 calcSize into sizeKB; nonzero on success */
    int  calcSize(int port, int *kb);       /* 0x00142D50 the files rounded up to 1 KB, plus the
                                               directory; 0 if it cannot be listed */
    int  getMainFileKB(int port);           /* 0x00142DE8 -1 if the main file is not there */
    int  deleteDir(int port, MemorycardProgressFunc progress, void *ctx);  /* 0x00142E50 0: error */
    /* Copies the save from port src to port dst (formatting dst first, or
     * deleting the save already there when replace); 0: error. */
    int  copyTo(int src, int dst, int replace, int format,
                MemorycardProgressFunc progress, void *ctx);    /* 0x00143008 */
    int  exists(int port);                  /* 0x00143360 1 on the card, 0 not, -1 error */
    /* Another read may be tried: counts the failed reads, 5 allowed (Lupo 2
     * names it allowError). */
    int  retry() { return readCount++ < 5; }                    /* 0x001476C0 */

    /*            pCell                        0x00 (next, prev, vptr +0x08) */
    /*                                         0x0C (alignment: dir is 8-aligned) */
    MemorycardDir dir;                      /* 0x10 */
    int           sizeKB;                   /* 0x58 1 KB clusters on the card; -1 unknown, -2 error,
                                               -3 invalid */
    int           infoRead;                 /* 0x5C readInfo succeeded (infoValid) */
    int           selectable;               /* 0x60 checkSelectable */
    int           readCount;                /* 0x64 failed reads (retry) */
};
EGCS_SIZE(gt2k_menu::MemorycardFile, 0x68);

/* vtable 0x0031BE58 (19 slots).  typeinfo __tfQ29gt2k_menu17MemorycardPS1File
 * 0x00147650 (si; storage 0x004B18C0).  Size 0x270.
 * (clone: GCC 14 would warn -Waligned-new in every file that includes this
 * one: the header is 16-aligned, and the original uses plain operator new
 * too.  An inline member's body is compiled at the end of its class, so the
 * pragmas enclose the class.) */
#ifndef BOOT_OLD_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waligned-new="
#endif
class MemorycardPS1File : public MemorycardFile {
public:
    MemorycardPS1File();                    /* 0x001433C0 */
    MemorycardPS1File(const MemorycardDir &d);  /* 0x001433F8 */
    /* [0] +0x0C 0x001476A0 the implicit destructor (~MemorycardFile). */
    /* (inline, see the top of the file: T108's copies) the implicit copy
     * constructor */
    virtual MemorycardFile *clone() { return new MemorycardPS1File(*this); }   /* [4] +0x2C 0x00147530 */
    virtual char *getTitle() { return header.title; }           /* [7] +0x44 0x00147528 */
    virtual int  getTitleFirstLineBytes() { return 0x40; }      /* [8] +0x4C 0x00147520 */
    virtual int  readInfo(int port);        /* [9] +0x54 0x00143430 the header block of "/NAME/NAME" */
    virtual void render(MemorycardIconRenderInfo &info);    /* [14] +0x7C 0x001434C8 the header's
                                               icon (frame time / 0.17); without info built-in icon 6 */

    /*          MemorycardFile                 0x00 .. 0x68 */
    /*                                         0x68 (alignment: header is 16-aligned; the copy in
                                               clone copies 0x70.. only) */
    McPs1Header header __attribute__((aligned(16)));   /* 0x70 */
};
#ifndef BOOT_OLD_GCC
#pragma GCC diagnostic pop
#endif
EGCS_SIZE(gt2k_menu::MemorycardPS1File, 0x270);

/* vtable 0x0031BDB8 (19 slots).  typeinfo __tfQ29gt2k_menu17MemorycardPS2File
 * 0x001474B0 (si; storage 0x004B1900).  Size 0x230. */
class MemorycardPS2File : public MemorycardFile {
public:
    MemorycardPS2File();                    /* 0x00143738 */
    MemorycardPS2File(const MemorycardDir &d);  /* 0x00143770 the title cleared */
    MemorycardPS2File(const MemorycardPS2File &f);  /* 0x001437A8 */
    /* [0] +0x0C 0x00147500 the implicit destructor (~MemorycardFile). */
    virtual char *getTitle() { return iconSys.titleName; }      /* [7] +0x44 0x001474A8 (inline) */
    virtual int  getTitleFirstLineBytes();  /* [8] +0x4C 0x001438A0 iconSys.offsLF if 2..0x20, else 64 */
    virtual int  readInfo(int port);        /* [9] +0x54 0x00143A70 "/NAME/icon.sys" (the title to
                                               EUC-JP, packTitle) */

    void packTitle();                       /* 0x00143908 the title's blank runs squeezed (put_spaces) */
    int  writeIconSys(int port);            /* 0x00143B50 "/NAME/icon.sys", 0x3C4 bytes, the title in
                                               Shift-JIS; 1 = ok */
    /* Copies the volume files `files' (NULL-terminated, at most 3) to
     * "/NAME/icon<i>.ico" and names them in iconSys; 1 = ok. */
    int  writeIcons(int port, const char **files);  /* 0x00143C18 */
    int  makeDir(int port);                 /* 0x00143DE8 "/NAME"; 1 = ok */
    void setTitle(const char *text);        /* 0x00143E30 a '\n' sets offsLF */

    /*                MemorycardFile           0x000 .. 0x068 */
    MemorycardIconSys iconSys;              /* 0x068 */
    u8                unk_22C[4];           /* 0x22C (alignment; declared so that GCC 14 does not
                                               put the subclasses' members in it) */
};
EGCS_SIZE(gt2k_menu::MemorycardPS2File, 0x230);

/* A PS2 save of another game: shows its own icon.  vtable 0x0031BD18 (19
 * slots).  typeinfo __tfQ29gt2k_menu22MemorycardOtherPS2File 0x00147458
 * (si; storage 0x004B1940).  Size 0x238. */
class MemorycardOtherPS2File : public MemorycardPS2File {
public:
    MemorycardOtherPS2File();               /* 0x00143EC0 */
    MemorycardOtherPS2File(const MemorycardDir &d); /* 0x00143F00 */
    MemorycardOtherPS2File(const MemorycardOtherPS2File &f);   /* 0x00143F40 shares the icon (one more
                                               reference) */
    virtual ~MemorycardOtherPS2File();      /* [0] +0x0C 0x00143FA0 releaseIcon(false) */
    virtual void prepareRelease();          /* [3] +0x24 0x00144228 the icon stops being drawn */
    /* (inline: T108's copies) the copy shares the icon */
    virtual MemorycardFile *clone() { return new MemorycardOtherPS2File(*this); }  /* [4] +0x2C 0x001473F0 */
    virtual int  isPrepareIcon() { return iconPrepared; }       /* [10] +0x5C 0x001473E8 */
    virtual int  isAllocIconMemory() { return icon != 0; }      /* [11] +0x64 0x001473D8 */
    /* Icon file `which' of icon.sys (0: kept in icon; 1 / 2: a model T077
     * keeps) with icon.sys's background and lights. */
    virtual int  readIcon(int port, int which); /* [12] +0x6C 0x00143FF8 */
    virtual void releaseIcon(bool noWait);  /* [13] +0x74 0x00144240 icon_model_release (T077) */
    virtual void render(MemorycardIconRenderInfo &info);    /* [14] +0x7C 0x00144280 the icon once
                                               ready (useSlot0: T077's slot 0); none: built-in icon 5 */

    /*           MemorycardPS2File             0x000 .. 0x230 */
    McIconModel *icon;                      /* 0x230 the list icon (reference counted, T077) */
    int          iconPrepared;              /* 0x234 isPrepareIcon */
};
EGCS_SIZE(gt2k_menu::MemorycardOtherPS2File, 0x238);

/* A directory that could not be read, or a GT3 name of an unknown kind.
 * vtable 0x0031BC78 (19 slots).  typeinfo
 * __tfQ29gt2k_menu20MemorycardDamageFile 0x00147368 (si; storage
 * 0x004B18A0).  Size 0x68. */
class MemorycardDamageFile : public MemorycardFile {
public:
    MemorycardDamageFile();                 /* 0x00144368 */
    MemorycardDamageFile(const MemorycardDir &d);   /* 0x001443A0 attr |= 8: never offered for copying */
    /* [0] +0x0C 0x001473B8 the implicit destructor (~MemorycardFile). */
    /* (inline: T108's copies) */
    virtual MemorycardFile *clone() { return new MemorycardDamageFile(*this); }    /* [4] +0x2C 0x001472F0 */
    virtual char *getTitle();               /* [7] +0x44 0x001443E8 g_002B1C8C, "damaged data" */
    virtual int  getTitleFirstLineBytes() { return 0x40; }      /* [8] +0x4C 0x001472E8 */
    virtual int  readInfo(int port) { infoRead = 1; return 1; } /* [9] +0x54 0x001472D8 */
    virtual void render(MemorycardIconRenderInfo &info);    /* [14] +0x7C 0x001443F8 built-in icon 7 */
};
EGCS_SIZE(gt2k_menu::MemorycardDamageFile, 0x68);

/* A placeholder entry (id -2): a line whose file is not loaded yet
 * (MemorycardFileManager::getFile; built-in icon 9 "icon_reading").  vtable 0x0031BBD8 (19
 * slots).  typeinfo __tfQ29gt2k_menu19MemorycardDummyFile 0x00147268 (si;
 * storage 0x004B1950).  Size 0x68 (MemorycardFileManager +0xFA90). */
class MemorycardDummyFile : public MemorycardFile {
public:
    MemorycardDummyFile();                  /* 0x00144460 */
    /* [0] +0x0C 0x001472B8 the implicit destructor (~MemorycardFile). */
    /* (inline: T108's copies; clone is also MemorycardDeletedFile's: a copy
     * of a deleted file is a dummy file) */
    virtual MemorycardFile *clone() { return new MemorycardDummyFile(*this); }     /* [4] +0x2C 0x001471F0 */
    virtual char *getTitle();               /* [7] +0x44 0x001444E0 "" */
    virtual int  getTitleFirstLineBytes() { return 0x20; }      /* [8] +0x4C 0x001471E8 */
    virtual int  readInfo(int port) { return 1; }               /* [9] +0x54 0x001471E0 */
    virtual void render(MemorycardIconRenderInfo &info);    /* [14] +0x7C 0x001444F0 built-in icon 9 */
    virtual const char *getInfoName();      /* [18] +0x9C 0x00144558 NULL */
};
EGCS_SIZE(gt2k_menu::MemorycardDummyFile, 0x68);

/* vtable 0x0031BB38 (19 slots; [4] clone is MemorycardDummyFile's).
 * typeinfo __tfQ29gt2k_menu21MemorycardDeletedFile 0x00147170 (si; storage
 * 0x004B18F0).  Size 0x68 (MemorycardFileManager +0xFAF8).  Its constructor
 * is implicit (inline in T109). */
class MemorycardDeletedFile : public MemorycardDummyFile {
public:
    /* [0] +0x0C 0x001471C0 the implicit destructor (~MemorycardFile). */
    virtual void render(MemorycardIconRenderInfo &info);    /* [14] +0x7C 0x00144560 built-in icon 12 */
};
EGCS_SIZE(gt2k_menu::MemorycardDeletedFile, 0x68);

/* A GT3 save.  vtable 0x0031BA88 (21 slots: [4] [19] [20] pure).  typeinfo
 * __tfQ29gt2k_menu17MemorycardGT3File 0x00147120 (si; storage 0x004B18E0).
 * Size 0x298. */
class MemorycardGT3File : public MemorycardPS2File {
public:
    MemorycardGT3File();                    /* 0x001445C8 a new save: icon.sys g_0049BEF0,
                                               thumbnailValid 1 */
    MemorycardGT3File(const MemorycardDir &d);  /* 0x001446C0 found on the card: icon.sys
                                               g_0049BEF0, thumbnailValid 0 until readInfo */
    virtual ~MemorycardGT3File();           /* [0] +0x0C 0x001447B0 ~MemorycardFile */
    /* Both the information and the thumbnail have been read. */
    virtual int  infoValid() { return infoRead != 0 && thumbnailValid != 0; }    /* [5] +0x34 0x00147100 (inline) */
    virtual int  readInfo(int port);        /* [9] +0x54 0x00144E38 the thumbnail file */
    virtual void *load(int port, MemorycardProgressFunc progress, void *ctx);    /* [16] +0x8C 0x001453D8 */
    virtual void *loadFromVolume(MemorycardProgressFunc progress, void *ctx); /* [17] +0x94 0x00145508 */
    /* The icon files of the kind, NULL-terminated (.sdata g_00351758 ..). */
    virtual const char **getIconFiles() = 0;    /* [19] +0xA4 */
    /* The low 30 bits of the name code: car index | course << 12 |
     * category << 21 | sub-category << 26 (from the thumbnail).  An s64
     * (the code is 42 bits): the implementations compute an int and
     * sign-extend it (retail's final move), makeUniqueName masks it as a
     * 64-bit value. */
    virtual s64  getNameCode() = 0;         /* [20] +0xAC */

    /* A new name of kind `type' (the code plus a 12-bit number not listed
     * on the card); 1 = done. */
    int   makeUniqueName(int port, char type);  /* 0x00144B90 */
    char *getThumbnailPath(char *buf);      /* 0x00144DE0 "/NAME/thumbnail"; returns buf */
    void  calcSize(int dataSize);           /* 0x00144EB0 sizeKB of a save of dataSize bytes */
    /* (format: the card first;) a new save: the directory, the icons and
     * icon.sys; newName: a fresh name (an old save is renamed); then the
     * data file and the thumbnail, with progress.  1 = done. */
    int   saveFiles(int port, void *data, int dataSize, char type, bool format,
                    MemorycardProgressFunc progress, void *ctx, int newName);  /* 0x00144F90 */
    /* A new comment: rewrites the thumbnail file only; 1 = done. */
    int   changeComment(int port, const char *comment,
                        MemorycardProgressFunc progress, void *ctx);    /* 0x00145590 */
    /* inline (see the top of the file; getThumbnail first: retail's order of
     * the copies); Lupo 2: setThumbnail(gt2k_game::GameThumbnail &) */
    gt2k_game::GameThumbnail *getThumbnail() { return &thumbnail; }        /* 0x001470F8 */
    void  setThumbnail(gt2k_game::GameThumbnail &t) { thumbnail = t; }    /* 0x001470B0 */

    /*                  MemorycardPS2File      0x000 .. 0x230 */
    gt2k_game::GameThumbnail thumbnail;     /* 0x230 */
    int                 thumbnailValid;     /* 0x290 the thumbnail file was read (or the save is new) */
};
EGCS_SIZE(gt2k_menu::MemorycardGT3File, 0x298);

/* Game data 'G', "BASCUS-97102GAMEDATA".  vtable 0x0031B9D8 (21 slots).
 * typeinfo __tfQ29gt2k_menu18MemorycardGameFile 0x00147040 (si; storage
 * 0x004B1960).  Size 0x298. */
class MemorycardGameFile : public MemorycardGT3File {
public:
    MemorycardGameFile();                   /* 0x00145648 */
    MemorycardGameFile(const MemorycardDir &d); /* 0x001456D8 */
    static void makeSearchPattern(char *buf);   /* 0x00145710 */
    /* [0] +0x0C 0x00147090 the implicit destructor (~MemorycardGT3File). */
    /* (inline: T108's copy) the implicit copy constructors: MemorycardPS2File's
     * (0x001437A8), the thumbnail and thumbnailValid */
    virtual MemorycardFile *clone() { return new MemorycardGameFile(*this); }      /* [4] +0x2C 0x00146F78 */
    virtual void checkSelectable(int filter, u64 course, u64 car, int category,
                                 int subCategory);  /* [6] +0x3C 0x00145878 'G' or 0 */
    virtual void render(MemorycardIconRenderInfo &info);    /* [14] +0x7C 0x00145730 */
    virtual int  save(int port, void *data, int size, bool format,
                      MemorycardProgressFunc progress, void *ctx);   /* [15] +0x84 0x001457B8 */
    virtual const char *getInfoName();      /* [18] +0x9C 0x00145798 "info_game" */
    virtual const char **getIconFiles();    /* [19] +0xA4 0x001457A8 */
    virtual s64  getNameCode();             /* [20] +0xAC 0x00145870 */
};
EGCS_SIZE(gt2k_menu::MemorycardGameFile, 0x298);

/* Replay 'R'.  vtable 0x0031B928 (21 slots).  typeinfo
 * __tfQ29gt2k_menu20MemorycardReplayFile 0x00146F08 (si; storage
 * 0x004B18D0).  Size 0x298. */
class MemorycardReplayFile : public MemorycardGT3File {
public:
    MemorycardReplayFile();                 /* 0x001458A0 */
    MemorycardReplayFile(const MemorycardDir &d);   /* 0x001458D8 */
    static void makeSearchPattern(char *buf);   /* 0x00145910 */
    /* [0] +0x0C 0x00146F58 the implicit destructor (~MemorycardGT3File). */
    virtual MemorycardFile *clone() { return new MemorycardReplayFile(*this); }    /* [4] +0x2C 0x00146E40 (inline) */
    virtual void checkSelectable(int filter, u64 course, u64 car, int category,
                                 int subCategory);  /* [6] +0x3C 0x00145B20 */
    virtual void render(MemorycardIconRenderInfo &info);    /* [14] +0x7C 0x00145930 */
    virtual int  save(int port, void *data, int size, bool format,
                      MemorycardProgressFunc progress, void *ctx);   /* [15] +0x84 0x001459B8 */
    virtual const char *getInfoName();      /* [18] +0x9C 0x00145998 */
    virtual const char **getIconFiles();    /* [19] +0xA4 0x001459A8 */
    virtual s64  getNameCode();             /* [20] +0xAC 0x00145AC0 */
};
EGCS_SIZE(gt2k_menu::MemorycardReplayFile, 0x298);

/* Best lap replay 'B' (a licence replay for categories 9..14).  vtable
 * 0x0031B878 (21 slots).  typeinfo
 * __tfQ29gt2k_menu27MemorycardBestlapReplayFile 0x00146DD0 (si; storage
 * 0x004B1920).  Size 0x298. */
class MemorycardBestlapReplayFile : public MemorycardGT3File {
public:
    MemorycardBestlapReplayFile();          /* 0x00145B98 */
    MemorycardBestlapReplayFile(const MemorycardDir &d);    /* 0x00145BD0 */
    static void makeSearchPattern(char *buf);   /* 0x00145C08 */
    /* [0] +0x0C 0x00146E20 the implicit destructor (~MemorycardGT3File). */
    virtual MemorycardFile *clone() { return new MemorycardBestlapReplayFile(*this); }  /* [4] +0x2C 0x00146D08 (inline) */
    virtual void checkSelectable(int filter, u64 course, u64 car, int category,
                                 int subCategory);  /* [6] +0x3C 0x00145E68 */
    virtual void render(MemorycardIconRenderInfo &info);    /* [14] +0x7C 0x00145C28 */
    virtual int  save(int port, void *data, int size, bool format,
                      MemorycardProgressFunc progress, void *ctx);   /* [15] +0x84 0x00145CD8 */
    virtual const char *getInfoName();      /* [18] +0x9C 0x00145CA8 */
    virtual const char **getIconFiles();    /* [19] +0xA4 0x00145CB8 */
    virtual s64  getNameCode();             /* [20] +0xAC 0x00145E08 */
};
EGCS_SIZE(gt2k_menu::MemorycardBestlapReplayFile, 0x298);

/* Car setting 'S'.  vtable 0x0031B7C8 (21 slots).  typeinfo
 * __tfQ29gt2k_menu21MemorycardSettingFile 0x00146C98 (si; storage
 * 0x004B18B0).  Size 0x298. */
class MemorycardSettingFile : public MemorycardGT3File {
public:
    MemorycardSettingFile();                /* 0x00145EE8 */
    MemorycardSettingFile(const MemorycardDir &d);  /* 0x00145F20 */
    static void makeSearchPattern(char *buf);   /* 0x00145F58 */
    /* [0] +0x0C 0x00146CE8 the implicit destructor (~MemorycardGT3File). */
    virtual MemorycardFile *clone() { return new MemorycardSettingFile(*this); }   /* [4] +0x2C 0x00146BD0 (inline) */
    virtual void checkSelectable(int filter, u64 course, u64 car, int category,
                                 int subCategory);  /* [6] +0x3C 0x00146060 */
    virtual void render(MemorycardIconRenderInfo &info);    /* [14] +0x7C 0x00145F78 */
    virtual int  save(int port, void *data, int size, bool format,
                      MemorycardProgressFunc progress, void *ctx);   /* [15] +0x84 0x001460D8 */
    virtual const char *getInfoName();      /* [18] +0x9C 0x00145FE0 */
    virtual const char **getIconFiles();    /* [19] +0xA4 0x00145FF0 */
    virtual s64  getNameCode();             /* [20] +0xAC 0x00146000 */
};
EGCS_SIZE(gt2k_menu::MemorycardSettingFile, 0x298);

/* A List<MemorycardFile> whose cells are built in place in a pool of equal
 * slots (the largest file class, 16-aligned); when the pool is full the
 * file the score function rates highest is recycled.  vtable 0x0031B798
 * (2 slots: [1] pList::sort).  typeinfo
 * __tfQ29gt2k_menu18MemorycardFileList 0x00146B30 (si, base
 * List<MemorycardFile>; storage 0x004B1890).  Size 0x28
 * (MemorycardFileManager +0xFA64; the next member, 8-aligned, at +0xFA90). */
class MemorycardFileList : public List<MemorycardFile> {
public:
    MemorycardFileList(int count);          /* 0x001461E0 a pool of count cells */
    virtual ~MemorycardFileList();          /* [0] +0x0C 0x001462F8 releaseAll, the pool freed */

    void  buildFreeList();                  /* 0x00146358 */
    void  releaseAll();                     /* 0x001463A0 */
    MemorycardFile *findById(int id);       /* 0x00146470 dir.id == id */
    MemorycardFile *findByName(const char *name);   /* 0x001464C0 */
    void  markAll();                        /* 0x00146540 every dir.setOldId() */
    void  removeMarked();                   /* 0x00146598 releases the files still marked */
    void *allocCell();                      /* 0x00146610 a free slot, NULL if none */
    int   hasFreeCell();                    /* 0x00146630 */
    void  release(MemorycardFile *file);    /* 0x00146640 unlinked, destroyed (direct destructor
                                               call), its slot freed */
    void  setScoreFunc(int (*fn)(void *ctx, MemorycardFile *file), void *ctx);  /* 0x00146698 */
    MemorycardFile *findMaxScore();         /* 0x001466A8 */
    MemorycardFile *findMaxScoreWithIcon(); /* 0x00146750 among the files holding icon memory */
    /* A new file of the kind its name says, NULL without a cell. */
    MemorycardFile *add(const MemorycardDir &d);    /* 0x00146810 */
    void  walk();                           /* 0x001469B8 steps through the list (no effect) */
    /* Replaces file (released) by a MemorycardDamageFile; 0 without a cell. */
    int   setDamaged(MemorycardFile *file); /* 0x001469F0 */
    /* inline (see the top of the file): T109's setPort expands it; Lupo 2
     * names it setPort(int) */
    void  setUnk24(int v) { unk_24 = v; }   /* 0x00146B28 */

    /*        List<MemorycardFile>             0x00 (head, tail, vptr +0x08) */
    void     *pool;                         /* 0x0C memalign(16, cellSize * cellCount) */
    u32       cellSize;                     /* 0x10 the largest file class, 16-aligned */
    int       cellCount;                    /* 0x14 */
    void     *freeList;                     /* 0x18 the free slots, linked by their first word */
    int     (*scoreFn)(void *ctx, MemorycardFile *file);    /* 0x1C */
    void     *scoreCtx;                     /* 0x20 */
    int       unk_24;                       /* 0x24 the card id (T109 setPort, via setUnk24); not read */
};
EGCS_SIZE(gt2k_menu::MemorycardFileList, 0x28);

/* The text helpers (in place unless noted). */
void  sjis_to_euc(char *str);               /* 0x00142790 Shift-JIS -> EUC-JP */
void  euc_to_sjis(char *str);               /* 0x00142818 EUC-JP -> Shift-JIS */
/* Each character of src -> its full-width EUC-JP pair (table 0x003029E8). */
void  ascii_to_fullwidth(char *dst, const char *src);  /* 0x00142890 */
int   bcd_to_int(int bcd);                  /* 0x001428E0 */
/* width of a run of blanks (1 per ASCII space, 2 per full-width one):
 * writes (width + 3) / 4 ASCII spaces, returns the end. */
char *put_spaces(char *p, int width);       /* 0x001438B8 */

/* The GT3 save names (see the top of the file). */
void  mcInitIconSys();                      /* 0x001447D8 menu/icon/icon.sys -> g_0049BEF0 */
char *mcGetNamePrefix();                    /* 0x00144910 "BASCUS-97102" (g_002A2160, built once) */
char *mcGetNameSuffix(char *name);          /* 0x00144960 name + 12 (no callers) */
int   mcIsGT3Name(const char *name);        /* 0x00144968 starts with the prefix */
char  mcGetNameType(const char *name);      /* 0x001449C8 name[12]: 'G' 'R' 'B' 'S' */
void  mcMakeSearchPattern(char *buf, int type); /* 0x001449D0 "/BASCUS-97102<type>*" (type < 0:
                                               no letter) */
int   mcDecodeChar(char c);                 /* 0x00144A30 0..63, -1 outside the alphabet */
char  mcEncodeChar(int value);              /* 0x00144A70 */
s64   mcDecodeNameCode(const char *name);   /* 0x00144A80 the 42-bit code, -1 if invalid */
void  mcMakeName(char *name, char type, s64 code);  /* 0x00144B00 prefix + type + 7 digits */
int   mcGetNameCourse(const char *name);    /* 0x00144C80 the fields of the name code (-1: none) */
int   mcGetNameCar(const char *name);       /* 0x00144CB8 */
int   mcGetNameCategory(const char *name);  /* 0x00144CF0 */
int   mcGetNameSubCategory(const char *name);   /* 0x00144D28 */
int   mcCountFiles(int port, char type);    /* 0x00144D60 (no callers) */

} /* namespace gt2k_menu */

/* (GT3's icon.sys, the template of new saves, g_0049BEF0 0x0049BEF0: a
 * file static of T108, retail's .bss; only T108 uses it.) */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu18MemorycardFileList(void);               /* 0x00146B30 */
void *__tfQ29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile(void); /* 0x00146B80 */
void *__tfQ29gt2k_menu21MemorycardSettingFile(void);            /* 0x00146C98 */
void *__tfQ29gt2k_menu27MemorycardBestlapReplayFile(void);      /* 0x00146DD0 */
void *__tfQ29gt2k_menu20MemorycardReplayFile(void);             /* 0x00146F08 */
void *__tfQ29gt2k_menu18MemorycardGameFile(void);               /* 0x00147040 */
void *__tfQ29gt2k_menu17MemorycardGT3File(void);                /* 0x00147120 */
void *__tfQ29gt2k_menu21MemorycardDeletedFile(void);            /* 0x00147170 */
void *__tfQ29gt2k_menu19MemorycardDummyFile(void);              /* 0x00147268 */
void *__tfQ29gt2k_menu20MemorycardDamageFile(void);             /* 0x00147368 */
void *__tfQ29gt2k_menu22MemorycardOtherPS2File(void);           /* 0x00147458 */
void *__tfQ29gt2k_menu17MemorycardPS2File(void);                /* 0x001474B0 */
void *__tfQ29gt2k_menu17MemorycardPS1File(void);                /* 0x00147650 */
void *__tfQ29gt2k_menu14MemorycardFile(void);                   /* 0x001477D0 */
void *__tfQ29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile(void); /* 0x00147820 */
}
#endif

#endif /* GAME_T108_MEMORYCARD_FILE_H */
