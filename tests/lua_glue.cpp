#include <catch2/catch.hpp>
#include <stdexcept>

#include <glue/lua.h>

TEST_CASE("LuaState","[lua]"){
  using namespace glue;

  LuaState lua;
  lua.openStandardLibs();
  
  SECTION("get nothing"){
    REQUIRE(!lua.get("nil"));
    REQUIRE_THROWS(lua.get<int>("nil"));
  }
  
  SECTION("get number"){
    REQUIRE_NOTHROW(lua.run("x = 40"));
    REQUIRE(lua.get<char>("x+2") == 42);
    REQUIRE(lua.get<int>("x+2") == 42);
    REQUIRE(lua.get<unsigned>("x+2") == 42);
    REQUIRE(lua.get<long>("x+2") == 42);
    REQUIRE(lua.get<size_t>("x+2") == 42);
    REQUIRE(lua.get<float>("x+2") == 42);
    REQUIRE(lua.get<double>("(x/16-0.4)*2") == Approx(4.2));
  }
  
  SECTION("get boolean"){
    REQUIRE(lua.get<bool>("true") == true);
    REQUIRE(lua.get<bool>("not true") == false);
  }
  
  SECTION("get string"){
    REQUIRE(lua.get<std::string>("'Hello Lua!'") == "Hello Lua!");
  }
  
  SECTION("get and call function"){
    REQUIRE(lua.get<lars::AnyFunction>("function(x) return x+1 end")(41).get<int>() == 42);
  }

  SECTION("set value"){
    lua.set("y", 21);
    REQUIRE(lua.get<int>("2*y") == 42);
  }
  
  SECTION("set lua value"){
    auto x = lua.get("42");
    lua.set("y",x);
    REQUIRE(lua.get<int>("y") == 42);
  }
  
  SECTION("set and call function"){
    lua.set("f", lars::AnyFunction([](int x){ return x+3; }));
    REQUIRE(lua.get<int>("f(39)") == 42);
  }
  
  SECTION("get element map"){
    lua.run("x = { a = 1, b = 'b', c = { d = 42 } }");
    auto ptr = lua.get<std::shared_ptr<Map>>("x");
    auto &map = *ptr;
    
    SECTION("get values"){
      REQUIRE(map["a"].get<int>() == 1);
      REQUIRE(map["b"].get<std::string>() == "b");
      REQUIRE(map["c"]["d"].get<int>() == 42);
    }
    
    SECTION("set values"){
      REQUIRE_NOTHROW(map["a"] = "42");
      REQUIRE(map["a"].get<std::string>() == "42");
      REQUIRE(lua.get<std::string>("x.a") == "42");
    }
    
    SECTION("update table"){
      REQUIRE_NOTHROW(map["a"]["b"]["c"] = 42);
      REQUIRE(map["a"]["b"]["c"].get<int>() == 42);
      REQUIRE(lua.get<int>("x.a.b.c") == 42);
      map["a"]["d"] = 3;
      REQUIRE(lua.get<int>("x.a.d") == 3);
    }
  }
  
  SECTION("global table"){
    lua.run("x = 42");
    REQUIRE(lua["x"].get<int>() == 42);
    REQUIRE(lua["tostring"](lua["x"]).get<std::string>() == "42.0");
  }
  
}


