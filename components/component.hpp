#ifndef _CPPREACT_COMPONENT_HPP
#define _CPPREACT_COMPONENT_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <list>

namespace cppreact {
enum _LayoutSizingMode { _FIT,
						 _GROW,
						 _FIXED };
struct _layout_sizing_axis {
	_LayoutSizingMode mode = _FIT;
	uint16_t min = 0;
	uint16_t max = UINT16_MAX;
	// GROW only field
	// This should be in (0,1] range
	float_t percent = 1;
};
struct _layout_sizing {
	_layout_sizing_axis x;
	_layout_sizing_axis y;
};
inline _layout_sizing_axis SIZING_FIT() { return {_FIT, 0, UINT16_MAX}; };
inline _layout_sizing_axis SIZING_FIT(uint16_t min, uint16_t max) { return {_FIT, min, max}; };
inline _layout_sizing_axis SIZING_FIXED(uint16_t x) { return {_FIXED, x, x}; };
inline _layout_sizing_axis SIZING_GROW(float_t percent = 1) { return {_GROW, 0, UINT16_MAX, percent}; };
inline _layout_sizing_axis SIZING_GROW(uint16_t min, uint16_t max, float_t percent = 1) { return {_GROW, min, max, percent}; };

enum _LayoutDirection { LEFT_TO_RIGHT,
						TOP_TO_BOTTOM };
enum _LayoutAlignmentX { ALIGN_X_LEFT,
						 ALIGN_X_CENTER,
						 ALIGN_X_RIGHT };
enum _LayoutAlignmentY { ALIGN_Y_TOP,
						 ALIGN_Y_CENTER,
						 ALIGN_Y_BOTTOM };
struct layout_config {
	_layout_sizing sizing = {SIZING_FIT(), SIZING_FIT()};
	struct {
		uint16_t left = 0;
		uint16_t right = 0;
		uint16_t top = 0;
		uint16_t bottom = 0;
	} padding;
	uint16_t child_gap = 0;
	_LayoutDirection direction = LEFT_TO_RIGHT;
	struct {
		_LayoutAlignmentX x = ALIGN_X_LEFT;
		_LayoutAlignmentY y = ALIGN_Y_TOP;
	} alignment;
	char *id = nullptr;
};
struct bounding_box {
	uint16_t x = 0;
	uint16_t y = 0;
	uint16_t width = 0;
	uint16_t height = 0;
};
// Basic component, can be used as a container
class component {
  public:
	layout_config get_config() { return config; }

  protected:
	layout_config config;
	// K-Tree
	struct {
		component *parent = nullptr;

		component *next = nullptr;
		component *prev = nullptr;

		component *begin = nullptr;
		component *end = nullptr;
		uint32_t count = 0;
	} tree;
	// Tree traversing and manuplation methods
	// Returned value of those should not be tampered
  public:
	void push_back(component *child) {
		if (child == nullptr)
			return;
		if (tree.begin == nullptr) {
			tree.begin = child;
			tree.end = child;
		} else {
			tree.end->tree.next = child;
			child->tree.prev = tree.end;
			tree.end = child;
		}
		child->tree.parent = this;
		tree.count++;
	}
	void insert_next(component *pos, component *child) {
		if (child == nullptr)
			return;
		if (pos->tree.next)
			pos->tree.next->tree.prev = child;
		else
			pos->tree.parent->tree.end = child;
		child->tree.next = pos->tree.next;
		pos->tree.next = child;
		child->tree.prev = pos;
		child->tree.parent = pos->tree.parent;
		pos->tree.parent->tree.count++;
	}
	component *detach(component *child) {
		if (child == nullptr)
			return nullptr;
		if (child->tree.next)
			child->tree.next->tree.prev = child->tree.prev;
		if (child->tree.prev)
			child->tree.prev->tree.next = child->tree.next;
		if (child->tree.parent) {
			child->tree.parent->tree.count--;
			if (child == child->tree.parent->tree.begin)
				child->tree.parent->tree.begin = child->tree.next;
			if (child == child->tree.parent->tree.end)
				child->tree.parent->tree.end = child->tree.prev;
		}
		child->tree.parent = nullptr;
		child->tree.next = nullptr;
		child->tree.prev = nullptr;
		return child;
	}

	component *next_preorder(component *stop_at = nullptr) {
		component *temp = this;
		if (temp->tree.begin)
			return temp->tree.begin;
		while (temp) {
			if (temp->tree.next)
				return temp->tree.next;
			if (temp->tree.parent == stop_at)
				return nullptr;
			temp = temp->tree.parent;
		}
		return nullptr;
	}
	component *prev_preorder(component *stop_at = nullptr) {
		component *temp = this;
		while (temp) {
			if (temp == stop_at)
				return nullptr;
			if (temp->tree.prev) {
				temp = temp->tree.prev;
				while (temp->tree.end)
					temp = temp->tree.end;
				return temp;
			}
			return temp->tree.parent;
		}
		return nullptr;
	}

	component *next_postorder(component *stop_at = nullptr) {
		component *temp = this;
		while (temp) {
			if (temp == stop_at)
				return nullptr;
			if (temp->tree.next) {
				temp = temp->tree.next;
				while (temp->tree.begin)
					temp = temp->tree.begin;
				return temp;
			}
			return temp->tree.parent;
		}
		return nullptr;
	}
	component *prev_postorder(component *stop_at = nullptr) {
		component *temp = this;
		if (temp->tree.end)
			return temp->tree.end;
		while (temp) {
			if (temp->tree.prev)
				return temp->tree.prev;
			if (temp->tree.parent == stop_at)
				return nullptr;
			temp = temp->tree.parent;
		}
		return nullptr;
	}
	component *next_sibling() {
		return this->tree.next;
	}
	component *prev_sibling() {
		return this->tree.prev;
	}
	component *first_child() {
		return this->tree.begin;
	}
	component *last_child() {
		return this->tree.end;
	}

  public:
	bounding_box box;


  public:
	// Usual usage methods
	component(layout_config config, std::initializer_list<component *> children = {}) : config(config) {
		for (component *child : children) {
			push_back(child);
		}
	}
	virtual ~component() {
		while (tree.begin != nullptr) {
			delete detach(tree.begin);
		}
	}

	void layout() {
		component *temp = this;
		while (temp) {
			temp->on_init_layout();
			temp = temp->next_preorder(this);
		}
		temp = this;
		while (temp->tree.begin)
			temp = temp->tree.begin;
		while (temp) {
			temp->on_fit_along();
			temp = temp->next_postorder(this);
		}
		temp = this;
		while (temp) {
			temp->on_child_grow_along();
			temp = temp->next_preorder(this);
		}
		temp = this;
		while (temp->tree.begin)
			temp = temp->tree.begin;
		while (temp) {
			temp->on_wrap();
			temp->on_fit_across();
			temp = temp->next_postorder(this);
		}
		temp = this;
		while (temp) {
			temp->on_child_grow_across();
			temp = temp->next_preorder(this);
		}
		temp = this;
		while (temp) {
			temp->on_child_pos();
			temp = temp->next_preorder(this);
		}
	}

	// Layout hook
  protected:
	virtual void on_init_layout() {
		box = {0, 0, 0, 0};
	}
	virtual void on_fit_along() {
		if (tree.parent) {
			if (config.direction == LEFT_TO_RIGHT)
				on_fit_x(true);
			else
				on_fit_y(true);
		} else
			on_fit_x(true);
	}
	virtual void on_fit_across() {
		if (tree.parent) {
			if (config.direction == LEFT_TO_RIGHT)
				on_fit_y(false);
			else
				on_fit_x(false);
		} else
			on_fit_y(false);
	}
	virtual void on_child_grow_along() {
		if (config.direction == LEFT_TO_RIGHT)
			on_child_grow_x_along();
		else
			on_child_grow_y_along();
	}
	virtual void on_child_grow_across() {
		if (config.direction == LEFT_TO_RIGHT)
			on_child_grow_y_across();
		else
			on_child_grow_x_across();
	}
	virtual void on_child_pos() {
		if (config.direction == LEFT_TO_RIGHT) {
			on_child_pos_x_along();
			on_child_pos_y_across();
		} else {
			on_child_pos_y_along();
			on_child_pos_x_across();
		}
	}
	virtual void on_wrap() {
	}
	// Basic layout algorithm
  private:
	inline void on_fit_x(bool along) {
		if (config.sizing.x.mode == _FIXED)
			box.width = config.sizing.x.min;
		else {
			if (along)
				box.width += config.child_gap *
							 ((tree.count > 1) ? (tree.count - 1) : 0);
			box.width += config.padding.left + config.padding.right;
			if (box.width < config.sizing.x.min)
				box.width = config.sizing.x.min;
		}
		if (tree.parent) {
			if (tree.parent->config.sizing.x.mode != _FIXED) {
				if (along)
					tree.parent->box.width += box.width;
				else
					tree.parent->box.width = std::max(tree.parent->box.width, box.width);
			}
		}
	}
	inline void on_fit_y(bool along) {
		if (config.sizing.y.mode == _FIXED)
			box.height = config.sizing.y.min;
		else {
			if (along)
				box.height += config.child_gap *
							  ((tree.count > 1) ? (tree.count - 1) : 0);
			box.height += config.padding.top + config.padding.bottom;
			if (box.height < config.sizing.y.min)
				box.height = config.sizing.y.min;
		}
		if (tree.parent) {
			if (tree.parent->config.sizing.y.mode != _FIXED) {
				if (along)
					tree.parent->box.height += box.height;
				else
					tree.parent->box.height = std::max(tree.parent->box.height, box.height);
			}
		}
	}
	inline void on_child_grow_x_along() {
		int32_t remaining_width = box.width - config.padding.left - config.padding.right;
		if (tree.count > 1)
			remaining_width -= config.child_gap * (tree.count - 1);
		std::list<component *> grow_components;
		for (component *i = tree.begin; i; i = i->tree.next) {
			if (i->config.sizing.x.mode == _GROW) {
				grow_components.push_back(i);
			}
		}
		if (grow_components.size() == 0)
			return;
		while (remaining_width > 0) {
			uint16_t smallest = UINT16_MAX;
			uint16_t second_smallest = UINT16_MAX;
			float_t count = 0;
			uint16_t width_to_add = remaining_width;
			uint16_t width_add_cap = UINT16_MAX;

			for (component *i : grow_components) {
				count += i->config.sizing.x.percent;
				uint16_t real_width = (i->box.width / i->config.sizing.x.percent);
				uint16_t real_width_add_cap = i->config.sizing.x.max / i->config.sizing.x.percent - real_width;
				width_add_cap = std::min(width_add_cap, real_width_add_cap);
				if (real_width < smallest) {
					second_smallest = smallest;
					smallest = real_width;
				}
				if (real_width > smallest) {
					second_smallest = std::min(second_smallest, real_width);
					width_to_add = second_smallest - smallest;
				}
			}

			width_to_add = std::min({width_to_add, uint16_t(remaining_width / count), width_add_cap});
			if (width_to_add == 0)
				break;
			for (auto i = grow_components.begin(); i != grow_components.end();) {
				(*i)->box.width += width_to_add * (*i)->config.sizing.x.percent;
				remaining_width -= width_to_add;
				if ((*i)->box.width >= (*i)->config.sizing.x.max) {
					grow_components.erase(i++);
				} else
					i++;
			}
		}
	}
	inline void on_child_grow_y_along() {
		int32_t remaining_height = box.height - config.padding.top - config.padding.bottom;
		if (tree.count > 1)
			remaining_height -= config.child_gap * (tree.count - 1);
		std::list<component *> grow_components;
		for (component *i = tree.begin; i; i = i->tree.next) {
			if (i->config.sizing.y.mode == _GROW) {
				grow_components.push_back(i);
			}
		}
		if (grow_components.size() == 0)
			return;
		while (remaining_height > 0) {
			uint16_t smallest = UINT16_MAX;
			uint16_t second_smallest = UINT16_MAX;
			float_t count = 0;
			uint16_t height_to_add = remaining_height;
			uint16_t height_add_cap = UINT16_MAX;
			for (component *i : grow_components) {
				count += i->config.sizing.y.percent;
				uint16_t real_height = (i->box.height / i->config.sizing.y.percent);
				uint16_t real_height_add_cap = i->config.sizing.y.max / i->config.sizing.y.percent - real_height;
				height_add_cap = std::min(height_add_cap, real_height_add_cap);
				if (real_height < smallest) {
					second_smallest = smallest;
					smallest = real_height;
				}
				if (real_height > smallest) {
					second_smallest = std::min(second_smallest, real_height);
					height_to_add = second_smallest - smallest;
				}
			}

			height_to_add = std::min({height_to_add, uint16_t(remaining_height / count), height_add_cap});
			if (height_to_add == 0)
				break;
			for (auto i = grow_components.begin(); i != grow_components.end();) {
				(*i)->box.height += height_to_add * (*i)->config.sizing.y.percent;
				remaining_height -= height_to_add;
				if ((*i)->box.height >= (*i)->config.sizing.y.max) {
					grow_components.erase(i++);
				} else
					i++;
			}
		}
	}
	inline void on_child_grow_x_across() {
		for (component *i = tree.begin; i; i = i->tree.next) {
			if (i->config.sizing.x.mode != _GROW)
				continue;
			i->box.width = std::min(
				uint16_t((box.width - config.padding.left - config.padding.right) * i->config.sizing.x.percent),
				i->config.sizing.x.max);
		}
	}
	inline void on_child_grow_y_across() {
		for (component *i = tree.begin; i; i = i->tree.next) {
			if (i->config.sizing.y.mode != _GROW)
				continue;
			i->box.height = std::min(
				uint16_t((box.height - config.padding.left - config.padding.right) * i->config.sizing.y.percent),
				i->config.sizing.y.max);
		}
	}
	struct _pos_along_center_commponents {
		uint16_t length;
		std::list<component *> components;
	};
	inline void on_child_pos_x_along() {
		// First pass: Add temporary components for centering childrens
		std::list<component *> temp_center_components;
		if (tree.count == 0)
			return;
		for (component *i = tree.begin; i->tree.next; i = i->tree.next) {
			if (i->config.alignment.x == ALIGN_X_RIGHT && i->tree.next->config.alignment.x == ALIGN_X_LEFT) {
				component *temp = new component({.alignment = {.x = ALIGN_X_CENTER}});
				insert_next(i, temp);
				temp_center_components.push_back(temp);
			}
		}
		// Second pass: Position leftmost and rightmost childrens
		component *start = tree.begin;
		component *end = tree.end;
		uint16_t left_offset = config.padding.left;
		for (; start; start = start->tree.next) {
			if (start->config.alignment.x != ALIGN_X_LEFT)
				break;
			if (start->tree.prev)
				start->box.x = start->tree.prev->box.x +
							   start->tree.prev->box.width +
							   config.child_gap;
			else
				start->box.x = box.x + config.padding.left;
			left_offset += start->box.width + config.child_gap;
		}
		for (; end; end = end->tree.prev) {
			if (end->config.alignment.x != ALIGN_X_RIGHT)
				break;
			if (end->tree.next)
				end->box.x = end->tree.next->box.x -
							 end->box.width -
							 config.child_gap;
			else
				end->box.x = box.x + box.width - end->box.width - config.padding.right;
		}
		if (start == nullptr)
			return;
		if (end == nullptr)
			return;
		// Third pass: Position center childrens
		std::list<_pos_along_center_commponents> center_components;
		for (component *i = tree.begin; i; i = i->tree.next) {
			if (i->config.alignment.x != ALIGN_X_CENTER)
				continue;
			bool is_connected = false;
			if (i->tree.prev)
				if (i->tree.prev->config.alignment.x == ALIGN_X_CENTER) {
					center_components.back().length += i->box.width + config.child_gap;
					center_components.back().components.push_back(i);
					is_connected = true;
				}
			if (!is_connected)
				center_components.push_back({uint16_t(i->box.width + config.child_gap), {i}});
		}
		uint16_t count = 0;
		for (auto &i : center_components) {
			count++;
			uint16_t start_pos = box.width / (center_components.size() + 1) * count;
			start_pos -= i.length / 2;
			for (auto j : i.components) {
				j->box.x = box.x + std::max(start_pos, left_offset);
				start_pos += j->box.width + config.child_gap;
				left_offset += j->box.width + config.child_gap;
			}
		}
		// Fourth pass: Remaining left components
		for (component *i = start; i != end->tree.next; i = i->tree.next) {
			if (i->config.alignment.x != ALIGN_X_LEFT)
				continue;
			i->box.x = i->tree.prev->box.x +
					   i->tree.prev->box.width +
					   config.child_gap;
		}
		// Fifth pass: Remaining right components
		for (component *i = end; i != start->tree.prev; i = i->tree.prev) {
			if (i->config.alignment.x != ALIGN_X_RIGHT)
				continue;
			i->box.x = i->tree.next->box.x -
					   i->box.width -
					   config.child_gap;
		}
		// Cleanup
		for (component *i : temp_center_components) {
			delete detach(i);
		}
	}
	inline void on_child_pos_y_along() {
		std::list<component *> temp_center_components;
		// First pass: Add temporary components for centering childrens
		for (component *i = tree.begin; i->tree.next; i = i->tree.next) {
			if (i->config.alignment.y == ALIGN_Y_BOTTOM && i->tree.next->config.alignment.y == ALIGN_Y_TOP) {
				component *temp = new component({.alignment = {.y = ALIGN_Y_CENTER}});
				insert_next(i, temp);
				temp_center_components.push_back(temp);
			}
		}
		// Second pass: Position topmost and bottommost childrens
		component *start = tree.begin;
		component *end = tree.end;
		uint16_t top_offset = config.padding.top;
		for (; start; start = start->tree.next) {
			if (start->config.alignment.y != ALIGN_Y_TOP)
				break;
			if (start->tree.prev)
				start->box.y = start->tree.prev->box.y +
							   start->tree.prev->box.height +
							   config.child_gap;
			else
				start->box.y = config.padding.top;
			top_offset += start->box.height + config.child_gap;
		}
		for (; end; end = end->tree.prev) {
			if (end->config.alignment.y != ALIGN_Y_BOTTOM)
				break;
			if (end->tree.next)
				end->box.y = end->tree.next->box.y -
							 end->box.height -
							 config.child_gap;
			else
				end->box.y = box.height - end->box.height - config.padding.bottom;
		}
		if (start == nullptr)
			return;
		if (end == nullptr)
			return;
		// Third pass: Position center childrens
		std::list<_pos_along_center_commponents> center_components;
		for (component *i = tree.begin; i; i = i->tree.next) {
			if (i->config.alignment.y != ALIGN_Y_CENTER)
				continue;
			bool is_connected = false;
			if (i->tree.prev)
				if (i->tree.prev->config.alignment.y == ALIGN_Y_CENTER) {
					center_components.back().length += i->box.height + config.child_gap;
					center_components.back().components.push_back(i);
					is_connected = true;
				}
			if (!is_connected)
				center_components.push_back({uint16_t(i->box.height + config.child_gap), {i}});
		}
		uint16_t count = 0;
		for (auto &i : center_components) {
			count++;
			uint16_t start_pos = box.height / (center_components.size() + 1) * count;
			start_pos -= i.length / 2;
			for (auto j : i.components) {
				j->box.y = box.y + std::max(start_pos, top_offset);
				start_pos += j->box.height + config.child_gap;
				top_offset = start_pos;
			}
		}
		// Fourth pass: Remaining top components
		for (component *i = start; i != end->tree.next; i = i->tree.next) {
			if (i->config.alignment.y != ALIGN_Y_TOP)
				continue;
			i->box.y = i->tree.prev->box.y +
					   i->tree.prev->box.height +
					   config.child_gap;
		}
		// Fifth pass: Remaining bottom components
		for (component *i = end; i != start->tree.prev; i = i->tree.prev) {
			if (i->config.alignment.y != ALIGN_Y_BOTTOM)
				continue;
			i->box.y = i->tree.next->box.y -
					   i->box.height -
					   config.child_gap;
		}
		// Cleanup
		for (component *i : temp_center_components) {
			delete detach(i);
		}
	}
	inline void on_child_pos_x_across() {
		for (component *i = tree.begin; i; i = i->tree.next) {
			switch (i->config.alignment.x) {
			case ALIGN_X_LEFT:
				i->box.x = box.x + config.padding.left;
				break;
			case ALIGN_X_CENTER:
				i->box.x = box.x + box.width / 2 - i->box.width / 2;
				break;
			case ALIGN_X_RIGHT:
				i->box.x = box.x + box.width - i->box.width - config.padding.right;
				break;
			}
		}
	}
	inline void on_child_pos_y_across() {
		for (component *i = tree.begin; i; i = i->tree.next) {
			switch (i->config.alignment.y) {
			case ALIGN_Y_TOP:
				i->box.y = box.y + config.padding.top;
				break;
			case ALIGN_Y_CENTER:
				i->box.y = box.y + box.height / 2 - i->box.height / 2;
				break;
			case ALIGN_Y_BOTTOM:
				i->box.y = box.y + box.height - i->box.height - config.padding.bottom;
				break;
			}
		}
	}
};
} // namespace cppreact

#endif
