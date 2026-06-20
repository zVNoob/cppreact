/** @file
 *  @brief owner<T> — a unique_ptr wrapper with implicit T* conversion.
 *
 *  Combines the zero-overhead ownership of unique_ptr with the ergonomics
 *  of a raw pointer: owner<T> converts to T* automatically, so callers
 *  can pass it directly to functions that expect raw pointers without
 *  writing .get() everywhere.  The underlying unique_ptr ensures proper
 *  destruction when the owner goes out of scope. */

#pragma once

#include <memory>

namespace cppreact {

/** @brief Owning smart pointer with implicit raw-pointer conversion.
 *
 *  Wraps std::unique_ptr<T> and adds operator T*() so that owner<T>
 *  can be passed directly to functions taking a T* — no .get() needed.
 *  This is ideal for factory functions that return a resource the caller
 *  owns, while letting APIs that borrow the resource use raw pointers. */
template<typename T>
class owner {
  std::unique_ptr<T> _ptr;
public:
  owner() = default;
  owner(std::unique_ptr<T> p) : _ptr(std::move(p)) {}
  owner(owner&&) = default;
  owner& operator=(owner&&) = default;
  owner(const owner&) = delete;
  owner& operator=(const owner&) = delete;

  operator T*() const { return _ptr.get(); }
  T* operator->() const { return _ptr.get(); }
  T& operator*() const { return *_ptr; }
  explicit operator bool() const { return !!_ptr; }
  T* get() const { return _ptr.get(); }
  std::unique_ptr<T>& inner() { return _ptr; }
};

}

