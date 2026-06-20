/** @file
 *  @brief property<T> wrapper that tracks changes via a dirty flag.
 */

#pragma once

namespace cppreact {
  namespace _storage {

    /** @brief Wrapper around a value that sets a dirty flag on every write.
     *
     *  @tparam T The underlying value type.
     *
     *  Reads are transparent via implicit conversion; writes set a
     *  shared boolean flag so consumers can detect changes.
     */
    template <typename T>
    class property {
      T* value;    ///< Pointer to the underlying value storage
      bool* changed; ///< Shared dirty flag, set to true on mutation
      public:
      property(T* value, bool* changed) : value(value), changed(changed) {}
      /** @brief Implicit read conversion. */
      operator const T&() const { return *value; }
      /** @brief Const arrow operator (read-only access to members). */
      const T* operator ->() const { return value; }
      /** @brief Mutable arrow operator; marks changed on access. */
      T* operator ->(){ *changed = true; return value; }
      /** @brief Assignment; copies the value and marks changed. */
      T& operator=(const T& rhs) const {
        *value = rhs;
        *changed = true;
        return *value;
      }
    };
  }
}
