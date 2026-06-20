#include "internal/arena.hpp"
#include "internal/state.hpp"
#include "internal/registry.hpp"
#include "internal/identifiable.hpp"
#include "internal/keycode.hpp"
#include "internal/key.hpp"
#include "internal/property.hpp"
#include "internal/component.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

using namespace cppreact;
using namespace cppreact::_storage;
using namespace cppreact::_detail;

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) do { \
  tests_run++; \
  if (!(cond)) { \
    fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); \
    tests_failed++; \
  } \
} while(0)

static void test_arena_basic() {
  arena a(64);
  int* p = a.allocate<int>(42);
  ASSERT(*p == 42, "arena allocate int");
  ASSERT(p != nullptr, "arena non-null");
}

static void test_arena_multiple() {
  arena a(128);
  int* p1 = a.allocate<int>(1);
  int* p2 = a.allocate<int>(2);
  int* p3 = a.allocate<int>(3);
  ASSERT(*p1 == 1 && *p2 == 2 && *p3 == 3, "arena multiple allocs");
  ASSERT(p2 > p1, "arena sequential addresses");
  ASSERT(p3 > p2, "arena sequential addresses");
}

static void test_arena_reset() {
  arena a(64);
  int* before = a.allocate<int>(99);
  *before = 42;
  a.reset();
  int* after = a.allocate<int>(100);
  ASSERT(*after == 100, "arena reset reuses block");
  ASSERT(before == after, "arena reset reuses same address");
}

static void test_arena_block_growth() {
  arena a(32);
  std::vector<int*> ptrs;
  for (int i = 0; i < 1000; i++)
    ptrs.push_back(a.allocate<int>(i));
  for (int i = 0; i < 1000; i++)
    ASSERT(*ptrs[i] == i, "arena growth preserves values");
}

struct Counted {
  static int alive;
  int id;
  Counted(int id) : id(id) { alive++; }
  ~Counted() { alive--; }
};
int Counted::alive = 0;

static void test_arena_destructors() {
  Counted::alive = 0;
  {
    arena a(64);
    a.allocate<Counted>(1);
    a.allocate<Counted>(2);
    ASSERT(Counted::alive == 2, "arena destructor tracking");
    a.reset();
    ASSERT(Counted::alive == 0, "arena reset calls destructors");
  }
}

static void test_state_basic() {
  state s;
  auto p = s.get<int>(42);
  ASSERT(int(p) == 42, "state get<int> default");
  p = 100;
  ASSERT(s.changed(), "state changed flag");
}

static void test_state_cursor_advance() {
  state s;
  auto a = s.get<int>(1);
  auto b = s.get<int>(2);
  auto c = s.get<int>(3);
  ASSERT(int(a) == 1 && int(b) == 2 && int(c) == 3, "state cursor advance");
}

static void test_state_reset() {
  state s;
  auto p = s.get<int>(0);
  p = 42;
  s.reset();
  ASSERT(!s.changed(), "state reset clears changed");
  auto q = s.get<int>(0);
  ASSERT(int(q) == 42, "state reset preserves values");
}

static void test_state_type_mismatch() {
  state s;
  s.get<int>(0);
  s.reset();
  bool threw = false;
  try { s.get<float>(0); } catch (std::runtime_error&) { threw = true; }
  ASSERT(threw, "state type mismatch throws");
}

static void test_property_write_detection() {
  bool dirty = false;
  int val = 0;
  property<int> p(&val, &dirty);
  ASSERT(int(p) == 0, "property read");
  p = 5;
  ASSERT(dirty, "property write sets dirty");
  ASSERT(val == 5, "property write updates value");
}

static void test_property_arrow() {
  bool dirty = false;
  std::pair<int, int> val = {0, 0};
  property<std::pair<int,int>> p(&val, &dirty);
  p->first = 7;
  ASSERT(dirty, "property -> sets dirty");
  ASSERT(val.first == 7, "property -> updates field");
}

static void test_registry_basic() {
  auto& reg = current_registry;
  reg.reset();
  auto& d1 = reg.get(42);
  ASSERT(d1.second.changed(), "registry new entry");
}

static void test_registry_reset() {
  auto& reg = current_registry;
  reg.reset();
  auto& d1 = reg.get(1);
  auto& d1b = reg.get(1);
  ASSERT(&d1 != &d1b, "registry cursor advances");
  ASSERT(d1.second.changed() && d1b.second.changed(), "registry entries start dirty");
}

using _config::element_config;

static void test_identifiable_id() {
  auto make = [](std::source_location loc = std::source_location::current()) {
    return _detail::component(element_config{}, loc);
  };
  auto c1 = make();
  auto c2 = make();
  ASSERT(c1.id() != c2.id(), "identifiable different IDs at different cols");
}

static void test_identifiable_parent() {
  auto child = _detail::component(element_config{}, std::source_location::current());
  uint64_t before = child.id();
  auto parent = _detail::component(element_config{}, std::source_location::current());
  child.set_parent(&parent);
  ASSERT(child.id() != before, "identifiable parent mixing changes ID");
}

static void test_keycode_values() {
  ASSERT(uint32_t(keycode::A) == 'A', "keycode A == ASCII A");
  ASSERT(uint32_t(keycode::SPACE) == ' ', "keycode SPACE == ASCII space");
  ASSERT(uint32_t(keycode::ENTER) == 0x110028, "keycode ENTER == 0x110028");
  ASSERT(uint32_t(keycode::ESCAPE) == 0x110029, "keycode ESCAPE == 0x110029");
  ASSERT(uint32_t(keycode::UP) == 0x110052, "keycode UP == 0x110052");
  ASSERT(uint32_t(keycode::F1) == 0x11003A, "keycode F1 == 0x11003A");
}

int main() {
  fprintf(stderr, "=== test_core: arena, state, property, registry, identifiable, keycode ===\n");

  test_arena_basic();
  test_arena_multiple();
  test_arena_reset();
  test_arena_block_growth();
  test_arena_destructors();

  test_state_basic();
  test_state_cursor_advance();
  test_state_reset();
  test_state_type_mismatch();

  test_property_write_detection();
  test_property_arrow();

  test_registry_basic();
  test_registry_reset();

  test_identifiable_id();
  test_identifiable_parent();

  test_keycode_values();

  fprintf(stderr, "  %d tests, %d failed\n", tests_run, tests_failed);
  return tests_failed > 0 ? 1 : 0;
}
