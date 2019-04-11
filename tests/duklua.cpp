
#include <catch2/catch.hpp>
#include <stdexcept>

#include <lars/glue.h>
#include <lars/duktape_glue.h>
#include <lars/lua_glue.h>

TEST_CASE("DukLua"){
  lars::Extension extension;
  
  // create duktape context
  lars::DuktapeContext duk;
  extension.connect(duk.get_glue());
    
  // create lua context
  lars::LuaState lua;
  lua.open_libs();
  extension.connect(lua.get_glue());
  
  // extensions extension
  auto extensions_extension = std::make_shared<lars::Extension>();
  extension.add_extension("extensions", extensions_extension);
  extensions_extension->add_function("create_extension", [&](std::string name){
    auto new_extension = std::make_shared<lars::Extension>();
    extension.add_extension(name, new_extension);
    return new_extension;
  });

  extensions_extension->add_function("add_function",[](std::shared_ptr<lars::Extension> extension,std::string name,lars::AnyFunction f){ extension->add_function(name, f); });
  
  REQUIRE_NOTHROW(duk.run("var duk_extension = extensions.create_extension('duk')"));
  REQUIRE_NOTHROW(lua.run("lua_extension = extensions.create_extension('lua')"));

  REQUIRE_NOTHROW(duk.run("extensions.add_function(duk_extension,'add',function(x,y){ return x + y; })"));
  REQUIRE_NOTHROW(lua.run("extensions.add_function(lua_extension,'add',function(x,y) return x + y end)"));

  // call duk from lua
  REQUIRE_NOTHROW(lua.get_value<std::string>("tostring(duk.add(4,2))") == "6.0");
  REQUIRE_NOTHROW(lua.get_value<std::string>("tostring(duk.add(4,'2'))") == "42");

  // call lua from duk
  REQUIRE_NOTHROW(duk.get_value<std::string>("(lua.add(4,2))") == "6");
  REQUIRE_NOTHROW(duk.get_value<std::string>("(lua.add(4,'2'))") == "6.0"); 

  // call duk and lua from c
  auto js_add = extension.get_extension("duk")->get_function("add");
  auto lua_add = extension.get_extension("lua")->get_function("add");
  
  REQUIRE(js_add(2,"3").get<std::string>() == "23");
  REQUIRE(lua_add(2,"3").get_numeric<int>() == 5);

}