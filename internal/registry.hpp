/** @file
 *  @brief Registry for storing ID-indexed arena+state data.
 */

#pragma once

#include "arena.hpp"
#include "state.hpp"
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <utility>

namespace cppreact {
  class renderer;
  namespace _storage {

    /** @brief Maps unique IDs to a deque of (arena, state) pairs, keyed by version index.
     *
     *  Used to persist per-element state across frames. Each call to get()
     *  advances an internal cursor, creating new data pairs as needed.
     */
    class registry {
      typedef std::pair<arena, state> data;                   ///< Arena + state pair per element
      typedef std::pair<uint64_t, std::deque<data>> entry;    ///< Version counter + deque of data
      typedef std::unordered_map<uint64_t, entry> storage;    ///< ID -> entry map
      storage _data; ///< Underlying hash map
      public:
      /** @brief Get or create the next data pair for a given key.
       *  @param key The element's unique ID.
       *  @return Reference to the arena+state pair.
       */
      renderer* current_renderer = nullptr;
      data& get(uint64_t key) {
        auto&t = _data[key];
        if (t.second.size() == t.first) t.second.emplace_back();
        return t.second[t.first++];
      }
      /** @brief Reset all version counters (but keep allocated memory). */
      void reset() {
        for (auto& it : _data) {
          it.second.first = 0;
        }
      }
    };
    /// Thread-local global registry instance
    static thread_local registry current_registry;
  }
}
