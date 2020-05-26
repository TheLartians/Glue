#include <doctest/doctest.h>
#include <glue/context.h>
#include <glue/enum.h>

using namespace glue;

namespace {

  enum class E : int { A = 0, B, C };

}  // namespace

TEST_CASE("EnumValue") {
  MapValue enumGlue = createEnum<E>().addValue("A", E::A).addValue("B", E::B).addValue("C", E::C);

  SUBCASE("values") {
    CHECK(enumGlue["A"]->get<E>() == E::A);
    CHECK(enumGlue["B"]->get<E>() == E::B);
    CHECK(enumGlue["C"]->get<E>() == E::C);
  }

  SUBCASE("as object") {
    glue::Context context;
    glue::Context::Path path{"E"};
    context.addMap(enumGlue, path);
    auto instance = context.createInstance(enumGlue["A"]);
    CHECK(instance[glue::keys::operators::eq](enumGlue["B"]).as<bool>());
    CHECK(instance["value"]().as<int>() == int(E::A));
  }
}
