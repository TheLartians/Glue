#pragma once

namespace glue {

  /**
   * Internal keys
   */
  namespace keys {

    static constexpr auto constructorKey = "__new";
    static constexpr auto extendsKey = "__glue_extends";
    static constexpr auto classKey = "__glue_class";

    namespace operators {
      static constexpr auto eq = "__eq";
      static constexpr auto lt = "__lt";
      static constexpr auto le = "__le";
      static constexpr auto gt = "__gt";
      static constexpr auto ge = "__ge";
      static constexpr auto mul = "__mul";
      static constexpr auto div = "__div";
      static constexpr auto idiv = "__idiv";
      static constexpr auto add = "__add";
      static constexpr auto sub = "__sub";
      static constexpr auto mod = "__mod";
      static constexpr auto pow = "__pow";
      static constexpr auto unm = "__unm";
      static constexpr auto tostring = "__tostring";
    }  // namespace operators
  }    // namespace keys

}  // namespace glue
