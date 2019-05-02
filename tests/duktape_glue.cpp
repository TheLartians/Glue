#include <catch2/catch.hpp>
#include <stdexcept>

#include <glue/duktape.h>

#if false


TEST_CASE("DuktapeGlue"){
  using namespace glue;
  
  Extension extension;
  
  // call functions and get return values
  extension.add_function("meaning_of_life",[](){ return 42; });
  extension.add_function("add",[](double a,int b){ return a+b; });
  
  REQUIRE(extension.get_function("add").returnType() == lars::getTypeIndex<decltype(double(1)+int(2))>());
  REQUIRE(extension.get_function("add").argumentType(0) == lars::getTypeIndex<double>());
  REQUIRE(extension.get_function("add").argumentType(1) == lars::getTypeIndex<int>());
  REQUIRE(extension.get_function("add")(lars::makeAny<double>(2),lars::makeAny<int>(3)).get<int>() == 5);
  
  // callbacks
  extension.add_function("call_callback",[](const lars::AnyFunction &f){ f(42); });
  
  // store callback
  auto get_stored_function = []()->lars::AnyFunction &{ static lars::AnyFunction f; return f; };
  extension.add_function("store_callback",[&](const lars::AnyFunction &f){ get_stored_function() = f; });
  extension.add_function("call_stored_callback",[&](){ return get_stored_function()(); });
  
  // create duktape context
  DuktapeContext context;
  extension.connect(context.get_glue());
  
  // assertion and sanity check
  REQUIRE_NOTHROW(context.run("function assert(x){ if(x) return; else throw 'assertion failed'; }"));
  REQUIRE_NOTHROW(context.run("assert(1==1)"));
  REQUIRE_THROWS(context.run("assert(1==0)"));
  
  // call functions
  REQUIRE_NOTHROW(context.run("var value = meaning_of_life(); assert(value == 42)"));
  REQUIRE_NOTHROW(context.run("assert(add(2,3) == 5);"));
  
  // callbacks
  REQUIRE_NOTHROW(context.run("store_callback(function(){ return 'stored callback has been called'; });"));
  REQUIRE_NOTHROW(context.run("assert(call_stored_callback() == 'stored callback has been called')"));
  
  // classes and auto update
  static unsigned my_class_instances = 0;
  struct MyClass{
    std::string data;
    MyClass(const MyClass &):data(){ my_class_instances++; }
    MyClass(){ my_class_instances++; }
    ~MyClass(){ my_class_instances--; }
  };
  extension.add_function("create_my_class",[](){ return MyClass(); });
  extension.add_function("get_my_class_data",[](const MyClass &my_class){ return my_class.data; });
  extension.add_function("set_my_class_data",[](MyClass &my_class,const std::string &str){ my_class.data = str; });
  
  REQUIRE_NOTHROW(context.run("var my_class = create_my_class()"));
  REQUIRE(my_class_instances == 1);
  REQUIRE_NOTHROW(context.run("set_my_class_data(my_class,'data')"));
  REQUIRE(context.getValue<std::string>("get_my_class_data(my_class)") == "data");
  REQUIRE_NOTHROW(context.run("my_class = undefined"));
  
  // destructor called
  REQUIRE(my_class_instances == 0);
  
  // capture and free
  REQUIRE_NOTHROW(context.run("function test(x){ store_callback(function(){ set_my_class_data(x,'data') }); } test(create_my_class())"));
  context.collect_garbage();
  REQUIRE(my_class_instances == 1);
  REQUIRE_NOTHROW(context.run("store_callback(function(){ })"));
  context.collect_garbage();
  REQUIRE(my_class_instances == 0);
  
  // inner extensions
  auto inner_extension = std::make_shared<Extension>();
  inner_extension->add_function("before", []()->std::string{ return "called inner extension function"; });
  extension.add_extension("inner", inner_extension);
  REQUIRE(context.getValue<std::string>("inner.before()") == "called inner extension function");
  
  // inner update
  inner_extension->add_function("after", []()->std::string{ return "called updated inner extension function"; });
  REQUIRE(context.getValue<std::string>("inner.after()") == "called updated inner extension function");
  
  // extensions extension
  auto extensions_extension = std::make_shared<Extension>();
  extension.add_extension("extensions", extensions_extension);
  extensions_extension->add_function("create_extension", [&](std::string name){
    auto new_extension = std::make_shared<Extension>();
    extension.add_extension(name, new_extension);
    return new_extension;
  });
  extensions_extension->add_function("add_function",[](std::shared_ptr<Extension> extension,std::string name,lars::AnyFunction f){ extension->add_function(name, f); });
  
  // call javascript function without return value
  REQUIRE_NOTHROW(context.run("var extension = extensions.create_extension('duk_extension')"));
  REQUIRE_NOTHROW(context.run("var res = ''"));
  REQUIRE_NOTHROW(context.run("extensions.add_function(extension,'f',function(x,y){ res = ('called f(' + x + ',' + y + ')' ) })"));
  REQUIRE_NOTHROW(context.run("duk_extension.f(1,'x'); assert(res == 'called f(1,x)');"));
  auto f = extension.get_extension("duk_extension")->get_function("f");
  REQUIRE(bool(f));
  REQUIRE_NOTHROW( f(std::string("x"),42) );
  REQUIRE(context.getValue<std::string>("res") == "called f(x,42)");
  REQUIRE_NOTHROW( f(1) );
  REQUIRE(context.getValue<std::string>("res") == "called f(1,undefined)");

  // call javascript function with return value
  REQUIRE_NOTHROW(context.run("extensions.add_function(extension,'g',function(x,y){ return x+y })"));
  REQUIRE_NOTHROW(context.run("assert(duk_extension.g('x',1) == 'x1')"));
  auto res = extension.get_extension("duk_extension")->get_function("g")(2,std::string("x"));
  REQUIRE(res);
  REQUIRE(res.get<std::string>() == "2x");
  res = extension.get_extension("duk_extension")->get_function("g")(2,40);
  REQUIRE(res);
  REQUIRE(res.get<double>() == 42);
  
  // call and return javascript object
  REQUIRE_NOTHROW(context.run("extensions.add_function(extension,'h',function(x){ return x.key2; })"));
  REQUIRE_NOTHROW(context.run("var map = { key1: 'value1', key2: 'value2' };"));
  REQUIRE_NOTHROW(context.run("assert(duk_extension.h(map) == 'value2');"));
  
  REQUIRE_NOTHROW(context.run("extensions.add_function(extension,'create_table',function(){ return { }; })"));
  REQUIRE_NOTHROW(context.run("extensions.add_function(extension,'set_key_value',function(t,k,v){ t[k] = v; })"));
  REQUIRE_NOTHROW(context.run("extensions.add_function(extension,'to_json',function(x){ return JSON.stringify(x); })"));
  
  auto create_table = extension.get_extension("duk_extension")->get_function("create_table");
  auto to_json = extension.get_extension("duk_extension")->get_function("to_json");
  auto set_key_value = extension.get_extension("duk_extension")->get_function("set_key_value");
  
  auto table = create_table();
  REQUIRE(to_json(table).get<std::string>() == "{}");
  set_key_value(table,"a",1);
  set_key_value(table,"b","2");
  set_key_value(table,"c",create_table());
  REQUIRE(to_json(table).get<std::string>() == "{\"a\":1,\"b\":\"2\",\"c\":{}}");

  // derived classes

  /* TODO
  struct A:lars::Visitable<A>{ int value; A(){} };
  struct B:lars::DVisitable<B,A>{ B(){ value = 1; } };
  struct C:lars::DVisitable<C,A>{ C(){ value = 2; } };
  
  extension.add_function("create_B", [](){ return B(); });
  extension.add_function("create_C", [](){ return C(); });
  extension.add_function("getValue", [](A & a){ return a.value; });
  REQUIRE_NOTHROW(context.run("assert(getValue(create_B()) == 1)"));
  REQUIRE_NOTHROW(context.run("assert(getValue(create_C()) == 2)"));
  REQUIRE_THROWS(context.run("getValue(create_my_class())"));
  */
}

#endif
