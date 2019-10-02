#include <catch2/catch.hpp>
#include <stdexcept>

#include <cmath>
#include <iostream>
#include <glue/lua.h>

TEST_CASE("LuaState","[lua]"){
  using namespace glue;

  LuaState lua;
  lua.openStandardLibs();
  
  SECTION("get nothing"){
    CHECK(!lua.get("nil"));
    CHECK_THROWS(lua.get<int>("nil"));
  }
  
  SECTION("get number"){
    CHECK_NOTHROW(lua.run("x = 40"));
    CHECK(lua.get<char>("x+2") == 42);
    CHECK(lua.get<int>("x+2") == 42);
    CHECK(lua.get<unsigned>("x+2") == 42);
    CHECK(lua.get<long>("x+2") == 42);
    CHECK(lua.get<size_t>("x+2") == 42);
    CHECK(lua.get<float>("x+2") == 42);
    CHECK(lua.get<double>("(x/16-0.4)*2") == Approx(4.2));
  }

  SECTION("numeric conversions") {
    lua.run("function identity(x) return x end");
    
    CHECK(lua["identity"](float(42.24)).get<float>() == Approx(42.24f));
    CHECK(lua["identity"]((long double)(42.24)).get<long double>() == Approx(42.24));

    CHECK(lua["identity"]((short)(42)).get<short>() == 42);
    CHECK(lua["identity"]((int)(42)).get<int>() == 42);
    CHECK(lua["identity"]((long)(42)).get<long>() == 42);
    CHECK(lua["identity"]((long long)(42)).get<long long>() == 42);

    CHECK(lua["identity"]((unsigned short)(42)).get<unsigned short>() == 42);
    CHECK(lua["identity"]((unsigned int)(42)).get<unsigned int>() == 42);
    CHECK(lua["identity"]((unsigned long)(42)).get<unsigned long>() == 42);
    CHECK(lua["identity"]((unsigned long long)(42)).get<unsigned long long>() == 42);
  }

  SECTION("set number"){
    CHECK(lua["tostring"](int(42)).get<std::string>() == "42");
    CHECK(lua["tostring"](unsigned(42)).get<std::string>() == "42");
    CHECK(lua["tostring"](size_t(42)).get<std::string>() == "42");
    CHECK(lua["tostring"]('x').get<std::string>() == "x");
    CHECK(lua["tostring"](42.0).get<std::string>() == "42.0");
  }
  
  SECTION("get boolean"){
    CHECK(lua.get<bool>("true") == true);
    CHECK(lua.get<bool>("not true") == false);
  }
  
  SECTION("get string"){
    CHECK(lua.get<std::string>("'Hello Lua!'") == "Hello Lua!");
  }
  
  SECTION("get and call function"){
    CHECK(lua.get<lars::AnyFunction>("function(x) return x+1 end")(41).get<int>() == 42);
  }

  SECTION("set value"){
    lua.set("y", 21);
    CHECK(lua.get<int>("2*y") == 42);
  }
  
  SECTION("set lua value"){
    auto x = lua.get("42");
    lua.set("y",x);
    CHECK(lua.get<int>("y") == 42);
  }
  
  SECTION("set and call function"){
    lua.set("f", lars::AnyFunction([](int x){ return x+3; }));
    CHECK(lua.get<int>("f(39)") == 42);
  }

  SECTION("get element map"){
    lua.run("x = { a = 1, b = 'b', c = { d = 42 } }");
    auto ptr = lua.get<std::shared_ptr<Map>>("x");
    auto &map = *ptr;
    
    SECTION("get values"){
      CHECK(map["a"].get<int>() == 1);
      CHECK(map["b"].get<std::string>() == "b");
      CHECK(map["c"]["d"].get<int>() == 42);
    }
    
    SECTION("set values"){
      CHECK_NOTHROW(map["a"] = "42");
      CHECK(map["a"].get<std::string>() == "42");
      CHECK(lua.get<std::string>("x.a") == "42");
    }
    
    SECTION("update table"){
      CHECK_NOTHROW(map["a"]["b"]["c"] = 42);
      CHECK(map["a"]["b"]["c"].get<int>() == 42);
      CHECK(lua.get<int>("x.a.b.c") == 42);
      map["a"]["d"] = 3;
      CHECK(lua.get<int>("x.a.d") == 3);
    }
    
    SECTION("inspect table"){
      CHECK(map.keys().size() == 3);
      CHECK_NOTHROW(map["e"]["f"] = 1);
      CHECK_NOTHROW(map["e"]["f"] = 1);
      CHECK(map["e"].asMap());
      CHECK(map["e"].asMap()->keys().size() == 2);
    }
  }
  
  SECTION("global table"){
    lua.run("x = 42");
    CHECK(lua["x"].get<int>() == 42);
    CHECK(lua["tostring"](lua["x"]).get<std::string>() == "42.0");
  }
  
  SECTION("extended element"){
    Element base;
    base["a"] = 1;
    base["b"] = 2;
    Element extends;
    setExtends(extends, base);
    lua["e"] = extends;
    CHECK(lua.get<int>("e.a") == 1);
    CHECK(lua.get<int>("e.b") == 2);
    extends["a"] = 3;
    CHECK(lua.get<int>("e.a") == 3);
    CHECK(lua.get<int>("e.b") == 2);
    extends["a"] = Any();
    CHECK(lua.get<int>("e.a") == 1);
    CHECK(lua.get<int>("e.b") == 2);
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
      CHECK_THAT(lua.get<std::string>("tostring(a)"), Catch::Matchers::Contains("A"));
      CHECK(lua.get<int>("Base.value(a)") == 42);
      CHECK(lua.get<int>("A.value(a)") == 42);
      CHECK(lua.get<int>("a:value()") == 42);
    }

    SECTION("shared pointer value"){
      lua.run("b = A.create_shared(420)");
      CHECK_THAT(lua.get<std::string>("tostring(b)"), Catch::Matchers::Contains("Base"));
      CHECK(lua.get<int>("Base.value(b)") == 420);
      CHECK(lua.get<int>("b:value()") == 420);
    }

    SECTION("default compare"){
      lua.run("a = A.create(42)");
      lua.run("b = A.create(42)");
      CHECK(lua.get<bool>("a == a"));
      CHECK(!lua.get<bool>("a == b"));
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
    CHECK(count == 2);
    lua["b"] = Any();
    lua.collectGarbage();
    CHECK(count == 1);
    lua["a"] = Any();
    lua.collectGarbage();
    CHECK(count == 0);
  }
  
  SECTION("operators"){
    struct A{ int value = 1; };
    Element a;
    setClass<A>(a);
    a[keys::operators::eq] = [](A&a, A&b){ return a.value == b.value; };
    a[keys::operators::add] = [](A&a, A&b){ return A{a.value + b.value}; };
    a[keys::operators::mul] = [](A&a, A&b){ return A{a.value * b.value}; };
    a[keys::operators::sub] = [](A&a, A&b){ return A{a.value - b.value}; };
    a[keys::operators::div] = [](A&a, A&b){ return A{a.value / b.value}; };
    a[keys::operators::lt ] = [](A&a, A&b){ return a.value < b.value; };
    a[keys::operators::gt ] = [](A&a, A&b){ return a.value > b.value; };
    a[keys::operators::le ] = [](A&a, A&b){ return a.value <= b.value; };
    a[keys::operators::ge ] = [](A&a, A&b){ return a.value >= b.value; };
    a[keys::operators::unm] = [](A&a){ return -a.value; };
    a[keys::operators::pow] = [](A&a, A&b){ return pow(a.value,b.value); };
    a[keys::operators::mod] = [](A&a, A&b){ return a.value % b.value; };
    a[keys::operators::idiv] = [](A&a, A&b){ return a.value / b.value; };
    a[keys::operators::tostring] = [](A&a){ return "A:" + std::to_string(a.value); };

    lua["A"] = a;
    lua["a"] = A();
    CHECK(lua.get<bool>("a == a"));
    CHECK(lua.get<bool>("a == a+a-a"));
    CHECK(!lua.get<bool>("a == a+a"));
    CHECK(lua.get<bool>("a ~= a+a"));
    CHECK(lua.get<A&>("(a + a) * (a / a + a) - a").value == 3);
    CHECK(lua.get<bool>("a + a > a"));
    CHECK(!lua.get<bool>("a + a < a"));
    CHECK(lua.get<bool>("a <= a"));
    CHECK(lua.get<bool>("a >= a"));
    CHECK(lua.get<int>("-a") == -1);
    CHECK(lua.get<int>("(a+a)^(a+a)") == 4);
    CHECK(lua.get<int>("a % (a+a)") == 1);
    CHECK(lua.get<int>("a // a") == 1);
    CHECK(lua.get<std::string>("tostring(a)") == "A:1");
    CHECK(lua.get<A &>("a").value == 1);
  }

  SECTION("errors"){
    SECTION("lua errors"){
      CHECK_THROWS_AS(lua.run("error('Hello Lua!')"), LuaState::Error);
      CHECK_THROWS_WITH(lua.run("error('Hello Lua!')"), Catch::Matchers::Contains("Hello Lua!"));
    }
  }

  SECTION("call function with any arguments"){
    lua["f"] = [](const lars::AnyArguments &args){
      return args.size();
    };
    CHECK(lua.get<int>("f(1,2,3,4)") == 4);
  }

  SECTION("callbacks"){
    SECTION("scalar values"){
      lua["count"] = [](lars::AnyFunction f){
        for (int i = 0; i<10; ++i) f(i);
      };
      CHECK_NOTHROW(lua.run("res = 0; count(function(i) res = res + i; end);"));
      CHECK_NOTHROW(lua.get<int>("res") == 45);
    }
    SECTION("class values"){
      struct A{ int value; };
      Element AGLue;
      setClass<A>(AGLue);
      AGLue["value"] = [](const A &a){ return a.value; };
      lua["classes"]["A"] = AGLue;

      SECTION("checks"){
        lua["a"] = A{42};
        CHECK(lua.get<int>("a:value()") == 42);
        A a{42};
        lua["a"] = std::reference_wrapper(a);
        CHECK(lua.get<int>("a:value()") == 42);
        lua["a"] = std::reference_wrapper(std::as_const(a));
        CHECK(lua.get<int>("a:value()") == 42);
      }

      lua["count"] = [](lars::AnyFunction f){ 
        for (int i = 0; i<10; ++i){
          auto a = A{i};
          f(a);
          f(std::as_const(a));
        };
      };
      CHECK_NOTHROW(lua.run("res = 0; count(function(i) res = res + i:value(); end);"));
      CHECK_NOTHROW(lua.get<int>("res") == 90);
    }
    
  }
  
  SECTION("convert objects from lua object and back"){
    lua["f"] = [](const Any &v)->AnyReference{ return v; };
    CHECK(lua.get<int>("f(42)") == 42);
    CHECK(lua.get<int>("f({a = 42}).a") == 42);
    CHECK((*lua.get<std::shared_ptr<glue::Map>>("f({a = 42})"))["a"].get<int>() == 42);
  }

  SECTION("run file"){
    std::string path = __FILE__;
    std::cout << "test: " << path << std::endl;
    path.erase(std::find(path.rbegin(), path.rend(), '/').base(), path.end());
    std::cout << "test: " << path << std::endl;
    path += "test.lua";
    CHECK(lua.runFile(path).get<int>() == -1);
  }

}

