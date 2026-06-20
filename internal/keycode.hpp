/** @file
 *  @brief Keycode enum covering the full Unicode range and SDL scancodes.
 */

#pragma once

#include <cstdint>

namespace cppreact {

  /** @brief Enum of all keyboard keycodes.
   *
   *  Values 0x000000–0x10FFFF are Unicode codepoints.
   *  Non-printable keys are at 0x110000 + SDL_Scancode,
   *  mirroring SDL's 0x40000000 + SDL_Scancode convention.
   */
  enum class keycode : uint32_t {
    UNKNOWN = 0,

    // Printable ASCII — same as their Unicode codepoint
    SPACE     = ' ',   EXCLAIM   = '!',  DQUOTE  = '"',  HASH    = '#',
    DOLLAR    = '$',   PERCENT   = '%',  AMPER   = '&',  SQUOTE  = '\'',
    LPAREN    = '(',   RPAREN    = ')',  STAR    = '*',  PLUS    = '+',
    COMMA     = ',',   MINUS     = '-',  DOT     = '.',  SLASH   = '/',
    _0        = '0',   _1        = '1',  _2      = '2',  _3      = '3',
    _4        = '4',   _5        = '5',  _6      = '6',  _7      = '7',
    _8        = '8',   _9        = '9',
    COLON     = ':',   SEMICOLON = ';',  LESS    = '<',  EQUAL   = '=',
    GREATER   = '>',   QUESTION  = '?',  AT      = '@',
    A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G',
    H = 'H', I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N',
    O = 'O', P = 'P', Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U',
    V = 'V', W = 'W', X = 'X', Y = 'Y', Z = 'Z',
    LBRACKET   = '[', BACKSLASH  = '\\',RBRACKET  = ']',
    CARET      = '^', UNDERSCORE = '_', GRAVE    = '`',
    LBRACE     = '{', PIPE       = '|', RBRACE   = '}',
    TILDE      = '~',

    // Non-printable — at 0x110000 + SDL_Scancode offset
    ENTER     = 0x110028, ///< Enter/Return key
    ESCAPE    = 0x110029, ///< Escape key
    BACKSPACE = 0x11002A, ///< Backspace key
    TAB       = 0x11002B, ///< Tab key
    DEL       = 0x11004C, ///< Delete key

    // Modifier keys
    LCTRL  = 0x1100E0,  RCTRL  = 0x1100E4, ///< Left / Right Control
    LSHIFT = 0x1100E1,  RSHIFT = 0x1100E5, ///< Left / Right Shift
    LALT   = 0x1100E2,  RALT   = 0x1100E6, ///< Left / Right Alt
    LGUI   = 0x1100E3,  RGUI   = 0x1100E7, ///< Left / Right GUI (Windows/Command)

    // Arrow keys
    UP    = 0x110052, ///< Up arrow
    DOWN  = 0x110051, ///< Down arrow
    LEFT  = 0x110050, ///< Left arrow
    RIGHT = 0x11004F, ///< Right arrow

    // Function keys
    F1  = 0x11003A,  F2  = 0x11003B,  F3  = 0x11003C,  F4  = 0x11003D,
    F5  = 0x11003E,  F6  = 0x11003F,  F7  = 0x110040,  F8  = 0x110041,
    F9  = 0x110042,  F10 = 0x110043,  F11 = 0x110044,  F12 = 0x110045,

    // Numpad
    KP_0 = 0x110062,  KP_1 = 0x110059,  KP_2 = 0x11005A, KP_3 = 0x11005B,
    KP_4 = 0x11005C,  KP_5 = 0x11005D,  KP_6 = 0x11005E, KP_7 = 0x11005F,
    KP_8 = 0x110060,  KP_9 = 0x110061,
    KP_DIV   = 0x110054,  KP_MUL = 0x110055,
    KP_MINUS = 0x110056,  KP_PLUS= 0x110057,
    KP_ENTER = 0x110058,  KP_DOT = 0x110063,
  };
}
