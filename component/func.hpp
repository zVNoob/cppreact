#ifndef _CPPREACT_FUNC_HPP
#define _CPPREACT_FUNC_HPP
#include "component.hpp"
#include <cstddef>
#include <functional>
#include <list>
#include <tuple>
#include <unordered_map>
#include <typeindex>
#include <utility>


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
        return *pos;
      }
    };
    std::unordered_map<std::type_index, std::pair<bool,void*>> _internal_data;
    public:
    state_system() {};
    ~state_system() {
    }
    template<typename T> 
    T& get(T value = T()) {
      auto current = _internal_data.insert({std::type_index(typeid(T)),{1,0}}).first;
      if (current->second.second == 0) {
        current->second.second = new _state_list<T>();
      }
      _state_list<T>* ptr = reinterpret_cast<_state_list<T>*>(current->second.second);
      if (current->second.first == 0) {
        ptr->reset();
        current->second.first = 1;
      }
      return ptr->get(value);
    }
    template<typename T>
    void clear() {
      auto current = _internal_data.insert({std::type_index(typeid(T)),{1,0}}).first;
      if (current->second.second) {
        _state_list<T>* ptr = reinterpret_cast<_state_list<T>*>(current->second.second);
        delete ptr;
        current->second.second = 0;
      }
    }
    void reset() {for (auto&i:_internal_data) i.second.first = 0;}
  };
  template <size_t d,typename... Items>
  class _func;
  template <size_t d>
  class _func<d> : public component {
    public:
    std::function<component*(state_system&)> f;
    state_system state;
    public:
    _func(std::function<component*(state_system&)> f) : 
      f(f),component({}) {
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
  template <size_t d,typename... Items>
  class _func : public _func<d> {
    template<typename T>
    class TupleLeaf {
      public:
      state_system* _target;
      void target(state_system* t) {_target = t;}
      ~TupleLeaf() {_target->clear<T>();}
    };
    std::tuple<TupleLeaf<Items>...> auto_destruct;
    public:
    _func(std::function<component*(state_system&)> f) :
      _func<d>(f) {
      std::apply([this](auto&&... args) {
        (
          (args.target(&(this->_func<0>::state))),
          ...
        );
      },auto_destruct);
    }
  };
  template <typename... Items>
  using func = _func<0, Items...>;
}
#endif
