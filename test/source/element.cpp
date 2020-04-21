#include <doctest/doctest.h>
#include <glue/element.h>

#include <set>

using namespace glue;

TEST_CASE("Element") {
  Element element;

  SUBCASE("empty element") {
    REQUIRE(!element);
    REQUIRE(element.asMap() == nullptr);
  }

  SUBCASE("set and get value") {
    REQUIRE_NOTHROW(element = 42);
    REQUIRE(element);
    REQUIRE(element.get<int>() == 42);
    REQUIRE_THROWS(element.get<std::string>());
    REQUIRE(element.tryGet<std::string>() == nullptr);
    REQUIRE(element.asMap() == nullptr);
  }

  SUBCASE("set and get keys") {
    REQUIRE_NOTHROW(element["a"] = 4);
    REQUIRE_NOTHROW(element["b"] = 2);
    REQUIRE_NOTHROW(element["c"]["d"]["e"] = 3);
    REQUIRE(element.asMap() != nullptr);
    REQUIRE(element["a"]);
    REQUIRE(!element["d"]);
    REQUIRE(element);
    REQUIRE(element["a"]);
    REQUIRE(element["b"]);
    REQUIRE(element["c"]["d"]);
    REQUIRE(element["a"].get<int>() == 4);
    REQUIRE(element["b"].get<int>() == 2);
    REQUIRE(element["c"]["d"]["e"].get<float>() == 3);
  }

  SUBCASE("set and call function") {
    REQUIRE_NOTHROW(element = [](int x) { return x * 2; });
    REQUIRE(element(21).get<int>() == 42);
  }

  SUBCASE("set and call mapped functions") {
    REQUIRE_NOTHROW(element["a"] = [](int x) { return x + 2; });
    REQUIRE_NOTHROW(element["b"] = [](int x) { return x * 2; });
    REQUIRE(element["a"](40).get<int>() == 42);
    REQUIRE(element["b"](21).get<int>() == 42);
  }

  SUBCASE("events") {
    unsigned changes = 0;
    std::set<std::string> changedItems;
    element.setToMap().onValueChanged.connect([&](auto &key, auto &) {
      changes++;
      changedItems.insert(key);
    });
    element["a"] = 1;
    element["b"] = 2;
    element["a"] = 3;
    REQUIRE(changedItems.size() == 2);
    REQUIRE(changes == 3);
  }

  SUBCASE("extends") {
    element["a"] = 5;
    Element inner;
    setExtends(inner, element);
    REQUIRE(inner["a"].get<int>() == 5);
    inner["a"] = 42;
    REQUIRE(inner["a"].get<int>() == 42);
    REQUIRE(element["a"].get<int>() == 5);
    inner["a"] = Any();
    REQUIRE(inner["a"].get<int>() == 5);
  }
}

TEST_CASE("element for class") {
  struct Base {
    int value;
    Base() = default;
    Base(Base &&) = default;
    Base(const Base &other) = delete;
  };

  struct A : public Base {
    A(int v) { value = v; }
    int member(int v) { return value + v; };
  };

  Element base;
  setClass<Base>(base);
  REQUIRE(getClass(base) != nullptr);
  REQUIRE(*getClass(base) == revisited::getTypeIndex<Base>());
  base["value"] = [](Base &b) { return b.value; };

  Element a;
  setClass<A>(a);
  REQUIRE(*getClass(a) == revisited::getTypeIndex<A>());
  setExtends(a, base);
  a["create"] = [](int v) { return Any::withBases<A, Base>(v); };

  auto av = a["create"](42);
  REQUIRE(a["value"](av).get<int>() == 42);
}

TEST_CASE("copy assignments") {
  Element a;

  SUBCASE("copy element") { Element b = a; }

  SUBCASE("copy assignment") {
    Element b;
    b = a;
  }

  SUBCASE("copy element entry") {
    a["x"] = 42;
    Element b;
    b["x"] = a["x"];
  }
}