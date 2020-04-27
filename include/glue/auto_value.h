#include <glue/value.h>

namespace glue {

  struct AutoValue : public ValueBase {
    std::shared_ptr<Value> value;
  };

}  // namespace glue
