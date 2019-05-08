#include <catch2/catch.hpp>
#include <stdexcept>

#include <cmath>

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
    
    SECTION("inspect table"){
      REQUIRE(map.keys().size() == 3);
      REQUIRE_NOTHROW(map["e"]["f"] = 1);
      REQUIRE_NOTHROW(map["e"]["f"] = 1);
      REQUIRE(map["e"].asMap());
      REQUIRE(map["e"].asMap()->keys().size() == 2);
    }
  }
  
  SECTION("global table"){
    lua.run("x = 42");
    REQUIRE(lua["x"].get<int>() == 42);
    REQUIRE(lua["tostring"](lua["x"]).get<std::string>() == "42.0");
  }
  
  SECTION("extended element"){
    Element base;
    base["a"] = 1;
    base["b"] = 2;
    Element extends;
    setExtends(extends, base);
    lua["e"] = extends;
    REQUIRE(lua.get<int>("e.a") == 1);
    REQUIRE(lua.get<int>("e.b") == 2);
    extends["a"] = 3;
    REQUIRE(lua.get<int>("e.a") == 3);
    REQUIRE(lua.get<int>("e.b") == 2);
    extends["a"] = Any();
    REQUIRE(lua.get<int>("e.a") == 1);
    REQUIRE(lua.get<int>("e.b") == 2);
  }
  
  SECTION("class element"){
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
    
    Element base;
    setClass<Base>(base);
    base["value"] = [](Base &b){ return b.value; };
    
    Element a;
    setClass<A>(a);
    setExtends(a, base);

    a["create"] = [](int v){ return Any::withBases<A,Base>(v); };
    a["create_shared"] = [](int v){ return std::shared_ptr<Base>(std::make_shared<A>(v)); };
    
    lua["Base"] = base;
    lua["A"] = a;
    
    SECTION("value and inheritance"){
      lua.run("a = A.create(42)");
      REQUIRE_THAT(lua.get<std::string>("tostring(a)"), Catch::Matchers::Contains("A"));
      REQUIRE(lua.get<int>("Base.value(a)") == 42);
      REQUIRE(lua.get<int>("A.value(a)") == 42);
      REQUIRE(lua.get<int>("a:value()") == 42);
    }

    SECTION("shared pointer value"){
      lua.run("b = A.create_shared(420)");
      REQUIRE_THAT(lua.get<std::string>("tostring(b)"), Catch::Matchers::Contains("Base"));
      REQUIRE(lua.get<int>("Base.value(b)") == 420);
      REQUIRE(lua.get<int>("b:value()") == 420);
    }

  }
  
  SECTION("memory"){
    unsigned count = 0;
    struct MemoryCounter {
      unsigned * count = nullptr;
      MemoryCounter(unsigned &c):count(&c){ (*count)++; }
      MemoryCounter(const MemoryCounter &) = delete;
      MemoryCounter(MemoryCounter &&other){ std::swap(count, other.count); }
      ~MemoryCounter(){ if(count) (*count)--; }
    };
    lua["a"] = MemoryCounter(count);
    lua["b"] = MemoryCounter(count);
    REQUIRE(count == 2);
    lua["b"] = Any();
    lua.collectGarbage();
    REQUIRE(count == 1);
    lua["a"] = Any();
    lua.collectGarbage();
    REQUIRE(count == 0);
  }
  
  SECTION("operators"){
    struct A{ int value = 1; };
    Element a;
    setClass<A>(a);
    a[operators::add] = [](A&a, A&b){ return A{a.value + b.value}; };
    a[operators::mul] = [](A&a, A&b){ return A{a.value * b.value}; };
    a[operators::sub] = [](A&a, A&b){ return A{a.value - b.value}; };
    a[operators::div] = [](A&a, A&b){ return A{a.value / b.value}; };
    a[operators::lt ] = [](A&a, A&b){ return a.value < b.value; };
    a[operators::gt ] = [](A&a, A&b){ return a.value > b.value; };
    a[operators::le ] = [](A&a, A&b){ return a.value <= b.value; };
    a[operators::ge ] = [](A&a, A&b){ return a.value >= b.value; };
    a[operators::unm] = [](A&a){ return -a.value; };
    a[operators::pow] = [](A&a, A&b){ return pow(a.value,b.value); };
    a[operators::mod] = [](A&a, A&b){ return a.value % b.value; };
    a[operators::idiv] = [](A&a, A&b){ return a.value / b.value; };

    lua["A"] = a;
    lua["a"] = A();
    REQUIRE(lua.get<A&>("(a + a) * (a / a + a) - a").value == 3);
    REQUIRE(lua.get<bool>("a + a > a"));
    REQUIRE(!lua.get<bool>("a + a < a"));
    REQUIRE(lua.get<bool>("a <= a"));
    REQUIRE(lua.get<bool>("a >= a"));
    REQUIRE(lua.get<int>("-a") == -1);
    REQUIRE(lua.get<int>("(a+a)^(a+a)") == 4);
    REQUIRE(lua.get<int>("a % (a+a)") == 1);
    REQUIRE(lua.get<int>("a // a") == 1);
  }

  SECTION("errors"){
    SECTION("lua errors"){
      REQUIRE_THROWS_AS(lua.run("error('Hello Lua!')"), LuaState::Error);
      REQUIRE_THROWS_WITH(lua.run("error('Hello Lua!')"), Catch::Matchers::Contains("Hello Lua!"));
    }
  }

  SECTION("call function with any arguments"){
    lua["f"] = [](const lars::AnyArguments &args){
      return args.size();
    };
    REQUIRE(lua.get<int>("f(1,2,3,4)") == 4);
  }

  SECTION("callbacks"){
    SECTION("scalar values"){
      lua["count"] = [](lars::AnyFunction f){
        for (int i = 0; i<10; ++i) f(i);
      };
      REQUIRE_NOTHROW(lua.run("res = 0; count(function(i) res = res + i; end);"));
      REQUIRE_NOTHROW(lua.get<int>("res") == 45);
    }
    SECTION("class values"){
      struct A{ int value; };
      Element AGLue;
      setClass<A>(AGLue);
      AGLue["value"] = [](const A &a){ return a.value; };
      lua["classes"]["A"] = AGLue;

      SECTION("checks"){
        lua["a"] = A{42};
        REQUIRE(lua.get<int>("a:value()") == 42);
        A a{42};
        lua["a"] = std::reference_wrapper(a);
        REQUIRE(lua.get<int>("a:value()") == 42);
        lua["a"] = std::reference_wrapper(std::as_const(a));
        REQUIRE(lua.get<int>("a:value()") == 42);
      }

      lua["count"] = [](lars::AnyFunction f){ 
        for (int i = 0; i<10; ++i){
          auto a = A{i};
          f(a);
          f(std::as_const(a));
        };
      };
      REQUIRE_NOTHROW(lua.run("res = 0; count(function(i) res = res + i:value(); end);"));
      REQUIRE_NOTHROW(lua.get<int>("res") == 90);
    }
  }

}

