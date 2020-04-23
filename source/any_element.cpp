#include <easy_iterator.h>
#include <glue/any_element.h>
#include <glue/element_map.h>

using namespace glue;
using easy_iterator::eraseIfFound;
using easy_iterator::found;

void AnyElement::setValue(Any &&value) { data = value; }

Any AnyElement::getValue() const { return data; }

Map &AnyElement::setToMap() { return set<ElementMap>(); }
