
#include <lars/log.h>
#include <lars/duktape.h>
#include <iostream>

#include <lua.hpp>
#include <lars/lua.h>

int main(int argc, char *argv[]) {
  auto shared_extension = std::make_shared<lars::Extension>();
  lars::Extension &extension = *shared_extension;
  
  // call functions and get return values
  extension.add_function("meaning_of_life",[](){ return 42; });
  extension.add_function("log",[](const std::string &str){ std::cout << "log: " << str << std::endl; });
  extension.add_function("add",[](float a,int b){ return a+b; });

  assert(extension.get_function("add").return_type() == lars::get_type_index<decltype(float(1)+int(2))>());
  assert(extension.get_function("add").argument_type(0) == lars::get_type_index<float>());
  assert(extension.get_function("add").argument_type(1) == lars::get_type_index<int>());
  assert(extension.get_function("add")(lars::make_any<double>(2),lars::make_any<int>(3)).get_numeric<int>() == 5);

  // callbacks
  extension.add_function("get_log_function",[](const std::string &prompt){ return lars::AnyFunction([=](std::string str){ std::cout << prompt << ": " << str << std::endl; }); });
  extension.add_function("call_callback",[](const lars::AnyFunction &f){ f(42); });

  // store callback
  auto get_stored_function = []()->lars::AnyFunction &{ static lars::AnyFunction f; return f; };
  extension.add_function("store_callback",[&](const lars::AnyFunction &f){ get_stored_function() = f; });
  extension.add_function("call_stored_callback",[&](lars::AnyArguments &args){ return get_stored_function().call(args); });

  // create lua context
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  
  lars::LuaGlue lua_glue(L);
  extension.connect(lua_glue);
  
  
  auto run_string = [&](std::string str){
    if(luaL_dostring(L, str.c_str())){
      std::cout << "lua error: " << lua_tostring(L,-1) << std::endl;
      lua_pop(L,1);
      return false;
    };
    return true;
  };

  assert( run_string("function assert(x, msg) if not x then error(msg or 'assertion failed') end end") );
  assert( run_string("assert(1 == 1)") );
  assert( !run_string("assert(1 == 0)") );
  std::cout << "... as planned" << std::endl;

  assert(run_string("local value = meaning_of_life(); assert(value == 42)"));
  assert(run_string("log('hello lua!')"));
  assert(run_string("log(meaning_of_life())"));
  assert(run_string("assert(add(2,3) == 5);"));

  
  // callbacks
  assert(run_string("local new_log = get_log_function('test'); new_log(add(2,40))"));
  assert(run_string("local function myLog(x) log('myLog: ' .. x) print('test 2123') end call_callback(myLog)"));

  assert(run_string("store_callback(function(n) log('stored callback has been called') return n+1; end);"));
  assert(run_string("assert(call_stored_callback(4) == 5)"));

  // classes and auto update
  static unsigned my_class_instances = 0;
  struct MyClass{
    std::string data;
    MyClass(const MyClass &other):data(other.data){ my_class_instances++; }
    MyClass(){ my_class_instances++; }
    ~MyClass(){ my_class_instances--; }
  };
  extension.add_function("create_my_class",[](){ return MyClass(); });
  extension.add_function("get_my_class_data",[](const MyClass &my_class){ return my_class.data; });
  extension.add_function("set_my_class_data",[](MyClass &my_class,const std::string &str){ my_class.data = str; });
  
  
  run_string("print('type of set_my_class_data: ',set_my_class_data)");

  assert(run_string("my_class = create_my_class()"));
  lua_gc(L, LUA_GCCOLLECT, 0);
  assert(my_class_instances == 1);
  run_string("print('type of set_my_class_data: ',set_my_class_data)");

  assert(run_string("set_my_class_data(my_class,'data')"));
  assert(run_string("log(get_my_class_data(my_class))"));
  assert(run_string("my_class = nil"));
  lua_gc(L, LUA_GCCOLLECT, 0);

  // destructor called
  assert(my_class_instances == 0);
  
  // capture and free
  assert(run_string("function test(x) store_callback(function() set_my_class_data(x,'data') end); end test(create_my_class())"));
  lua_gc(L, LUA_GCCOLLECT, 0);
  assert(my_class_instances == 1);
  assert(run_string("store_callback(function()end)"));
  lua_gc(L, LUA_GCCOLLECT, 0);
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
  
  // call lua function without return value
  assert(run_string("extension = extensions.create_extension('lua_extension')"));
  assert(run_string("extensions.add_function(extension,'f',function(x,y) log('called f(' .. tostring(x) .. ',' .. tostring(y) .. ')' ) end)"));
  assert(run_string("lua_extension.f(1,'x')")); // -> called f(1,x)
  auto f = extension.get_extension("lua_extension")->get_function("f");
  assert(f);
  f(std::string("x"),42); // -> called f(x,42)
  f(1); // -> called f(1,nil)
  
  // call lua function with return value
  assert(run_string("extensions.add_function(extension,'g',function(x,y) return tostring(x) .. tostring(y) end)"));
  assert(run_string("assert(lua_extension.g('x','1') == 'x1')"));
  auto res = extension.get_extension("lua_extension")->get_function("g")(2,std::string("x"));
  assert(res && res.get<std::string>() == "2x");
  res = extension.get_extension("lua_extension")->get_function("g")(2,40);
  assert(res && res.get<std::string>() == "240");
  
  // call and return lua object
  assert(run_string("extensions.add_function(extension,'h',function(x) return x.key2; end)"));
  assert(run_string("map = { key1 = 'value1', key2 = 'value2' };"));
  assert(run_string("assert(lua_extension.h(map) == 'value2');"));
  
  assert(run_string("extensions.add_function(extension,'create_table',function() return { } end)"));
  assert(run_string("extensions.add_function(extension,'set_key_value',function(t,k,v) t[k] = v end)"));
  assert(run_string("extensions.add_function(extension,'table_to_string',function(x) local res = '' for k,v in pairs(x) do res = res .. tostring(k) .. '=' .. tostring(v) .. ',' end return res end)"));

  auto create_table = extension.get_extension("lua_extension")->get_function("create_table");
  auto set_key_value = extension.get_extension("lua_extension")->get_function("set_key_value");
  auto table_to_string = extension.get_extension("lua_extension")->get_function("table_to_string");

  auto table = create_table();
  set_key_value(table,"a",1);
  set_key_value(table,"b","2");
  set_key_value(table,"c",create_table());
  LARS_LOG(table_to_string(table).get<std::string>());
  
  // derived classes
  struct A:lars::Visitable<A>{ int value; A(){} };
  struct B:lars::DVisitable<B,A>{ B(){ value = 1; } };
  struct C:lars::DVisitable<C,A>{ C(){ value = 2; } };

  extension.add_function("create_B", [](){ return B(); });
  extension.add_function("create_C", [](){ return C(); });
  extension.add_function("get_value", [](A & a){ return a.value; });
  extension.add_function("get_Cvalue", [](C & a){ return a.value; });
  assert(run_string("assert(get_value(create_B()) == 1)"));
  assert(run_string("assert(get_value(create_C()) == 2)"));
  assert(run_string("assert(get_Cvalue(create_C()) == 2)"));
  assert(!run_string("assert(get_Cvalue(create_B()) == 2)"));
  assert(!run_string("get_value(create_my_class())"));
  assert(!run_string("get_value('test')"));

  // class extensions
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
  
  assert(run_string("local a = MyClass.new(); a:set_data('a'); assert(a:get_data() == 'a');"));
  assert(run_string("local b = MyClass.new('b'); assert(b:get_data() == 'b');"));

  // Derived classes
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

  assert(run_string("a = MyDerivedClass.new()"));
  assert(run_string("assert(a:data_2() == '2')"));
  assert(run_string("a:set_data('hello')"));
  assert(run_string("assert(a:data_2() == 'hello2')"));
  assert(run_string("assert(a:shared_data_2() == 'hello2')"));

  assert(run_string("a = MyDerivedClass.shared_new()"));
  assert(run_string("assert(a:data_2() == '2')"));
  assert(run_string("a:set_data('hello')"));
  assert(run_string("assert(a:shared_data_2() == 'hello2')"));
  //  assert(run_string("assert(a:shared_get_data() == 'hello')"));

  // Coroutines
  assert(run_string("wait = coroutine.create(function() \
                      store_callback(function(n) return coroutine.yield(n+3); end) \
                      call_stored_callback(4); \
                    end)"));
  assert(run_string("local status, value = coroutine.resume(wait); assert(status and value == 7, value);"));
  
  try { // yielding from c will not work but should raise an exception
    assert( extension.get_function("call_stored_callback")(1).get<int>() == 4);
  } catch (std::exception &e) {
    extension.get_function("log")(std::string(e.what()));
  }
  
  extension.get_function("log")("all tests passed");
  
  return 0;
}
