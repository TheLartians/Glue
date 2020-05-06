#include <easy_iterator.h>
#include <glue/anymap.h>

using namespace glue;

Any AnyMap::get(const std::string &key) const {
  if (auto it = easy_iterator::find(data, key)) {
    return it->second;
  } else {
    return Any();
  }
}

void AnyMap::set(const std::string &key, const Any &value) { data[key] = value; }

bool AnyMap::forEach(const std::function<bool(const std::string &)> &callback) const {
  for (auto &&v : data) {
    if (callback(v.first)) return true;
  }
  return false;
}
