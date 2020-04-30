#pragma once

#include <glue/value.h>

#include <exception>

namespace glue {

  struct View : public ValueBase {
    Value data;

    View() = default;
    View(const View &) = default;

    template <class T,
              class Enable = typename std::enable_if<std::is_base_of<ValueBase, T>::value>::type>
    View(const T &d) : data(*d) {}
    template <class T,
              class Enable = typename std::enable_if<!std::is_base_of<ValueBase, T>::value>::type>
    View(T d) : data(std::move(d)) {}

    View operator[](const std::string &key) const {
      if (auto map = data.asMap()) {
        return View(map[key]);
      } else {
        throw std::runtime_error("value is not a map");
      }
    }

    template <typename... Args> View operator()(Args &&... args) const {
      if (auto f = data.asFunction()) {
        return View(f(std::forward<Args>(args)...));
      } else {
        throw std::runtime_error("value is not a map");
      }
    }

    auto operator*() const { return *data; }
    auto operator-> () const { return data; }
  };

}  // namespace glue