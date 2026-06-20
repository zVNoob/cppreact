#pragma once

/** @file renderer.hpp
 *  @brief Abstract base class for renderers with event handling and run loop */

#include "../container/stack.hpp"
#include "../internal/arena.hpp"
#include "../internal/registry.hpp"
#include "../widgets/rect.hpp"
#include "../widgets/image.hpp"
#include "../widgets/text.hpp"
#include "../input/button_simple.hpp"
#include "../input/scrolling.hpp"
#include "../internal/font.hpp"
#include "../internal/key.hpp"
#include "../internal/ime.hpp"
#include "../internal/owner.hpp"
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <source_location>
#include <utility>
#include <vector>

namespace cppreact {
  /** @brief Abstract base class for renderers
   *
   * Manages texture and font caches, keyboard input, IME text, cursor state,
   * and the per-frame layout/render pipeline.  Platform-specific subclasses
   * implement texture creation, font loading, and the main event loop. */
  class renderer {
    private:
      /** @brief Current mouse-cursor state (pressed buttons and position) */
      struct {
        uint16_t mask = 0; ///< Bitmask of pressed buttons (LCLICK | RCLICK | MCLICK)
        uint16_t x = 0;    ///< Cursor X position
        uint16_t y = 0;    ///< Cursor Y position
      } cursor;
      int32_t _scroll_x = 0; ///< Accumulated horizontal scroll delta
      int32_t _scroll_y = 0; ///< Accumulated vertical scroll delta
      _storage::arena _cmd_arena{16*1024}; ///< Arena allocator for per-frame render commands
      std::map<std::filesystem::path, std::shared_ptr<_detail::single_font>> _fonts_cache; ///< Cache of loaded single-font faces by file path
      std::source_location _loc; ///< Source location captured at construction for debugging
      std::set<std::shared_ptr<_detail::key>> _keys; ///< Registered keyboard shortcut handlers
      uint64_t _focus = 0; ///< ID of the currently focused widget (0 = none)
      std::pair<std::string, uint64_t> _ime_text = {"", 0}; ///< Current IME composition text and cursor position
    public:
      /** @brief Construct a renderer, capturing the caller's source location */
      renderer(std::source_location loc = std::source_location::current()) {
        _loc = loc;
      }

      /** @brief Load a texture from disk
        * @param path File path to the texture image
        * @return owner<texture> wrapping the platform-specific texture */
      owner<texture> load_texture(const std::filesystem::path& path) {
          return owner<texture>(on_load_texture(path));
      }

      /** @brief Load a composite font from multiple single-font faces
        *
        * Each entry in @p path corresponds to a font variant (regular, bold,
        * italic, etc.).  The faces are combined into a single _detail::font
        * that selects between them based on the @p bold and @p italic flags.
        * @param path List of font-file paths, one per variant
        * @param size Font size in pixels
        * @param bold Prefer the bold variant
        * @param italic Prefer the italic variant
        * @return owner<font> wrapping the composite font */
      owner<_detail::font> load_font(const std::vector<std::filesystem::path>& path, int size, bool bold = false, bool italic = false) {
        std::vector<std::shared_ptr<_detail::single_font>> fonts;
        for (auto& p : path)
          fonts.push_back(load_single_font(p, size));
        return owner<_detail::font>(std::make_unique<_detail::font>(std::move(fonts), bold, italic));
      }
      /** @brief Register a keyboard-shortcut handler
       * @param keys Key combination that triggers the callback
       * @param func Callback invoked with the key combination and press/release state
       * @return Shared pointer that keeps the handler alive until unregistered */
      std::shared_ptr<_detail::key> register_key(std::vector<keycode> keys, std::function<void(std::vector<keycode>, bool)> func) {
        auto k = std::make_shared<_detail::key>(keys, func);
        _keys.insert(k);
        return k;
      }
      uint64_t get_focus_id() const {return _focus;}
      /** @brief Set the widget that currently has keyboard focus
       * @param id Widget to focus, or nullptr to clear focus */
      void set_focus(_detail::identifiable* id) {
        if (id) _focus = id->id();
        else _focus = 0;
      }
      /** @brief Unregister a previously registered key handler
       * @param key Handler to remove (must have been returned by register_key) */
      void unregister_key(std::shared_ptr<_detail::key> key) {
        _keys.erase(key);
      }
      /** @brief Create a texture from raw RGBA pixel data (platform-specific)
        * @param width Texture width in pixels
        * @param height Texture height in pixels
        * @param pixels Raw interleaved RGBA pixel data
        * @return owner<texture> wrapping the platform-specific texture */
      virtual owner<texture> get_texture(uint16_t width, uint16_t height, std::vector<uint8_t> pixels) = 0;
      /** @brief Enter the main event loop (platform-specific)
       * @param root Root widget of the UI tree */
      virtual void run(_detail::identifiable* root) = 0;
    protected:
      /** @brief Update the tracked cursor state (called by subclasses)
       * @param mask Bitmask of pressed mouse buttons
       * @param x Cursor X position
       * @param y Cursor Y position */
      void set_cursor(uint16_t mask, uint16_t x, uint16_t y) {
        cursor.mask = mask;
        cursor.x = x;
        cursor.y = y;
      }
      /** @brief Accumulate scroll-wheel deltas (called by subclasses)
       * @param x Horizontal scroll delta
       * @param y Vertical scroll delta */
      void set_scroll(int32_t x, int32_t y) {
        _scroll_x = x;
        _scroll_y = y;
      }
      /** @brief Forward a key event to all registered handlers
       * @param k Keycode that was pressed or released
       * @param down true if the key was pressed, false if released */
      void set_key(keycode k, bool down) {
        for (auto& key : _keys)
          key.get()->on_key(k, down);
      }
      /** @brief Update the current IME composition state
       * @param text The IME composition string
       * @param cursor Cursor position within the composition */
      void set_editing(std::string text, int cursor) {
        _ime_text.first = text;
        _ime_text.second = cursor;
      }
      /** @brief Load a single font face from disk, caching it by path
       * @param path Font file path
       * @param size Font size in pixels
       * @return Shared pointer to the loaded single-font face */
      std::shared_ptr<_detail::single_font> load_single_font(const std::filesystem::path& path, int size) {
        if (_fonts_cache.find(path) == _fonts_cache.end())
          _fonts_cache[path] = on_load_single_font(path, size);
        return _fonts_cache[path];
      }
      void clear_font_cache() {_fonts_cache.clear();}
    protected:
      /** @brief Execute one frame of the UI loop
       *
       * Resets the per-frame registry and arena, then runs the full widget
       * layout pipeline (fit, grow, position).  Renders the widget tree into
       * a list of render commands, dispatches drawing commands immediately,
       * and processes interactive (button/scroll) commands in reverse render
       * order so that the topmost widget receives input first.
       * @param root Root widget to update and render
       * @param cfg Container configuration controlling sizing behaviour */
      void run_once(_detail::identifiable* root, _config::container_config cfg) {
        _storage::current_registry.reset();
        _cmd_arena.reset();
        _detail::stack actual_root(cfg,{root}, _loc);
        actual_root.on_update();
        actual_root.on_fit_x();
        actual_root.on_child_grow_x();
        actual_root.on_fit_y();
        actual_root.on_child_grow_y();
        actual_root.on_child_pos_x();
        actual_root.on_child_pos_y();
        std::vector<_detail::button_render_command*> btn_cmds;
        std::vector<_detail::scroll_render_command*> scr_cmds;
        auto prev_arena = _storage::current_arena;
        _storage::current_arena = &_cmd_arena;
        auto cmds = actual_root.on_render(actual_root.box());
        _storage::current_arena = prev_arena;
        for (auto e : cmds) {
          if (dynamic_cast<_detail::rect_render_command*>(e)) {
            on_rect_cmd(static_cast<_detail::rect_render_command&>(*e));
          } else if (dynamic_cast<_detail::image_render_command*>(e)) {
            auto* img = (_detail::image_render_command*)e;
            on_image_cmd(static_cast<_detail::image_render_command&>(*e));
          } else if (dynamic_cast<_detail::text_render_command*>(e)) {
            on_text_cmd(static_cast<_detail::text_render_command&>(*e));
          } else if (dynamic_cast<_detail::ime_render_command*>(e)) {
            on_ime_cmd(static_cast<_detail::ime_render_command&>(*e));
            if (_ime_text.first.size() > 0)
              static_cast<_detail::ime_render_command&>(*e).editing(_ime_text.first, _ime_text.second);
          } else
          if (dynamic_cast<_detail::button_render_command*>(e))
            btn_cmds.push_back(static_cast<_detail::button_render_command*>(e));
          if (dynamic_cast<_detail::scroll_render_command*>(e))
            scr_cmds.push_back(static_cast<_detail::scroll_render_command*>(e));
        }
        // Process interactive commands in reverse render order so the topmost widget has priority
        for (int i = btn_cmds.size() - 1; i >= 0; i--)
          if (on_button_cmd(*btn_cmds[i])) break;
        for (int i = scr_cmds.size() - 1; i >= 0; i--)
          if (on_scroll_cmd(*scr_cmds[i])) break;
        _scroll_x = 0;
        _scroll_y = 0;
      }
      /** @brief Load a texture from disk (platform-specific)
       * @param path File path as a string
       * @return Shared pointer to the loaded texture, or nullptr on failure */
      virtual std::unique_ptr<texture> on_load_texture(const std::string& path) = 0;
      /** @brief Load a single font face from disk (platform-specific)
       * @param path Font-file path as a string
       * @param size Font size in pixels
       * @return Shared pointer to the loaded single-font face */
      virtual std::shared_ptr<_detail::single_font> on_load_single_font(const std::string& path, int size) = 0;
      /** @brief Handle a rectangle render command
       * @param cfg Rectangle render command to execute */
      virtual void on_rect_cmd(_detail::rect_render_command& cfg) = 0;
      /** @brief Handle an image render command
       * @param cfg Image render command to execute */
      virtual void on_image_cmd(_detail::image_render_command& cfg) = 0;
      /** @brief Handle a text render command 
       * @param cfg Text render command to execute */
      virtual void on_text_cmd(_detail::text_render_command& cfg) = 0;
      /** @brief Handle an IME render command (default: no-op)
       * @param cfg IME render command */
      virtual void on_ime_cmd(_detail::ime_render_command& cfg) {}
      /** @brief Handle a button render command (hit-test + dispatch)
       *
       * Checks whether the cursor lies inside the button's bounding box.
       * If so, translates the cursor button mask into event bits and invokes
       * the button's callback with local coordinates.
       * @param cfg Button render command
       * @return true if the cursor was within the button bounds */
      virtual bool on_button_cmd(_detail::button_render_command& cfg) {
        if (int32_t(cursor.x) < int32_t(cfg.box.x) ||
            int32_t(cursor.x) >= int32_t(cfg.box.x) + int32_t(cfg.box.width) ||
            int32_t(cursor.y) < int32_t(cfg.box.y) ||
            int32_t(cursor.y) >= int32_t(cfg.box.y) + int32_t(cfg.box.height))
          return false;
        uint16_t bits = HOVER;
        if (cursor.mask & LCLICK) bits |= LCLICK;
        if (cursor.mask & RCLICK) bits |= RCLICK;
        if (cursor.mask & MCLICK) bits |= MCLICK;
        if (bits & cfg.event_mask)
          cfg.func(bits, std::pair<uint16_t, uint16_t>(
            uint16_t(std::max<int32_t>(0, int32_t(cursor.x) - int32_t(cfg.box.x))),
            uint16_t(std::max<int32_t>(0, int32_t(cursor.y) - int32_t(cfg.box.y)))
          ));
        return true;
      }
      /** @brief Handle a scroll render command (hit-test + dispatch)
       *
       * Checks whether the cursor lies inside the scrollable area.
       * If so and accumulated scroll deltas are non-zero, forwards them
       * to the scrollable widget's callback.
       * @param cfg Scroll render command
       * @return true if the cursor was within the scrollable area */
      virtual bool on_scroll_cmd(_detail::scroll_render_command& cfg) {
        if (int32_t(cursor.x) < int32_t(cfg.box.x) ||
            int32_t(cursor.x) >= int32_t(cfg.box.x) + int32_t(cfg.box.width) ||
            int32_t(cursor.y) < int32_t(cfg.box.y) ||
            int32_t(cursor.y) >= int32_t(cfg.box.y) + int32_t(cfg.box.height))
          return false;
        if (_scroll_x != 0 || _scroll_y != 0)
          cfg.func(_scroll_x, _scroll_y);
        return true;
      }
  };
}
