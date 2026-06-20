#include "container/vbox.hpp"
#include "container/hbox.hpp"
#include "container/stack.hpp"
#include "internal/component.hpp"
#include "internal/element.hpp"
#include "internal/registry.hpp"
#include <cstdio>
#include <cstdlib>

using namespace cppreact;
using namespace cppreact::_config;
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

// Helper: run the full layout pipeline on any element
static void run_layout(element* e) {
  e->on_update();
  e->on_fit_x();
  e->on_child_grow_x();
  e->on_fit_y();
  e->on_child_grow_y();
  e->on_child_pos_x();
  e->on_child_pos_y();
}

static void test_vbox_fit_sizes_to_tallest_child() {
  // vbox with two FIXED children (50 wide x 30 tall, 80 wide x 20 tall)
  // vbox width should = 80 + padding, vbox height = 30 + 20 + spacing + padding
  auto c1 = component(element_config{.sizing = {FIXED(50), FIXED(30)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(80), FIXED(20)}}, {});
  auto box = cppreact::vbox({.sizing = {FIT(), FIT()}, .padding = PAD(0), .spacing = 0}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(box->box().width == 80, "vbox fit width = max child width");
  ASSERT(box->box().height == 50, "vbox fit height = sum of child heights");
}

static void test_vbox_padding() {
  auto c1 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto box = cppreact::vbox({.sizing = {FIT(), FIT()}, .padding = PAD(10), .spacing = 0}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(box->box().width == 30, "vbox fit width with padding");
  ASSERT(box->box().height == 60, "vbox fit height with padding");
}

static void test_vbox_spacing() {
  auto c1 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto box = cppreact::vbox({.sizing = {FIT(), FIT()}, .padding = PAD(0), .spacing = 8}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(box->box().height == 48, "vbox fit height with spacing");
}

static void test_vbox_grow_distribution() {
  // Two children with GROW weight 1.0 each in a 200px tall vbox
  auto c1 = component(element_config{.sizing = {FIXED(50), GROW(1.0f)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(50), GROW(1.0f)}}, {});
  auto box = cppreact::vbox({.sizing = {FIXED(200), FIXED(200)}, .padding = PAD(0), .spacing = 0}, {&c1, &c2}, {});
  run_layout(box);
  // Each should get half: 100px
  ASSERT(box->box().width == 200, "vbox grow width");
  ASSERT(box->box().height == 200, "vbox grow height");
  ASSERT(c1.box().height == 100, "vbox grow child1 = 100");
  ASSERT(c2.box().height == 100, "vbox grow child2 = 100");
}

static void test_vbox_grow_unequal_weights() {
  auto c1 = component(element_config{.sizing = {FIXED(50), GROW(1.0f)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(50), GROW(3.0f)}}, {});
  auto box = cppreact::vbox({.sizing = {FIXED(200), FIXED(200)}, .padding = PAD(0), .spacing = 0}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(c1.box().height == 50, "vbox unequal weight child1 = 50");
  ASSERT(c2.box().height == 150, "vbox unequal weight child2 = 150");
}

static void test_vbox_child_positions() {
  auto c1 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto box = cppreact::vbox({.sizing = {FIT(), FIT()}, .padding = PAD(0), .spacing = 0}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(box->box().x == 0, "vbox origin x = 0");
  ASSERT(box->box().y == 0, "vbox origin y = 0");
  // First child at y=0, second at y=20
  ASSERT(c1.box().y == 0, "vbox child1 y = 0");
  ASSERT(c2.box().y == 20, "vbox child2 y = 20");
}

static void test_hbox_fit_sizes_to_widest_child() {
  auto c1 = component(element_config{.sizing = {FIXED(30), FIXED(50)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(20), FIXED(80)}}, {});
  auto box = cppreact::hbox({.sizing = {FIT(), FIT()}, .padding = PAD(0), .spacing = 0}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(box->box().width == 50, "hbox fit width = sum of child widths");
  ASSERT(box->box().height == 80, "hbox fit height = max child height");
}

static void test_hbox_grow_distribution() {
  auto c1 = component(element_config{.sizing = {GROW(1.0f), FIXED(50)}}, {});
  auto c2 = component(element_config{.sizing = {GROW(1.0f), FIXED(50)}}, {});
  auto box = cppreact::hbox({.sizing = {FIXED(200), FIXED(50)}, .padding = PAD(0), .spacing = 0}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(c1.box().width == 100, "hbox grow child1 = 100");
  ASSERT(c2.box().width == 100, "hbox grow child2 = 100");
}

static void test_hbox_child_positions() {
  auto c1 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(40), FIXED(20)}}, {});
  auto box = cppreact::hbox({.sizing = {FIT(), FIT()}, .padding = PAD(0), .spacing = 0}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(c1.box().x == 0, "hbox child1 x = 0");
  ASSERT(c2.box().x == 30, "hbox child2 x = 30");
}

static void test_hbox_spacing() {
  auto c1 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto box = cppreact::hbox({.sizing = {FIT(), FIT()}, .padding = PAD(0), .spacing = 10}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(box->box().width == 70, "hbox fit width with spacing");
  ASSERT(c2.box().x == 40, "hbox child2 x with spacing = 30 + 10");
}

static void test_stack_sizes_to_largest_child() {
  auto c1 = component(element_config{.sizing = {FIXED(50), FIXED(30)}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(80), FIXED(20)}}, {});
  auto box = cppreact::stack({.sizing = {FIT(), FIT()}, .padding = PAD(0), .spacing = 0}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(box->box().width == 80, "stack width = max child width");
  ASSERT(box->box().height == 30, "stack height = max child height");
}

static void test_stack_alignment() {
  auto c1 = component(element_config{.sizing = {FIXED(30), FIXED(20)}, .alignment = {ALIGN_BEGIN, ALIGN_BEGIN}}, {});
  auto c2 = component(element_config{.sizing = {FIXED(10), FIXED(10)}, .alignment = {ALIGN_END, ALIGN_END}}, {});
  auto box = cppreact::stack({.sizing = {FIXED(100), FIXED(100)}, .padding = PAD(0)}, {&c1, &c2}, {});
  run_layout(box);
  ASSERT(c1.box().x == 0, "stack ALIGN_BEGIN x = 0");
  ASSERT(c1.box().y == 0, "stack ALIGN_BEGIN y = 0");
  ASSERT(c2.box().x == 90, "stack ALIGN_END x = 100 - 10");
  ASSERT(c2.box().y == 90, "stack ALIGN_END y = 100 - 10");
}

static void test_mixed_sizing_fixed_and_grow() {
  auto fixed = component(element_config{.sizing = {FIXED(40), FIXED(30)}}, {});
  auto grow = component(element_config{.sizing = {GROW(1.0f), FIXED(30)}}, {});
  auto box = cppreact::hbox({.sizing = {FIXED(200), FIXED(30)}, .padding = PAD(0), .spacing = 0}, {&fixed, &grow}, {});
  run_layout(box);
  ASSERT(fixed.box().width == 40, "mixed fixed child keeps 40");
  ASSERT(grow.box().width == 160, "mixed grow child gets 200-40 = 160");
}

static void test_nested_containers() {
  // outer vbox -> [inner hbox, inner hbox]
  auto i1c1 = component(element_config{.sizing = {FIXED(20), FIXED(20)}}, {});
  auto i1c2 = component(element_config{.sizing = {FIXED(30), FIXED(20)}}, {});
  auto inner1 = cppreact::hbox({.sizing = {FIT(), FIT()}, .padding = PAD(0)}, {&i1c1, &i1c2}, {});

  auto i2c1 = component(element_config{.sizing = {FIXED(40), FIXED(20)}}, {});
  auto i2c2 = component(element_config{.sizing = {FIXED(50), FIXED(20)}}, {});
  auto inner2 = cppreact::hbox({.sizing = {FIT(), FIT()}, .padding = PAD(0)}, {&i2c1, &i2c2}, {});

  auto outer = cppreact::vbox({.sizing = {FIT(), FIT()}, .padding = PAD(0), .spacing = 4}, {inner1, inner2}, {});
  run_layout(outer);

  ASSERT(inner1->box().width == 50, "inner hbox1 width = 20+30");
  ASSERT(inner2->box().width == 90, "inner hbox2 width = 40+50");
  ASSERT(outer->box().width == 90, "outer vbox width = max(50,90)");
  ASSERT(outer->box().height == 44, "outer vbox height = 20+20+4");
}

static void test_bounding_box_intersect() {
  bounding_box a{10, 10, 0, 0, 100, 100};
  bounding_box b{50, 50, 0, 0, 100, 100};
  auto r = element::intersect(a, b);
  ASSERT(r.x == 50, "intersect x");
  ASSERT(r.y == 50, "intersect y");
  ASSERT(r.width == 60, "intersect width");
  ASSERT(r.height == 60, "intersect height");
}

static void test_bounding_box_clip() {
  bounding_box a{10, 10, 5, 5, 100, 100};
  bounding_box b{50, 50, 0, 0, 100, 100};
  auto r = element::clip(a, b);
  ASSERT(r.x == 50, "clip x");
  ASSERT(r.y == 50, "clip y");
  ASSERT(r.offset_x == 0, "clip offset_x reset to b origin");
  ASSERT(r.offset_y == 0, "clip offset_y reset to b origin");
}

int main() {
  fprintf(stderr, "=== test_layout: vbox, hbox, stack, bounding_box ===\n");

  test_vbox_fit_sizes_to_tallest_child();
  test_vbox_padding();
  test_vbox_spacing();
  test_vbox_grow_distribution();
  test_vbox_grow_unequal_weights();
  test_vbox_child_positions();

  test_hbox_fit_sizes_to_widest_child();
  test_hbox_grow_distribution();
  test_hbox_child_positions();
  test_hbox_spacing();

  test_stack_sizes_to_largest_child();
  test_stack_alignment();

  test_mixed_sizing_fixed_and_grow();
  test_nested_containers();

  test_bounding_box_intersect();
  test_bounding_box_clip();

  fprintf(stderr, "  %d tests, %d failed\n", tests_run, tests_failed);
  return tests_failed > 0 ? 1 : 0;
}
