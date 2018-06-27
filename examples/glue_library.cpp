
#include <lars/log.h>
#include <lars/duktape.h>
#include <iostream>

int main(int argc, char *argv[]) {
  lars::Extension extension;
  
  // call functions and get return values
  extension.add_function("meaning_of_life",[](){ return 42; });
  extension.add_function("log",[](const std::string &str){ std::cout << "log: " << str << std::endl; });
  extension.add_function("add",[](double a,int b){ return a+b; });
  
  // callbacks
  extension.add_function("get_log_function",[](const std::string &prompt){ return lars::AnyFunction([=](std::string str){ std::cout << prompt << ": " << str << std::endl; }); });
  extension.add_function("call_callback",[](const lars::AnyFunction &f){ f(42); });

  // store callback
  auto get_stored_function = []()->lars::AnyFunction &{ static lars::AnyFunction f; return f; };
  extension.add_function("store_callback",[&](const lars::AnyFunction &f){ get_stored_function() = f; });
  extension.add_function("call_stored_callback",[&](){ get_stored_function()(); });

  duk_context *ctx = duk_create_heap(NULL, NULL, NULL, NULL, NULL);
  
  lars::DuktapeGlue glue(ctx);
  extension.connect(glue);
  
  auto evaluate_string = [&](std::string str){
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
  
  // call functions
  assert(evaluate_string("var value = meaning_of_life(); if(value != 42) throw 'wrong universe!'"));
  assert(!evaluate_string("var value = meaning_of_life(); if(value == 42) throw 'correct universe!'"));
  assert(evaluate_string("log('hello world!')"));
  assert(evaluate_string("log('2+3=' + add(2, 3))"));
  
  // callbacks
  assert(evaluate_string("var new_log = get_log_function('test'); new_log(add(2,40))"));
  assert(evaluate_string("function myLog(x){ log('myLog: ' + x); }; call_callback(myLog)"));
  assert(evaluate_string("store_callback(function(){ log('stored callback has been called') }); log('stored callback');"));
  assert(evaluate_string("call_stored_callback()"));
    
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
  
  assert(evaluate_string("var my_class = create_my_class()"));
  assert(my_class_instances == 1);
  assert(evaluate_string("set_my_class_data(my_class,'data')"));
  assert(evaluate_string("log(get_my_class_data(my_class))"));
  assert(evaluate_string("my_class = undefined"));

  // destructor called
  assert(my_class_instances == 0);
  
  // capture and free
  assert(evaluate_string("function test(x){ store_callback(function(){ set_my_class_data(x,'data') }); } test(create_my_class())"));
  duk_gc(ctx, 0);
  assert(my_class_instances == 1);
  assert(evaluate_string("store_callback(function(){ })"));
  duk_gc(ctx, 0);
  assert(my_class_instances == 0);
  
  // inner extensions
  auto inner_extension = std::make_shared<lars::Extension>();
  inner_extension->add_function("before", []()->std::string{ return "called inner extension function"; });
  extension.add_extension("inner", inner_extension);
  assert(evaluate_string("log(inner.before())"));

  // inner update
  inner_extension->add_function("after", []()->std::string{ return "called updated inner extension function"; });
  assert(evaluate_string("log(inner.after())"));

  // extensions extension
  auto extensions_extension = std::make_shared<lars::Extension>();
  extension.add_extension("extensions", extensions_extension);
  extensions_extension->add_function("create_extension", [&](std::string name){
    auto new_extension = std::make_shared<lars::Extension>();
    extension.add_extension(name, new_extension);
    return new_extension;
  });
  extensions_extension->add_function("add_function",[](std::shared_ptr<lars::Extension> extension,std::string name,lars::AnyFunction f){ extension->add_function(name, f); });
  
  assert(evaluate_string("var extension = extensions.create_extension('duk_extension')"));
  assert(evaluate_string("extensions.add_function(extension,'f',function(x,y){ log('called f(' + x + ',' + y + ')' ) })"));
  auto f = extension.get_extension("duk_extension")->get_function("f");
  assert(f);
  f(std::string("x"),42); // -> called f(x,42)
  f(); // -> called f(42)

  return 0;
}
