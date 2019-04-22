#define Uses_TScreen
#define Uses_THardwareInfo
#include <tvision/tv.h>

#include <ncurses.h>
#include <clocale>
#include <unordered_map>
using std::unordered_map;

static bool hasColors;

THardwareInfo::THardwareInfo()
{
    // Allow printing UTF-8 text.
    setlocale(LC_ALL, "");
    // Start curses mode.
    initscr();
    /* Capture keyboard input, but allow exiting with Ctrl+C while we can't
     * do it from within the application.
     */
    cbreak();
    keypad(stdscr, true);
    // Disable echoing of pressed keys.
    noecho();
    // Enable colors if the terminal supports it.
    hasColors = getScreenMode() & TDisplay::smCO80;
    if (hasColors)
        start_color();
}

THardwareInfo::~THardwareInfo() {
    // End curses mode when exiting the application.
    endwin();
}

void THardwareInfo::setCaretPosition( ushort x, ushort y )
{
    // Note that [y, x] = [row, column]
    wmove(stdscr, y, x);
}

ushort THardwareInfo::getScreenMode()
{
/* The original implementation just reads the video mode, and sets the small font
 * if the number of rows is greater than 25.
 * This function is called from TDisplay::getCrtMode, which just returns the
 * value this does, and it is then assigned to a attribute of TScreen.
 *
 * For the time being, we will only use the video mode to tell Turbo Vision
 * to use color or monochrome formatting according to the terminal capabilities.
 * The video mode does not determine the number of rows or columns used.
 */
    return has_colors() ? TDisplay::smCO80 : TDisplay::smMono;
}

void THardwareInfo::setCaretSize( ushort size )
{
/* The caret is the keyboard cursor. If size is 0, the caret is hidden. The
 * other possible values are from 1 to 100, theoretically, and represent the
 * percentage of the character cell the caret fills.
 * https://docs.microsoft.com/en-us/windows/console/console-cursor-info-str
 *
 * ncurses supports only three levels: invisible (0), normal (1) and
 * very visible (2). They don't make a difference in all terminals, but
 * we can try mapping them to the values requested by Turbo Vision.
 */
    curs_set(size > 0 ? size == 100 ? 2 : 1 : 0);
}

/* The Turbo Vision library has all its characters encoded in code page 437.
 * Until unicode support is not added, it's better to just translate them
 * with a lookup table. The following table allows translating the characters
 * stored by Turbo Vision into the corresponding UTF-8 mulibyte characters.
 * Taken from: https://en.wikipedia.org/wiki/Code_page_437
 */

const char* cp437toUtf8[256] = {
    "\0", "☺", "☻", "♥", "♦", "♣", "♠", "•", "◘", "○", "◙", "♂", "♀", "♪", "♫", "☼",
    "►", "◄", "↕", "‼", "¶", "§", "▬", "↨", "↑", "↓", "→", "←", "∟", "↔", "▲", "▼",
    " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
    "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
    "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_",
    "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~", "⌂",
    "Ç", "ü", "é", "â", "ä", "à", "å", "ç", "ê", "ë", "è", "ï", "î", "ì", "Ä", "Å",
    "É", "æ", "Æ", "ô", "ö", "ò", "û", "ù", "ÿ", "Ö", "Ü", "¢", "£", "¥", "₧", "ƒ",
    "á", "í", "ó", "ú", "ñ", "Ñ", "ª", "º", "¿", "⌐", "¬", "½", "¼", "¡", "«", "»",
    "░", "▒", "▓", "│", "┤", "╡", "╢", "╖", "╕", "╣", "║", "╗", "╝", "╜", "╛", "┐",
    "└", "┴", "┬", "├", "─", "┼", "╞", "╟", "╚", "╔", "╩", "╦", "╠", "═", "╬", "╧",
    "╨", "╤", "╥", "╙", "╘", "╒", "╓", "╫", "╪", "┘", "┌", "█", "▄", "▌", "▐", "▀",
    "α", "ß", "Γ", "π", "Σ", "σ", "µ", "τ", "Φ", "Θ", "Ω", "δ", "∞", "φ", "ε", "∩",
    "≡", "±", "≥", "≤", "⌠", "⌡", "÷", "≈", "°", "∙", "·", "√", "ⁿ", "²", "■", " "
};

/* Turbo Vision stores char/attribute information in a CHAR_INFO struct:
 * https://docs.microsoft.com/en-us/windows/console/char-info-str
 * The lower 16 bits store the char value and the higher 16 bits store the
 * character attributes. Turbo Vision usually only fills the lower 8 bits
 * of each part.
 */

/* From the 8 bits used for attributes, the lower half defines the foreground
 * color while the upper half defines the background color. Each color is
 * defined as Intensity-Red-Green-Blue, while ncurses can handle 4-bit colors
 * in the format Bold-Blue-Green-Red.
 *
 * Boldness and Intensity are the same except for the fact that terminals
 * can only apply the bold attribute to the foreground color.
 *
 * Some examples here:
 * https://www.linuxjournal.com/content/about-ncurses-colors-0
 */

/* The best way to use colors in ncurses is to define <foreground, background>
 * color pairs. In order to do this in terminals with limited color support
 * (such as the linux console), it's best to start counting the pairs from one.
 * Pair number 0 is reserved for default-colored text.
 */

static unordered_map<ushort, short> pairDefinitions;
ushort definedPairs = 0;

static void defineColorPairIfNecessary(uchar attr);
static uchar swapRedBlue (uchar c);

void THardwareInfo::screenWrite( ushort x, ushort y, ushort *buf, DWORD len )
{
    setCaretPosition(x, y);
    // It takes two shorts to store a char and its attributes;
    for (int i = 0; i < 2*len; i += 2) {
        /* Both of these fields are 16-bit in theory, but Turbo Vision only
         * uses the lower 8. */
        uchar character = buf[i];
        uchar attr = buf[i + 1];
        /* The bold attribute is the left-most bit of the foreground color.
         * The intensity attribute of the background color is discarded. */
        bool bold = attr & 0x08;
        /* Discard intensity attributes when identifying color pairs, because
         * the bold attribute is used separately and does not define pairs. */
        uchar pairKey = attr & 0x77;
        if (hasColors) {
            defineColorPairIfNecessary(pairKey);
            wattron(stdscr, COLOR_PAIR(pairDefinitions[pairKey]));
        }
        // Monochrome terminals might still support bold text.
        if (bold)
            wattron(stdscr, A_BOLD);
        // Print a single character, which might be multi-byte in UTF-8.
        wprintw(stdscr, "%s", cp437toUtf8[character]);
        /* A future possible optimization would be not to repeat attribute
         * setting and unsetting when unnecessary. */
        if (hasColors)
            wattroff(stdscr, COLOR_PAIR(pairDefinitions[pairKey]));
        if (bold)
            wattroff(stdscr, A_BOLD);
    }
    /* Print to the screen, although this is not strictly necessary because
     * it gets later done automatically when waiting for user input in getch(). */
    wrefresh(stdscr);
}

uchar swapRedBlue (uchar c) {
    /* Swap the Red and Blue bits of a color encoded in the Microsoft way, so that
     * we can easily pass it to ncurses later.
     */
    return (c & ~0x5) | ((c & 0x4) >> 2) | ((c & 0x1) << 2);
}

void defineColorPairIfNecessary(uchar pairKey) {
    /* Color pairs are defined as they are used, in order not to make
     * any assumptions on the amount of color pairs supported by the terminal.
     */
    if (!pairDefinitions[pairKey]) {
        uchar fgColor = swapRedBlue (0x07 & pairKey),
              bgColor = swapRedBlue (pairKey >> 4);
        init_pair(++definedPairs, fgColor, bgColor);
        pairDefinitions[pairKey] = definedPairs;
    }
}