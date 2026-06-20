/** @file
 *  @brief State management with a hooks-like API using property<T>.
 */

#pragma once

#include "property.hpp"
#include <any>
#include <memory>
#include <vector>
#include <stdexcept>

namespace cppreact {

  /** @brief State container providing a hooks-style property API.
   *
   *  Stores typed values in a cursor-advancing vector, returning
   *  property<T> wrappers that detect mutations.
   */
  class state {
    std::vector<std::unique_ptr<std::any>> _storage; ///< Holds typed values as std::any
    size_t _cursor = 0;  ///< Current access cursor for hook-like API
    bool _changed;       ///< Dirty flag set when any property is written
    public:
    state(): _changed(true) {}
    state(const state& other) = delete;
    state& operator=(const state& other) = delete;

    /** @brief Get or create a property for the current cursor position.
     *
     *  @tparam T The type of the state value.
     *  @param value Default value used if storage is uninitialized.
     *  @return A property<T> wrapping the internal storage.
     *  @throws std::runtime_error if the stored type differs from T.
     */
    template<typename T>
    _storage::property<T> get(const T& value = T()) {
      if (_cursor >= _storage.size())
        _storage.push_back(std::make_unique<std::any>(value));
      std::any& current = *_storage[_cursor];
      T* typed_ptr = std::any_cast<T>(&current);
      if (!typed_ptr)
        throw std::runtime_error("Hook type mismatch! Call order changed or type changed.");
      _cursor++;
      return _storage::property<T>(typed_ptr, &_changed);
    }

    /** @brief Check whether any property was mutated since last reset. */
    bool changed() {return _changed;}
    /** @brief Clear the dirty flag and reset the cursor. */
    void reset() {_changed = false;_cursor = 0;}
  };
}
