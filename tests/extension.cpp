#include <catch2/catch.hpp>
#include <stdexcept>

#include <glue/extension.h>

using namespace glue;

TEST_CASE("Extension", "[extension]"){
  NewExtension extension;
  
  SECTION("Values"){
    extension["v"] = 42;

    REQUIRE(extension.getMember("v") != nullptr);
    REQUIRE(extension.getMember("v")->get<int>() == 42);
    REQUIRE(std::as_const(extension).getMember("v")->get<int>() == 42);
    REQUIRE(extension["v"].get<int>() == 42);
    REQUIRE(extension["v"].get<float>() == 42);
    REQUIRE_THROWS(extension["v"].get<std::string>());
    REQUIRE_NOTHROW(static_cast<lars::Any &>(extension["v"]));
    REQUIRE_NOTHROW(static_cast<const lars::Any &>(extension["v"]));
    REQUIRE_THROWS(static_cast<const NewExtension &>(extension["v"]));
    REQUIRE_THROWS(static_cast<const lars::AnyFunction &>(extension["v"]));
    
    extension["s"] = "Hello Glue!";
    REQUIRE(extension["s"].get<std::string>() == "Hello Glue!");
  }
  
  SECTION("Functions"){
    SECTION("Undefined member"){
      REQUIRE_NOTHROW(extension.getMember("add") == nullptr);
      REQUIRE_NOTHROW(std::as_const(extension).getMember("add") == nullptr);
      REQUIRE_THROWS_AS(std::as_const(extension)["add"], NewExtension::MemberNotFoundException);
      REQUIRE_THROWS_WITH(std::as_const(extension)["add"], Catch::Matchers::Contains("add"));
    }
    
    SECTION("return value"){
      REQUIRE_NOTHROW(extension["get"] = [](){ return 42; });
      REQUIRE(extension["get"]().get<int>() == 42);
    }
    
    SECTION("return any"){
      extension["getAny"] = [](){ return lars::makeAny<int>(42); };
      REQUIRE(extension["getAny"]().get<int>() == 42);
    }
    
    SECTION("function casting"){
      extension["f"] = [](){ };
      REQUIRE_NOTHROW(extension["f"]());
      REQUIRE_THROWS_AS(extension["f"].asExtension(), NewExtension::Member::InvalidCastException);
    }
    
    SECTION("Arguments"){
      extension["add"] = [](int a, int b){ return a + b; };
      lars::AnyFunction f = extension["add"];
      REQUIRE_THROWS_AS(f(), lars::AnyFunctionInvalidArgumentCountException);
      REQUIRE_THROWS_AS(f(2), lars::AnyFunctionInvalidArgumentCountException);
      REQUIRE(f(2,3).get<int>() == 5);
      REQUIRE_THROWS_AS(f(2,3,4), lars::AnyFunctionInvalidArgumentCountException);
    }
    
    SECTION("Any Arguments"){
      extension["addAny"] = [](const lars::Any &a, const lars::Any &b){
        return a.get<double>() + b.get<double>();
      };
      REQUIRE(extension["addAny"](2,3.5).get<double>() == Approx(5.5));
    }
  }
 
  SECTION("Inner extension"){
    {
    NewExtension inner;
    inner["f"] = [](){ return 5; };
    extension["inner"] = inner;
    }
    REQUIRE_NOTHROW(static_cast<const NewExtension &>(extension["inner"]));
    REQUIRE(extension["inner"]["f"]().get<int>() == 5);
  }
  
  SECTION("extends"){
    NewExtension inner;
    inner["v"] = 5;
    setExtends(extension, inner);
    REQUIRE(std::as_const(extension)["v"].get<int>() == 5);
  }
  
}

TEST_CASE("Class extension", "[extension]"){
  struct Base {
    int value;
    Base() = default;
    Base(Base &&) = default;
    Base(const Base &other) = delete;
  };
  
  struct A: public Base {
    A(int v){ value = v; }
    int member(int v){ return value + v; };
  };
  
  NewExtension baseExtension;
  setClass<Base>(baseExtension);
  baseExtension["value"] = [](Base &b){ return b.value; };
  
  NewExtension aExtension;
  setClass<A>(aExtension);
  aExtension["create"] = [](int v){ return Any::withBases<A,Base>(v); };
  setExtends(aExtension, baseExtension);
  
  auto a = aExtension["create"](42);
  REQUIRE(std::as_const(aExtension)["value"](a).get<int>() == 42);
}
