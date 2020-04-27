#pragma once

#include <glue/instance.h>
#include <glue/keys.h>
#include <glue/value.h>

#include <cctype>

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

  template <class T> void setClassInfo(const MapValue &value) {
    value[keys::classKey] = createClassInfo<T>();
  }

  inline auto getClassInfo(const MapValue &value) {
    return value[keys::classKey]->as<const ClassInfo &>();
  }

  template <class T, class... Bases> struct ClassGenerator : public glue::ValueBase {
    MapValue data = createAnyMap();

    ClassGenerator() { setClassInfo<T>(data); }

    template <class B, class R, typename... Args>
    ClassGenerator &addMethod(const std::string &name, R (B::*f)(Args...)) {
      static_assert(std::is_base_of<B, T>::value);
      data[name]
          = [f](T &o, Args... args) { return std::invoke(f, o, std::forward<Args>(args)...); };
      return *this;
    }

    template <class B, class R, typename... Args>
    ClassGenerator &addMethod(const std::string &name, R (B::*f)(Args...) const) {
      static_assert(std::is_base_of<B, T>::value);
      data[name] = [f](const T &o, Args... args) {
        return std::invoke(f, o, std::forward<Args>(args)...);
      };
      return *this;
    }

    template <typename... Args> ClassGenerator &addConstructor() {
      data[keys::constructorKey] = [](Args... args) {
        revisited::Any v;
        if constexpr (sizeof...(Bases) > 0) {
          v.setWithBases<T, Bases...>(std::forward<Args>(args)...);
        } else {
          v = T(std::forward<Args>(args)...);
        }
        return v;
      };
      return *this;
    }

    template <typename... Args, class... B>
    ClassGenerator &addConstructorWithBases(revisited::TypeList<B...>) {
      data[keys::constructorKey] = [](Args... args) { return T(std::forward<Args>(args)...); };
      return *this;
    }

    template <class O> ClassGenerator &addConstMember(const std::string &name, O T::*ptr) {
      data[name] = [ptr](const T &o) { return o.*ptr; };
      return *this;
    }

    template <class O> ClassGenerator &addMember(const std::string &name, O T::*ptr) {
      addConstMember(name, ptr);
      std::string setName = "set" + name;
      setName[3] = char(toupper(setName[3]));
      data[setName] = [ptr](T &o, const O &v) { o.*ptr = v; };
      return *this;
    }

    template <class F> ClassGenerator &addMethod(const std::string &name, F f) {
      data[name] = AnyFunction(f);
      return *this;
    }

    template <class O> ClassGenerator &setExtends(const O &base) {
      data[keys::extendsKey] = MapValue(base);
      return *this;
    }

    explicit operator MapValue() const { return data; }

    template <typename... Args> Instance construct(Args &&... args) {
      return Instance(data, data[keys::constructorKey].asFunction()(std::forward<Args>(args)...));
    }
  };

  template <class T, class... B> auto createClass() { return ClassGenerator<T, B...>(); }

}  // namespace glue