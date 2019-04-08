#include <catch2/catch.hpp>
#include <stdexcept>

#include <lars/glue.h>
#include <lars/lua_glue.h>

TEST_CASE("LuaGlue"){
  
  auto shared_extension = std::make_shared<lars::Extension>();
  lars::Extension &extension = *shared_extension;
  
  // call functions and get return values
  extension.add_function("meaning_of_life",[](){ return 42; });
  extension.add_function("add",[](float a,int b){ return a+b; });
  
  SECTION("test extension functions"){
    REQUIRE(extension.get_function("add").return_type() == lars::get_type_index<decltype(float(1)+int(2))>());
    REQUIRE(extension.get_function("add").argument_type(0) == lars::get_type_index<float>());
    REQUIRE(extension.get_function("add").argument_type(1) == lars::get_type_index<int>());
    REQUIRE(extension.get_function("add")(lars::make_any<double>(2),lars::make_any<int>(3)).get_numeric<int>() == 5);
  }
    
  // callbacks
  extension.add_function("call_callback",[](const lars::AnyFunction &f){ f(42); });
  
  // store callback
  lars::AnyFunction stored_function;
  extension.add_function("store_callback",[&](const lars::AnyFunction &f){ stored_function = f; });
  extension.add_function("call_stored_callback",[&](lars::AnyArguments &args){ return stored_function.call(args); });
  
  // create lua context
  lars::LuaState lua;
  lua.openLibs();
  
  lars::LuaGlue lua_glue = lua.getGlue();
  extension.connect(lua_glue);
  
  SECTION("test lua execution"){
    REQUIRE_NOTHROW(lua.run("function assert(x, msg) if not x then error(msg or 'assertion failed') end end") );
    REQUIRE_NOTHROW(lua.run("assert(1 == 1)") );
    REQUIRE_THROWS(lua.run("assert(1 == 0)") );
  }
  
  SECTION("test calling extension"){
    REQUIRE_NOTHROW(lua.run("local value = meaning_of_life(); assert(value == 42)"));
    REQUIRE_NOTHROW(lua.run("assert(add(2,3) == 5);"));
  }
  
  SECTION("test callbacks"){
    REQUIRE_NOTHROW(lua.run("store_callback(function(n) return n+1; end);"));
    REQUIRE_NOTHROW(lua.run("assert(call_stored_callback(4) == 5)"));
  }
  
  // classes and auto update
  static unsigned my_class_instances = 0;
  struct MyClass{
    std::string data;
    MyClass(const MyClass &other):data(other.data){ my_class_instances++; }
    MyClass(){ my_class_instances++; }
    ~MyClass(){ my_class_instances--; }
  };
  
  SECTION("test custom class"){
    extension.add_function("create_my_class",[](){ return MyClass(); });
    extension.add_function("get_my_class_data",[](const MyClass &my_class){ return my_class.data; });
    extension.add_function("set_my_class_data",[](MyClass &my_class,const std::string &str){ my_class.data = str; });
    
    REQUIRE_NOTHROW(lua.run("my_class = create_my_class()"));
    lua.collectGarbage();
    REQUIRE(my_class_instances == 1);
    
    REQUIRE_NOTHROW(lua.run("set_my_class_data(my_class,'data')"));
    REQUIRE_NOTHROW(lua.run("assert(get_my_class_data(my_class) == 'data')"));
    REQUIRE_NOTHROW(lua.run("my_class = nil"));
    lua.collectGarbage();
    
    // destructor called
    REQUIRE(my_class_instances == 0);
    
    // capture and free
    REQUIRE_NOTHROW(lua.run("function test(x) store_callback(function() set_my_class_data(x,'data') end); end test(create_my_class())"));
    lua.collectGarbage();
    REQUIRE(my_class_instances == 1);
    REQUIRE_NOTHROW(lua.run("store_callback(function()end)"));
    lua.collectGarbage();
    REQUIRE(my_class_instances == 0);
  }
    
  SECTION("inner extensions"){
    auto inner_extension = std::make_shared<lars::Extension>();
    inner_extension->add_function("before", []()->std::string{ return "inner=>before"; });
    extension.add_extension("inner", inner_extension);
    REQUIRE_NOTHROW(lua.run("assert(inner.before() == 'inner=>before')"));
  
    SECTION("inner update"){
      inner_extension->add_function("after", []()->std::string{ return "inner=>after"; });
      REQUIRE_NOTHROW(lua.run("assert(inner.after() == 'inner=>after')"));
    }
  }
  
  SECTION("extensions extension"){
    auto extensions_extension = std::make_shared<lars::Extension>();
    extension.add_extension("extensions", extensions_extension);
    extensions_extension->add_function("create_extension", [&](std::string name){
      auto new_extension = std::make_shared<lars::Extension>();
      extension.add_extension(name, new_extension);
      return new_extension;
    });
    
    extensions_extension->add_function("add_function",[](std::shared_ptr<lars::Extension> extension,std::string name,lars::AnyFunction f){ extension->add_function(name, f); });
    REQUIRE_NOTHROW(lua.run("extension = extensions.create_extension('lua_extension')"));

    SECTION("call lua function without return value"){
      REQUIRE_NOTHROW(lua.run("extensions.add_function(extension,'f',function(x,y) tmp = 'called f(' .. tostring(x) .. ',' .. tostring(y) .. ')' end)"));
      REQUIRE_NOTHROW(lua.run("lua_extension.f(1,'x'); assert(tmp == 'called f(1.0,x)');"));
      auto f = extension.get_extension("lua_extension")->get_function("f");
      REQUIRE(f);
      REQUIRE_NOTHROW(f(std::string("x"),42));
      REQUIRE_NOTHROW(lua.run("assert(tmp == 'called f(x,42)')"));
      REQUIRE_NOTHROW(f(1));
      REQUIRE_NOTHROW(lua.run("assert(tmp == 'called f(1,nil)')"));
      REQUIRE_NOTHROW(lua.run("assert(extension)"));
    }
    
    SECTION("call lua function with return value"){
      REQUIRE_NOTHROW(lua.run("assert(extension)"));
      REQUIRE_NOTHROW(lua.run("extensions.add_function(extension,'g',function(x,y) return 'called g(' .. tostring(x) .. ',' .. tostring(y) .. ')' end)"));
      REQUIRE_NOTHROW(lua.run("assert(lua_extension.g('x','1') == 'called g(x,1)')"));
      lars::Any res;
      REQUIRE_NOTHROW(res = extension.get_extension("lua_extension")->get_function("g")(2,std::string("x")));
      REQUIRE(res);
      REQUIRE(res.get<std::string>() == "called g(2,x)");
      REQUIRE_NOTHROW(lua.run("extensions.add_function(extension,'add',function(a,b) return a+b end)"));
      REQUIRE_NOTHROW(res = extension.get_extension("lua_extension")->get_function("add")(2,40));
      REQUIRE(res);
      REQUIRE(res.get_numeric<int>() == 42);
    }
    
    SECTION("call and return lua object"){
      REQUIRE_NOTHROW(lua.run("extensions.add_function(extension,'h',function(x) return x.key2; end)"));
      REQUIRE_NOTHROW(lua.run("map = { key1 = 'value1', key2 = 'value2' };"));
      REQUIRE_NOTHROW(lua.run("assert(lua_extension.h(map) == 'value2');"));
      
      REQUIRE_NOTHROW(lua.run("extensions.add_function(extension,'create_table',function() return { } end)"));
      REQUIRE_NOTHROW(lua.run("extensions.add_function(extension,'set_key_value',function(t,k,v) t[k] = v end)"));
      REQUIRE_NOTHROW(lua.run("extensions.add_function(extension,'table_to_string',function(x) local res = '' for k,v in pairs(x) do res = res .. tostring(k) .. '=' .. tostring(v) .. ',' end return res end)"));
    
      auto create_table = extension.get_extension("lua_extension")->get_function("create_table");
      auto set_key_value = extension.get_extension("lua_extension")->get_function("set_key_value");
      auto table_to_string = extension.get_extension("lua_extension")->get_function("table_to_string");
      
      auto table = create_table();
      set_key_value(table,"a",1);
      set_key_value(table,"b","2");
      set_key_value(table,"c",create_table());
    }
  
  }

  SECTION("derived classes"){
    struct A:lars::Visitable<A>{ int value; A(){} };
    struct B:lars::DVisitable<B,A>{ B(){ value = 1; } };
    struct C:lars::DVisitable<C,A>{ C(){ value = 2; } };
    
    extension.add_function("create_B", [](){ return B(); });
    extension.add_function("create_C", [](){ return C(); });
    extension.add_function("get_value", [](A & a){ return a.value; });
    extension.add_function("get_Cvalue", [](C & a){ return a.value; });
    REQUIRE_NOTHROW(lua.run("assert(get_value(create_B()) == 1)"));
    REQUIRE_NOTHROW(lua.run("assert(get_value(create_C()) == 2)"));
    REQUIRE_NOTHROW(lua.run("assert(get_Cvalue(create_C()) == 2)"));
    REQUIRE_THROWS(lua.run("assert(get_Cvalue(create_B()) == 2)"));
    REQUIRE_THROWS(lua.run("get_value(create_my_class())"));
    REQUIRE_THROWS(lua.run("get_value('test')"));
  }
  
  SECTION("class extensions"){
    auto my_class_extension = std::make_shared<lars::Extension>();
    my_class_extension->set_class<MyClass>();
    
    my_class_extension->add_function("new", [&](lars::AnyArguments &args){
      if(args.size() == 0) return MyClass();
      if(args.size() == 1){ MyClass c; c.data = args[0].get<std::string>(); return c; };
      throw std::runtime_error("create MyClass with zero or one arguments");
    });
    my_class_extension->add_function("set_data", [](MyClass &o,const std::string &str){ o.data = str; });
    my_class_extension->add_function("get_data", [](MyClass &o){ return o.data; });
    my_class_extension->add_function("shared_get_data", [](const std::shared_ptr<MyClass> &o){ return o->data; });
    extension.add_extension("MyClass", my_class_extension);
    
    REQUIRE_NOTHROW(lua.run("local a = MyClass.new(); a:set_data('a'); assert(a:get_data() == 'a');"));
    REQUIRE_NOTHROW(lua.run("local b = MyClass.new('b'); assert(b:get_data() == 'b');"));
  
    SECTION("Derived classes"){
      class MyDerivedClass:public lars::BVisitable<MyDerivedClass,MyClass>{
      public:
        std::string data_2(){ return data + "2"; }
      };
      
      auto my_derived_class_extension = std::make_shared<lars::Extension>();
      my_derived_class_extension->set_class<MyDerivedClass>();
      my_derived_class_extension->set_base_class<MyClass>();
      my_derived_class_extension->add_function("new", [](){ return MyDerivedClass(); });
      my_derived_class_extension->add_function("shared_new", [](){ return std::make_shared<MyDerivedClass>(); });
      my_derived_class_extension->add_function("data_2", [](MyDerivedClass &c){ return c.data_2(); });
      my_derived_class_extension->add_function("shared_data_2", [](const std::shared_ptr<MyDerivedClass> &c){ return c->data_2(); });
      extension.add_extension("MyDerivedClass", my_derived_class_extension);
      
      REQUIRE_NOTHROW(lua.run("a = MyDerivedClass.new()"));
      REQUIRE_NOTHROW(lua.run("assert(a:data_2() == '2')"));
      REQUIRE_NOTHROW(lua.run("a:set_data('hello')"));
      REQUIRE_NOTHROW(lua.run("assert(a:data_2() == 'hello2')"));
      REQUIRE_NOTHROW(lua.run("assert(a:shared_data_2() == 'hello2')"));
      
      REQUIRE_NOTHROW(lua.run("a = MyDerivedClass.shared_new()"));
      REQUIRE_NOTHROW(lua.run("assert(a:data_2() == '2')"));
      REQUIRE_NOTHROW(lua.run("a:set_data('hello')"));
      REQUIRE_NOTHROW(lua.run("assert(a:shared_data_2() == 'hello2')"));
      //  REQUIRE_NOTHROW(lua.run("assert(a:shared_get_data() == 'hello')"));
    }
  }

  /**
   * Test Disabled:
   * yielding from callbacks won't work, as they will always be run from the main lua thread
   */

  /*
  SECTION("Coroutines"){
    REQUIRE_NOTHROW(lua.run("wait = coroutine.create(function()"
                       "  print('a1'); store_callback(function(n) print('b'); return coroutine.yield(n+3); end)"
                       "  print('a2'); call_stored_callback(4);"
                       "end)" ));
    
    REQUIRE_NOTHROW(lua.run("local status, value = coroutine.resume(wait); assert(status and value == 7, value);"));
    
    REQUIRE_THROWS(extension.get_function("call_stored_callback")(1));
  }
  */
  
}
