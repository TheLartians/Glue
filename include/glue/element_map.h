#pragma once

#include <glue/any_element.h>
#include <glue/element_map_entry.h>

namespace glue {
  
  /**
   * A Map implementation that stores it's data in Elements
   */
  class ElementMap : public revisited::DerivedVisitable<ElementMap, Map> {
  protected:
    std::unordered_map<std::string, AnyElement> data;
    std::unordered_map<std::string, lars::Observer> elementObservers;
    std::shared_ptr<Map> extends;

  public:
    Any getValue(const std::string &key) const final override;
    void setValue(const std::string &key, Any &&value) final override;
    std::vector<std::string> keys() const final override;
  };

}
