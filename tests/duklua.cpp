
#include <catch2/catch.hpp>
#include <stdexcept>

#include <glue/glue.h>
#include <glue/duktape.h>
#include <glue/lua.h>

#if false


TEST_CASE("DukLua"){
  using namespace glue;

  Extension extension;
  
  // create duktape context
  DuktapeContext duk;
  extension.connect(duk.get_glue());
    
  // create lua context
  LuaState lua;
  lua.openLibs();
  extension.connect(lua.get_glue());
  
  // extensions extension
  auto extensions_extension = std::make_shared<Extension>();
  extension.add_extension("extensions", extensions_extension);
  extensions_extension->add_function("create_extension", [&](std::string name){
    auto new_extension = std::make_shared<Extension>();
    extension.add_extension(name, new_extension);
    return new_extension;
  });

  extensions_extension->add_function("add_function",[](std::shared_ptr<Extension> extension,std::string name, lars::AnyFunction f){ extension->add_function(name, f); });
  
  REQUIRE_NOTHROW(duk.run("var duk_extension = extensions.create_extension('duk')"));
  REQUIRE_NOTHROW(lua.run("lua_extension = extensions.create_extension('lua')"));

  REQUIRE_NOTHROW(duk.run("extensions.add_function(duk_extension,'add',function(x,y){ return x + y; })"));
  REQUIRE_NOTHROW(lua.run("extensions.add_function(lua_extension,'add',function(x,y) return x + y end)"));

  // call duk from lua
  REQUIRE_NOTHROW(lua.getValue<std::string>("tostring(duk.add(4,2))") == "6.0");
  REQUIRE_NOTHROW(lua.getValue<std::string>("tostring(duk.add(4,'2'))") == "42");

  // call lua from duk
  REQUIRE_NOTHROW(duk.getValue<std::string>("(lua.add(4,2))") == "6");
  REQUIRE_NOTHROW(duk.getValue<std::string>("(lua.add(4,'2'))") == "6.0"); 

  // call duk and lua from c
  auto js_add = extension.get_extension("duk")->get_function("add");
  auto lua_add = extension.get_extension("lua")->get_function("add");
  
  REQUIRE(js_add(2,"3").get<std::string>() == "23");
  REQUIRE(lua_add(2,"3").get<int>() == 5);

}

#endif
