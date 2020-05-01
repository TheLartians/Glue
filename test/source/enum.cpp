#include <doctest/doctest.h>
#include <glue/enum.h>

using namespace glue;

namespace {

  enum class E { A, B, C };

}  // namespace

TEST_CASE("EnumValue") {
  MapValue enumGlue = createEnum<E>().addValue("A", E::A).addValue("B", E::B).addValue("C", E::C);

  CHECK(enumGlue["A"]->get<E>() == E::A);
  CHECK(enumGlue["B"]->get<E>() == E::B);
  CHECK(enumGlue["C"]->get<E>() == E::C);
}