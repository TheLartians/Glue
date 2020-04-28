#pragma once

#include <glue/map.h>

#include <functional>
#include <optional>
#include <vector>

namespace glue {

  namespace detail {
    /**
     * detect callable types.
     * source: https://stackoverflow.com/questions/15393938/
     */
    template <class...> using void_t = void;
    template <class T> using has_opr_t = decltype(&T::operator());
    template <class T, class = void> struct is_callable : std::false_type {};
    template <class T> struct is_callable<T, void_t<has_opr_t<typename std::decay<T>::type>>>
        : std::true_type {};
  }  // namespace detail

  struct MapValue;
  struct FunctionValue;

  /**
   * The base class for value wrappers that store their actual data in a `data` member.
   */
  struct ValueBase {};

  struct Value : public ValueBase {
    Any data;

    Value() = default;
    Value(const Value &) = default;
    Value(Value &&) = default;

    template <class T> Value(T &&v) { *this = std::forward<T>(v); }

    Any value() const { return data; }

    MapValue asMap() const;
    AnyFunction asFunction() const;

    explicit operator bool() const { return bool(this->data); }
    const auto *operator-> () const { return &this->data; }
    auto *operator-> () { return &this->data; }
    const auto &operator*() const { return this->data; }
    auto &operator*() { return this->data; }

    template <class T, typename = typename std::enable_if<
                           !std::is_base_of<ValueBase, typename std::decay<T>::type>::value>::type>
    Value &operator=(T &&value) {
      if constexpr (detail::is_callable<T>::value) {
        **this = Any::create<AnyFunction>(value);
      } else {
        **this = std::forward<T>(value);
      }
      return *this;
    }

    template <class T>
    typename std::enable_if<std::is_base_of<ValueBase, typename std::decay<T>::type>::value,
                            Value>::type &
    operator=(T &&value) {
      **this = value.data;
      return *this;
    }

    Value &operator=(const Value &) = default;
  };

  struct MapValue : public ValueBase {
    std::shared_ptr<Map> data;

    MapValue() = default;
    MapValue(const MapValue &) = default;
    MapValue(MapValue &&) = default;
    MapValue(std::shared_ptr<Map> d) : data(std::move(d)) {}
    MapValue &operator=(const MapValue &) = default;

    struct MappedValue : public Value {
      Map &parent;
      std::string key;

      void set(const std::string &k, Any v) { parent.set(k, std::move(v)); }

      template <class T>
      typename std::enable_if<!std::is_base_of<ValueBase, typename std::decay<T>::type>::value,
                              MappedValue &>::type
      operator=(T &&value) {
        if constexpr (detail::is_callable<T>::value) {
          set(key, Any::create<AnyFunction>(value));
        } else {
          set(key, std::forward<T>(value));
        }
        return *this;
      }

      template <class T>
      typename std::enable_if<std::is_base_of<ValueBase, typename std::decay<T>::type>::value,
                              MappedValue &>::type
      operator=(T &&value) {
        return *this = value.data;
      }
    };

    Value get(const std::string &key) const;
    Value rawGet(const std::string &key) const { return data->get(key); }
    MappedValue operator[](const std::string &key) const {
      return MappedValue{{get(key)}, *data, key};
    }
    std::vector<std::string> keys() const;
    void forEach(std::function<bool(const std::string &, Value)>) const;

    void setExtends(Value v) const;

    explicit operator bool() const { return bool(this->data); }
    const auto *operator-> () const { return &this->data; }
    auto *operator-> () { return &this->data; }
    const auto &operator*() const { return this->data; }
    auto &operator*() { return this->data; }
  };

  MapValue createAnyMap();

}  // namespace glue
