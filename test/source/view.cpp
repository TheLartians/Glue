#include <doctest/doctest.h>
#include <glue/view.h>

using namespace glue;

TEST_CASE("Value") {
  auto inner = glue::createAnyMap();
  inner["a"] = [](int x) { return 42 + x; };
  auto root = glue::createAnyMap();
  root["inner"] = inner;
  root["value"] = 42;

  View view(*root);
  CHECK(view["value"]->get<int>() == 42);
  CHECK(!*view["xxx"]);
  CHECK_THROWS(view["inner"]());
  CHECK_NOTHROW(view["inner"]);
  CHECK_NOTHROW(view["inner"]["a"]);
  CHECK(view["inner"]["a"](6)->get<int>() == 48);
}
