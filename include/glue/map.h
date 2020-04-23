#pragma once

#include <glue/types.h>
#include <observe/event.h>

namespace glue {

  class ElementMapEntry;

  /**
   * Base class for element maps
   */
  class Map : public revisited::Visitable<Map> {
  public:
    virtual Any getValue(const Key &key) const = 0;
    virtual void setValue(const Key &key, Any &&value) = 0;
    virtual std::vector<Key> keys() const = 0;

    observe::Event<const Key &, const Any &> onValueChanged;

    Map() {}
    Map(const Map &) = delete;
    virtual ~Map() {}

    ElementMapEntry operator[](const Key &key);
  };

}  // namespace glue
