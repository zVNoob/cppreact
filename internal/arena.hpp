/** @file
 *  @brief Custom arena allocator for efficient frame-based memory management.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace cppreact {
  namespace _storage {

  /** @brief A fast arena allocator that groups allocations into blocks.
   *
   *  Allocated objects can be non-trivially destructible; destructors
   *  are tracked and called on reset. Memory blocks themselves are
   *  retained across resets to minimise OS-level allocation calls.
   */
  class arena {
    /** @brief A contiguous chunk of memory within the arena. */
    struct block {
      std::unique_ptr<char[]> data; ///< Raw memory
      uint32_t size;   ///< Total capacity of this block
      uint32_t offset; ///< Current allocation offset within the block

      block(uint32_t s) : data(std::make_unique<char[]>(s)), size(s), offset(0) {}
    };

    /// All blocks owned by this arena
    std::vector<block> _blocks;

    /** @brief Entry in the cleanup list for non-trivial destructors. */
    struct Cleanup {
      void (*destructor)(void*); ///< Function pointer to the destructor
      void* ptr;                ///< Pointer to the allocated object
    };
    std::vector<Cleanup> _cleanup_list; ///< Destructors to call on reset

    arena(const arena&) = delete;
    arena& operator=(const arena&) = delete;
    arena(arena&&) noexcept = default;
    arena& operator=(arena&&) noexcept = default;

  public:
    /** @brief Create an arena with an initial block size.
     *  @param initial_size Size in bytes of the first block (default 4 KiB).
     */
    arena(uint32_t initial_size = 4*1024) {
      _blocks.emplace_back(initial_size);
    }

    /** @brief Destroy the arena, calling reset to run all pending destructors. */
    ~arena() {reset();}

    /** @brief Allocate and construct an object of type T.
     *  @tparam T The type to allocate.
     *  @tparam Args Constructor argument types.
     *  @param args Forwarded constructor arguments.
     *  @return Pointer to the newly constructed object.
     */
    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
      size_t alignment = alignof(T);
      size_t size = sizeof(T);

      // Try to find space in the current (last) block
      block& current = _blocks.back();
      uintptr_t curr_addr = reinterpret_cast<uintptr_t>(current.data.get() + current.offset);
      uintptr_t padding = (alignment - (curr_addr % alignment)) % alignment;

      if (current.offset + padding + size <= current.size)
        return perform_allocation<T>(current, padding, args...);

      // Current block is full — allocate a new one with exponential growth
      uint32_t next_size = current.size * 2;
      _blocks.emplace_back(next_size);

      // Recalculate alignment for the fresh block
      block& new_block = _blocks.back();
      uintptr_t new_addr = reinterpret_cast<uintptr_t>(new_block.data.get() + new_block.offset);
      uintptr_t new_padding = (alignment - (new_addr % alignment)) % alignment;

      return perform_allocation<T>(new_block, new_padding, args...);
    }

    /** @brief Reset the arena.
     *
     *  Calls destructors for all non-trivially-destructible objects in
     *  reverse allocation order, then resets block offsets. Memory
     *  blocks are kept and reused to avoid repeated OS calls.
     */
    void reset() {
      // Call destructors in reverse order
      for (auto it = _cleanup_list.rbegin(); it != _cleanup_list.rend(); ++it) {
        it->destructor(it->ptr);
      }
      _cleanup_list.clear();

      // Keep the blocks, just reset the offsets
      for (auto& block : _blocks) {
        block.offset = 0;
      }
    }

  private:
    /** @brief Low-level allocation: place an object in the given block at the padded offset. */
    template<typename T, typename... Args>
    T* perform_allocation(block& block, uintptr_t padding, Args&&... args) {
      block.offset += static_cast<uint32_t>(padding);
      T* ptr = reinterpret_cast<T*>(block.data.get() + block.offset);
      block.offset += sizeof(T);

      T* object = new (ptr) T(std::forward<Args>(args)...);

      if constexpr (!std::is_trivially_destructible_v<T>) {
        _cleanup_list.push_back({
          [](void* p) { static_cast<T*>(p)->~T(); },
          ptr
        });
      }
      return object;
    }
  };

  /// Thread-local pointer to the current arena context
  static thread_local arena* current_arena = nullptr;

  /** @brief Allocate through the current thread-local arena, or fall back to heap. */
  template<typename T, typename... Args>
  T* allocate(Args&&... args) {
    if (current_arena)
      return current_arena->allocate<T>(std::forward<Args>(args)...);
    return new T(std::forward<Args>(args)...);
  }
  } // namespace _storage
} // namespace cppreact
