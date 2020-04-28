#pragma once

#include <glue/instance.h>
#include <glue/keys.h>
#include <glue/value.h>

#include <cctype>

namespace glue {

  using TypeID = revisited::TypeID;
  using TypeIndex = revisited::TypeIndex;
  template <class T> auto getTypeIndex() { return revisited::getTypeIndex<T>(); }
  template <class T> auto getTypeID() { return revisited::getTypeID<T>(); }

  struct ClassInfo {
    TypeID typeID;
    TypeID constTypeID;
    TypeID sharedTypeID;
    TypeID sharedConstTypeID;

    /**
     * Optional method that will be used to convert arbitrary Any objects of this type
     * to Any objects supporting conversions.
     */
    std::function<Any(Any)> converter;
  };

  template <class T> ClassInfo createClassInfo() {
    ClassInfo result;
    result.typeID = getTypeID<T>();
    result.constTypeID = getTypeID<const T>();
    result.sharedTypeID = getTypeID<std::shared_ptr<T>>();
    result.sharedConstTypeID = getTypeID<std::shared_ptr<const T>>();
    return result;
  }

  inline auto getClassInfo(const MapValue &value) {
    return value[keys::classKey]->getShared<ClassInfo>();
  }

  template <typename... args> struct WithBases {};

  template <class T> struct ClassGenerator : public ValueBase {
    MapValue data = createAnyMap();

    template <class... Bases> ClassGenerator(WithBases<Bases...>) {
      auto classInfo = createClassInfo<T>();
      if constexpr (sizeof...(Bases) > 0) {
        classInfo.converter = [](Any value) {
          if (auto t = value.getShared<T>()) {
            value.setWithBases<T, Bases...>(*t);
          } else if (auto ts = value.getShared<const T>()) {
            // value.setWithBases<std::shared_ptr<const T>, Bases...>(ts);
          }
          return value;
        };
      }
      data[keys::classKey] = classInfo;
    }

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

    template <class O>
    typename std::enable_if<std::is_base_of<ValueBase, O>::value, ClassGenerator &>::type
    setExtends(const O &base) {
      data[keys::extendsKey] = base.data;
      return *this;
    }

    explicit operator MapValue() const { return data; }

    template <typename... Args> Instance construct(Args &&... args) const {
      auto classInfo = getClassInfo(data);
      auto value = data[keys::constructorKey].asFunction()(std::forward<Args>(args)...);
      if (classInfo && classInfo->converter) {
        value = classInfo->converter(value);
      }
      return Instance(data, value);
    }
  };

  template <class T, class... B> auto createClass(WithBases<B...> bases = WithBases<>()) {
    return ClassGenerator<T>(bases);
  }

}  // namespace glue