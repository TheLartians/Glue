#pragma once

#include <glue/map.h>
#include <glue/any_element.h>
#include <glue/element_map_entry.h>

#include <cctype>
#include <exception>
#include <unordered_map>

namespace glue {
  
  struct ClassInfo {
    revisited::TypeID typeID;
    revisited::TypeID constTypeID;
    revisited::TypeID sharedTypeID;
    revisited::TypeID sharedConstTypeID;
  };
  
  template <class T> ClassInfo createClassInfo() {
    ClassInfo result;
    result.typeID = revisited::getTypeID<T>();
    result.constTypeID = revisited::getTypeID<const T>();
    result.sharedTypeID = revisited::getTypeID<std::shared_ptr<T>>();
    result.sharedConstTypeID = revisited::getTypeID<std::shared_ptr<const T>>();
    return result;
  }
  
  template <class T> void setClassInfo(Element &element) {
    element[keys::classKey] = createClassInfo<T>();
  }

  inline auto getClassInfo(Element &element) {
    return element[keys::classKey].tryGet<ClassInfo>();
  }

  template <class T> AnyElement &AnyElement::addValue(const std::string &key, T &&value) {
    (*this)[key] = std::forward<T>(value);
    return *this;
  }
  
  template <class T> class ClassElement : public AnyElement {
  public:
    ClassElement() { setClassInfo<T>(*this); }

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

    ClassElement &setBase(const Element &e) {
      glue::setExtends(*this, e);
      return *this;
    }

    template <class O> ClassElement &addValue(const std::string &key, O &&value) {
      (*this)[key] = std::forward<O>(value);
      return *this;
    }
  };
  
}  // namespace glue
