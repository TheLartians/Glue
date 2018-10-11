
#include <lars/log.h>
#include <lars/duktape.h>
#include <lars/lua.h>
#include <iostream>

#include <duktape/duktape.h>

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>


int main(int argc, char *argv[]) {
  lars::Extension extension;
  
  // call functions and get return values
  extension.add_function("meaning_of_life",[](){ return 42; });
  extension.add_function("log",[](const std::string &str){ std::cout << "log: " << str << std::endl; });
  extension.add_function("add",[](double a,int b){ return a+b; });

  assert(extension.get_function("add").return_type() == lars::get_type_index<decltype(double(1)+int(2))>());
  assert(extension.get_function("add").argument_type(0) == lars::get_type_index<double>());
  assert(extension.get_function("add").argument_type(1) == lars::get_type_index<int>());
  assert(extension.get_function("add")(lars::make_any<double>(2),lars::make_any<int>(3)).get_numeric<int>() == 5);

  // callbacks
  extension.add_function("get_log_function",[](const std::string &prompt){ return lars::AnyFunction([=](std::string str){ std::cout << prompt << ": " << str << std::endl; }); });
  extension.add_function("call_callback",[](const lars::AnyFunction &f){ f(42); });

  // store callback
  auto get_stored_function = []()->lars::AnyFunction &{ static lars::AnyFunction f; return f; };
  extension.add_function("store_callback",[&](const lars::AnyFunction &f){ get_stored_function() = f; });
  extension.add_function("call_stored_callback",[&](){ get_stored_function()(); });

  // create duktape context
  duk_context *ctx = duk_create_heap(NULL, NULL, NULL, NULL, NULL);
  
  lars::DuktapeGlue duktape_glue(ctx);
  extension.connect(duktape_glue);
  
  auto run_string = [&](std::string str){
    duk_push_string(ctx, str.c_str());
    bool complete = false;
    if (duk_peval(ctx) != 0) {
      std::cout << "Duk: " << duk_safe_to_string(ctx, -1) << std::endl;
    } else {
      complete = true;
      // std::cout << duk_safe_to_string(ctx, -1)) << std::endl;
    }
    duk_pop(ctx);  /* pop result */
    return complete;
  };
  
  // assertion and sanity check
  assert(run_string("function assert(x){ if(x) return; else throw 'assertion failed'; }"));
  assert(run_string("assert(1==1)"));
  assert(!run_string("assert(1==0)"));
  std::cout << "... as planned" << std::endl;

  // call functions
  assert(run_string("var value = meaning_of_life(); assert(value == 42)"));
  assert(run_string("log('hello world!')"));
  assert(run_string("assert(add(2,3) == 5);"));

  // callbacks
  assert(run_string("var new_log = get_log_function('test'); new_log(add(2,40))"));
  assert(run_string("function myLog(x){ log('myLog: ' + x); }; call_callback(myLog)"));
  assert(run_string("store_callback(function(){ log('stored callback has been called') });"));
  assert(run_string("call_stored_callback()"));
    
  // classes and auto update
  static unsigned my_class_instances = 0;
  struct MyClass{
    std::string data;
    MyClass(const MyClass &other):data(){ my_class_instances++; }
    MyClass(){ my_class_instances++; }
    ~MyClass(){ my_class_instances--; }
  };
  extension.add_function("create_my_class",[](){ return MyClass(); });
  extension.add_function("get_my_class_data",[](const MyClass &my_class){ return my_class.data; });
  extension.add_function("set_my_class_data",[](MyClass &my_class,const std::string &str){ my_class.data = str; });
  
  assert(run_string("var my_class = create_my_class()"));
  assert(my_class_instances == 1);
  assert(run_string("set_my_class_data(my_class,'data')"));
  assert(run_string("log(get_my_class_data(my_class))"));
  assert(run_string("my_class = undefined"));

  // destructor called
  assert(my_class_instances == 0);
  
  // capture and free
  assert(run_string("function test(x){ store_callback(function(){ set_my_class_data(x,'data') }); } test(create_my_class())"));
  duk_gc(ctx, 0);
  assert(my_class_instances == 1);
  assert(run_string("store_callback(function(){ })"));
  duk_gc(ctx, 0);
  assert(my_class_instances == 0);
  
  // inner extensions
  auto inner_extension = std::make_shared<lars::Extension>();
  inner_extension->add_function("before", []()->std::string{ return "called inner extension function"; });
  extension.add_extension("inner", inner_extension);
  assert(run_string("log(inner.before())"));

  // inner update
  inner_extension->add_function("after", []()->std::string{ return "called updated inner extension function"; });
  assert(run_string("log(inner.after())"));

  // extensions extension
  auto extensions_extension = std::make_shared<lars::Extension>();
  extension.add_extension("extensions", extensions_extension);
  extensions_extension->add_function("create_extension", [&](std::string name){
    auto new_extension = std::make_shared<lars::Extension>();
    extension.add_extension(name, new_extension);
    return new_extension;
  });
  extensions_extension->add_function("add_function",[](std::shared_ptr<lars::Extension> extension,std::string name,lars::AnyFunction f){ extension->add_function(name, f); });
  
  // call javascript function without return value
  assert(run_string("var extension = extensions.create_extension('duk_extension')"));
  assert(run_string("extensions.add_function(extension,'f',function(x,y){ log('called f(' + x + ',' + y + ')' ) })"));
  assert(run_string("duk_extension.f(1,'x')")); // -> called f(1,x)
  auto f = extension.get_extension("duk_extension")->get_function("f");
  assert(f);
  f(std::string("x"),42); // -> called f(x,42)
  f(1); // -> called f(1,undefined)
  
  // call javascript function with return value
  assert(run_string("extensions.add_function(extension,'g',function(x,y){ return x+y })"));
  assert(run_string("assert(duk_extension.g('x',1) == 'x1')"));
  auto res = extension.get_extension("duk_extension")->get_function("g")(2,std::string("x"));
  assert(res && res.get<std::string>() == "2x");
  res = extension.get_extension("duk_extension")->get_function("g")(2,40);
  assert(res && res.get_numeric<double>() == 42);
  
  // call and return javascript object
  assert(run_string("extensions.add_function(extension,'h',function(x){ return x.key2; })"));
  assert(run_string("var map = { key1: 'value1', key2: 'value2' };"));
  assert(run_string("assert(duk_extension.h(map) == 'value2');"));
  
  assert(run_string("extensions.add_function(extension,'create_table',function(){ return { }; })"));
  assert(run_string("extensions.add_function(extension,'set_key_value',function(t,k,v){ t[k] = v; })"));
  assert(run_string("extensions.add_function(extension,'to_json',function(x){ return JSON.stringify(x); })"));

  auto create_table = extension.get_extension("duk_extension")->get_function("create_table");
  auto to_json = extension.get_extension("duk_extension")->get_function("to_json");
  auto set_key_value = extension.get_extension("duk_extension")->get_function("set_key_value");
  
  auto table = create_table();
  LARS_LOG(to_json(table).get<std::string>());
  set_key_value(table,"a",1);
  set_key_value(table,"b","2");
  set_key_value(table,"c",create_table());
  LARS_LOG(to_json(table).get<std::string>());
  
  // derived classes
  struct A:lars::Visitable<A>{ int value; A(){} };
  struct B:lars::DVisitable<B,A>{ B(){ value = 1; } };
  struct C:lars::DVisitable<C,A>{ C(){ value = 2; } };
  
  extension.add_function("create_B", [](){ return B(); });
  extension.add_function("create_C", [](){ return C(); });
  extension.add_function("get_value", [](A & a){ return a.value; });
  assert(run_string("assert(get_value(create_B()) == 1)"));
  assert(run_string("assert(get_value(create_C()) == 2)"));
  assert(!run_string("get_value(create_my_class())"));

  return 0;
}
