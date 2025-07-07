#ifndef _CPPREACT_FUNC_HPP
#define _CPPREACT_FUNC_HPP

#include "component.hpp"
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <any>

namespace cppreact {
  template <typename T>
  class Property {
    T* value;
    bool* changed;
    public:
      Property(T* value, bool* changed) {
        this->value = value;
        this->changed = changed;
      }
    operator T() {
      return *value;
    }
    T& operator=(T value) {
      if (*this->value != value) {
        *this->value = value;
        *this->changed = true;
      }
      return *this->value;
    }
  };

  class state_system {
    template<typename T> 
    class _state_list {
      std::list<T> data;
      typename std::list<T>::iterator pos;
      bool* changed;
      public:
      _state_list(bool* changed) : changed(changed),
        pos(data.end()) {}
      void reset() {pos = data.begin();}
      Property<T> get(T& inp) {
        if (pos == data.end()) {
          data.push_back(inp);
          pos = --data.end();
        }
        return Property<T>(&*(pos++),changed);
      }
    };
    std::unordered_map<std::type_index, std::pair<bool,std::any>> _internal_data;
    public:
    bool changed;
    state_system() {changed = true;}
    template<typename T> 
    Property<T> get(T value = T()) {
      auto current = _internal_data.insert({std::type_index(typeid(T)),{0,_state_list<T>(&changed)}}).first;
      _state_list<T>& ptr = std::any_cast<_state_list<T>&>(current->second.second);
      if (current->second.first == 0) {
        ptr.reset();
        current->second.first = 1;
      }
      return ptr.get(value);
    }
    void reset() {for (auto&i:_internal_data) i.second.first = 0;}
  };
  // Functional component, accepts a function that returns a component for dynamic output with currying via state
  class func : public component {
    state_system state;
    std::function<component*(state_system&)> f;
    public:
    func(std::function<component*(state_system&)> f) : component({}) {
      this->f = f;
    }
    protected:
    void on_init_layout() override {
      component::on_init_layout();
      if (state.changed) {
        if (tree.begin)
          delete detach(tree.begin);
        if (f) 
          push_back(f(state));
        state.reset();
        state.changed = false;
        if (tree.begin) {
          config = tree.begin->get_config();
          config.padding = {0,0,0,0};
        }
      }
    }
  };
}

#endif
