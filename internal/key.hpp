/** @file
 *  @brief Keyboard shortcut/combination handling with callbacks.
 */

#pragma once

#include "keycode.hpp"
#include <functional>
#include <map>
#include <utility>
#include <vector>
namespace cppreact {
  class renderer;
  namespace _detail {

    /** @brief Tracks a multi-key combination and fires callbacks on press/release.
     *
     *  When all keys in the combination are pressed simultaneously the
     *  callback fires @c true; when any key is released it fires @c false.
     */
    class key {
      friend class cppreact::renderer;
      int _key_pressed = 0;                    ///< Number of keys currently held
      std::map<keycode, bool> _keys;           ///< Per-key pressed state
      std::vector<keycode> _original_combo;    ///< The original key combination
      std::function<void(std::vector<keycode>, bool, bool)> _func; ///< Callback

      /** @brief Handle a key-down or key-up event for this combination. */
      void on_key(keycode k, bool down, bool is_ime) {
        // Special case: listening for every keys
        if (_keys.size() == 0) {
          _func({k}, down, is_ime);
          return;
        }
        if (_key_pressed == down ? _keys.size() : 0)
          return;
        auto it = _keys.find(k);
        if (it != _keys.end()) {
          if (it->second == down)
            return;
          it->second = down;
          _key_pressed += down ? 1 : -1;
        }
        if (_key_pressed == _keys.size()) {
          _func(_original_combo, true, is_ime);
        }
        if (_key_pressed == 0) {
          _func(_original_combo, false, is_ime);
        }
      }
      public:
      /** @brief Register a key combination and its callback.
       *  @param keys The sequence of keycodes in the combination.
       *  @param func Callback invoked with the combo and a bool (true = pressed, false = released).
       */
      key(std::vector<keycode> keys, std::function<void(std::vector<keycode>, bool, bool)> func) {
        _original_combo = keys;
        _func = func;
        for (auto& k : keys)
          _keys[k] = false;
      }
    };
  } // namespace _detail
} // namespace cppreact
