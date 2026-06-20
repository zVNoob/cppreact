/** @file
 *  @brief IME callback and render command for input method editor support.
 */

#pragma once

#include "element.hpp"
#include <functional>
#include <string>

namespace cppreact {
  namespace _detail {

    /** @brief Callbacks for IME session start and end events. */
    struct ime_callback {
      std::function<void()> start = [](){}; ///< Called when IME composition begins
      std::function<void()> end = [](){};   ///< Called when IME composition ends
    };

    /** @brief Render command that carries IME composition state. */
    class ime_render_command : public render_command {
    public:
      ime_callback& callback; ///< IME session callbacks
      /** @brief Called when the composition text or cursor position changes. */
      std::function<void(std::string text, int cursor)> editing = [](std::string  ,int){};
      ime_render_command(ime_callback& callback, std::function<void(std::string text, int cursor)> editing,bounding_box box) :
        render_command(box), editing(editing), callback(callback) {}
    };
  }
}
