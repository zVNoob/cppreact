#ifndef _CPPREACT_FUNC_HPP
#define _CPPREACT_FUNC_HPP
#include "component.hpp"
#include <functional>
#include <list>
#include <any>
#include <unordered_map>
#include <typeindex>

namespace cppreact {
  class state_system {
    template<typename T> 
    class _state_list {
      std::list<T> data;
      typename std::list<T>::iterator pos;
      public:
      _state_list() : 
        pos(data.end()) {}
      void reset() {pos = data.begin();}
      T& get(T& inp) {
        if (pos == data.end()) {
          data.push_back(inp);
          pos = --data.end();
        }
        return *(pos++);
      }
    };
    std::unordered_map<std::type_index, std::pair<bool,std::any>> _internal_data;
    public:
    template<typename T> 
    T& get(T value = T()) {
      auto current = _internal_data.insert({std::type_index(typeid(T)),{0,_state_list<T>()}}).first;
      _state_list<T>& ptr = std::any_cast<_state_list<T>&>(current->second.second);
      if (current->second.first == 0) {
        ptr.reset();
        current->second.first = 1;
      }
      return ptr.get(value);
    }
    void reset() {for (auto&i:_internal_data) i.second.first = 0;}
  };

  struct dynamic_register_data {
    bool changed;
    component* obj;
    dynamic_register_data** ref;
    ~dynamic_register_data() {
      if (ref) *ref = 0;
    }
  };

  class func : public component {
    public:
    std::function<component*(state_system&)> f;
    dynamic_register_data data;
    state_system state;
    public:
    func(std::function<component*(state_system&)> f) : 
      f(f),component({}),data({false,this,nullptr}) {
    }
    protected:
    void on_init_layout() override {
      component::on_init_layout();
      if (k_tree.child_begin) delete k_tree.child_begin;
      k_tree.child_begin = k_tree.child_end = f(state);
      state.reset();
      k_tree.child_begin->k_tree.parent = this;
      config = k_tree.child_begin->config;
      config.padding = {0,0,0,0};
      config.child_gap = 0;
    }
  };
}
#endif
