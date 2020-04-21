#pragma once

#include <glue/element.h>

#include <cctype>
#include <exception>
#include <unordered_map>

namespace glue {

  class BoundAny;

  class ClassElementContext {
  public:
    void addMap(const std::shared_ptr<glue::Map> &);
    void addElement(const ElementInterface &);
    std::shared_ptr<glue::Map> getMapForType(const revisited::TypeIndex &) const;
    BoundAny bind(revisited::Any &&v) const;

  private:
    std::unordered_map<revisited::TypeIndex, std::shared_ptr<glue::Map>> types;
  };

  class BoundAny {
  public:
    BoundAny(const ClassElementContext &c, revisited::Any &&v)
        : data(std::move(v)), context(c), map(context.getMapForType(data.type())) {}

    auto operator[](const std::string &name) {
      return [this, name](auto &&... args) -> BoundAny {
        if (!map) {
          throw std::runtime_error("called glue accessor on non-registered type");
        } else {
          return context.bind((*map)[name](data, args...));
        }
      };
    }

    const revisited::Any &operator*() const { return data; }
    const revisited::Any *operator->() const { return &data; }

  private:
    revisited::Any data;
    const ClassElementContext &context;
    std::shared_ptr<glue::Map> map;
  };

  template <class T> class ClassElement : public Element {
  public:
    ClassElement() { setClass<T>(*this); }

    template <typename... Args>
    ClassElement &addConstructor(const std::string &name = keys::constructorKey) {
      (*this)[name] = [](Args... args) { return T(args...); };
      return *this;
    }

    template <class B, class R, typename... Args>
    ClassElement &addNonConstMethod(const std::string &name, R (B::*f)(Args...)) {
      (*this)[name]
          = [f](T &o, Args &&... args) { return std::invoke(f, o, std::forward<Args>(args)...); };
      return *this;
    }

    template <class B, class R, typename... Args>
    ClassElement &addConstMethod(const std::string &name, R (B::*f)(Args...) const) {
      (*this)[name] = [f](const T &o, Args &&... args) {
        return std::invoke(f, o, std::forward<Args>(args)...);
      };
      return *this;
    }

    template <class B, class R, typename... Args>
    ClassElement &addMethod(const std::string &name, R (B::*f)(Args...)) {
      return addNonConstMethod(name, f);
    }

    template <class B, class R, typename... Args>
    ClassElement &addMethod(const std::string &name, R (B::*f)(Args...) const) {
      return addConstMethod(name, f);
    }

    ClassElement &addMethod(const std::string &name, revisited::AnyFunction f) {
      return addFunction(name, f);
    }

    ClassElement &addFunction(const std::string &name, revisited::AnyFunction f) {
      (*this)[name] = f;
      return *this;
    }

    template <class O> ClassElement &addConstMember(const std::string &name, O T::*ptr) {
      (*this)[name] = [ptr](const T &o) { return o.*ptr; };
      return *this;
    }

    template <class O> ClassElement &addMember(const std::string &name, O T::*ptr) {
      if (name.size() == 0) {
        throw std::runtime_error("glue: member must have a valid name");
      }
      addConstMember(name, ptr);
      std::string setName = "set" + name;
      setName[3] = char(toupper(setName[3]));
      (*this)[setName] = [ptr](T &o, const O &v) { o.*ptr = v; };
      return *this;
    }

    ClassElement &setExtends(const ElementInterface &e) {
      glue::setExtends(*this, e);
      return *this;
    }

    template <class O> ClassElement &addValue(const std::string &key, O &&value) {
      (*this)[key] = std::forward<O>(value);
      return *this;
    }
  };

}  // namespace glue
