/* T227 - the text input page.  0x001C1B88..0x001C1CC8.
 *
 * Called by T097 NameEntryControl::runPage and T068
 * MemorycardControl::fileOperation: hands the TextBox statics (T231) the
 * limit, the output buffer and a copy of the initial text, opens `page' and
 * runs it frame by frame until it is left.  In a frame where START is
 * pressed on the controller's pad the current list's cursor goes to button
 * 6 instead (? the keyboard page's end key; the .mbl decides). */
/* Retail's .rodata between T224's and T267's (0x00335220..0x00335366,
 * unreferenced) is type names only: SmartIndexT<int>, List / Cell<Button>,
 * OperatorList, List / Cell<OperatorHolder>, OperatorHolder, Operator, the
 * view of a file that saw the button list (T225 / T226 see no class).  Of
 * the three files between T224 and T267 only T227 uses the button list, so
 * the names are taken as T227's. */
// DATA_START 0x00335220
#include "game/menu/framework/T206_button.h"               /* (seen: Cell<Button>, T216's operator classes) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member. */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList::setCursor */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager */
#include "game/menu/framework/T231_text_box.h"             /* text_box_set_max_length / _output / _initial */
#include "game/menu/framework/T247_control_base.h"         /* ControlBase::getControl1 */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn, CONTROL_START */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "game/menu/framework/T227_text_input.h"
#include "game/menu/framework/menu_widgets_internal.h"

using namespace gt2k_menu;

enum { TEXT_INPUT_END_BUTTON = 6 };         /* (?) the cursor START jumps to */

// FUNC 0x001C1B88 run_text_input
int run_text_input(PageManager *pm, const char *page, const char *initial, char *out,
                   int maxLen, int (*abortFn)(void *), void *abortArg)
{
    char text[0x40];                        /* sp+0x00 */

    text_box_set_max_length(maxLen);
    text_box_set_output(out);
    strcpy(text, initial);
    text_box_set_initial(text);
    pm->setPage(page, 0);
    pm->startPage(0);
    while (pm->exited == 0) {
        ButtonList *list = pm->buttonLists[pm->popupLevel];
        /* through the pointer: retail calls getButtonsOn through the vtable */
        if (list != 0 && (pm->control->getControl1()->getButtonsOn() & CONTROL_START) != 0) {
            list->setCursor(TEXT_INPUT_END_BUTTON);
        } else {
            if (abortFn != 0 && abortFn(abortArg) != 0)
                pm->exitRequest = 1;        /* doExit(), expanded */
            pm->update();
            gfx_vsync(1);
        }
    }
    pm->closeAndExit();
    gfx_vsync(2);
    return pm->decided;
}
