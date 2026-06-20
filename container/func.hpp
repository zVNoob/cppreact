/** @file
 *  @brief Functional component wrapper. */

#pragma once

#include "../internal/arena.hpp"
#include "../internal/identifiable.hpp"
#include "../internal/element.hpp"
#include "../internal/registry.hpp"
#include "../internal/state.hpp"
#include <functional>
#include <source_location>

namespace cppreact {
  namespace _detail { 
    class func;
    /** @brief Signature for a lambda that produces an element from a state reference. */
    typedef std::function<element*(state&)> element_lambda;
    /** @brief Signature for a lambda that produces an element from a state reference and the owning func pointer. */
    typedef std::function<element*(state&, func*)> element_lambda_full;
    /** @brief Functional component that wraps an element-rendering lambda.
     *
     *  The lambda is called on demand to produce a child element tree.
     *  The result is cached and only re-evaluated when the associated
     *  state changes. A dedicated arena is used for the lambda's
     *  allocations. */
    class func : public identifiable {
    private:
      element_lambda_full _func; ///< The wrapped lambda function
      element* current_element = nullptr; ///< Cached element from the last invocation
    public:
      /** @brief Construct a func with a lambda and source location.
       *  @param func The element-rendering lambda
       *  @param loc Source location for diagnostics */
      func(element_lambda_full func, std::source_location loc) :
        identifiable(loc), _func(func) {
        }
      /** @brief Get (or compute) the element tree.
       *
       *  If the state has changed or no cached element exists, the lambda
       *  is re-evaluated. A temporary arena is swapped in for the
       *  lambda's allocations and restored on completion. */
      element & get() override {
        auto& data = _storage::current_registry.get(id());
        if (data.second.changed() || (current_element == nullptr)) {
          // Reset arena and state, then invoke the lambda under a private arena
          data.second.reset();
          data.first.reset();
          _storage::arena* old_arena = _storage::current_arena;
          _storage::current_arena = &data.first;
          current_element = _func(data.second, this);
          _storage::current_arena = old_arena;
        }
        return *current_element;
      }
    };
  }
  /** @brief Allocate a func component with a full lambda that receives both state and self pointer.
   *  @param func The element-rendering lambda accepting (state&, func*)
   *  @param loc Source location for diagnostics
   *  @return Pointer to the allocated func */
  inline _detail::func* func(_detail::element_lambda_full func, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::func>(func, loc);
  };
  /** @brief Allocate a func component with a simple lambda that receives only state.
   *  @param func The element-rendering lambda accepting (state&)
   *  @param loc Source location for diagnostics
   *  @return Pointer to the allocated func */
  inline _detail::func* func(_detail::element_lambda func, std::source_location loc = std::source_location::current()) {
    _detail::element_lambda_full f = [func = std::move(func)](state& s, _detail::func*) {
      return func(s);
    };
    return _storage::allocate<_detail::func>(f, loc);
  };
}
