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
    bool* changed_flag = 0;
    template<typename T>
    class property {
      T* data;
      state_system* owner;
      public:
      operator T() {return *data;}
      T& operator=(T value) const {
        *data = value;
        if (owner->changed_flag) *(owner->changed_flag) = true;
        return *data;
      }
      property(T* data,state_system* owner) : data(data),owner(owner) {}
    };
    template<typename T> 
    class _state_list {
      std::list<T> data;
      typename std::list<T>::iterator pos;
      public:
      _state_list() : 
        pos(data.end()) {}
      void reset() {pos = data.begin();}
      property<T> get(T& inp,state_system* owner) {
        if (pos == data.end()) {
          data.push_back(inp);
          pos = --data.end();
        }
        return property<T>(&*(pos++),owner);
      }
    };
    std::unordered_map<std::type_index, std::pair<bool,std::any>> _internal_data;
    public:
    state_system(bool* changed_flag = 0) : changed_flag(changed_flag) {};
    ~state_system() {
    }
    template<typename T> 
    property<T> get(T value = T()) {
      auto current = _internal_data.insert({std::type_index(typeid(T)),{0,_state_list<T>()}}).first;
      _state_list<T>& ptr = std::any_cast<_state_list<T>&>(current->second.second);
      if (current->second.first == 0) {
        ptr.reset();
        current->second.first = 1;
      }
      return ptr.get(value,this);
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
  enum {DYNAMIC_REGISTER_ID = 10};

  class func : public component {
    public:
    std::function<component*(state_system&)> f;
    dynamic_register_data data;
    state_system state;
    public:
    func(std::function<component*(state_system&)> f) : 
      f(f),component({}),data({false,this,nullptr}),state(&data.changed) {
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
    std::list<render_command> on_layout() override {
      std::list<render_command> res;
      if (data.ref == 0) res.push_back({.box = box,.id = DYNAMIC_REGISTER_ID,.data = &data});
      std::list<render_command> temp = std::move(component::on_layout());
      res.insert(res.end(),temp.begin(),temp.end());
      return res;
    }
  };
}
#endif
