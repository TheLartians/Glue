
#include <revisited/any.h>
#include <revisited/any_function.h>
#include <optional>
#include <functional>

namespace glue2 {

  using Any = revisited::Any;
  using AnyFunction = revisited::AnyFunction;

  struct Map: public revisited::Visitable<Map> {
    virtual Any get(const std::string &) const = 0;
    virtual void set(const std::string &, const Any &) = 0;
    virtual bool forEach(std::function<bool(const std::string &, const Any &)>) const = 0;
  };

  struct MapValue;

  struct Value {
    Any data;

    Any value()const { return data; }
    MapValue asMap() const;
    std::optional<AnyFunction> asFunction() const;

    explicit operator bool() const { return bool(this->data); }
    const auto *operator->() const { return &this->data; }
    auto *operator->() { return &this->data; }
    const auto &operator*() const { return this->data; }
    auto &operator*() { return this->data; }
  };

  struct MapValue {
    std::shared_ptr<Map> data;

    struct MappedValue: public Value {
      Map &parent;
      std::string key;
      MappedValue &operator=(const Any &value){ parent.set(key, value); return *this; }
    };

    MappedValue operator[](const std::string &key) const { return MappedValue{{data->get(key)}, *data, key}; }

    explicit operator bool() const { return bool(this->data); }
    const auto *operator->() const { return &this->data; }
    auto *operator->() { return &this->data; }
    const auto &operator*() const { return this->data; }
    auto &operator*() { return this->data; }
  };

}
