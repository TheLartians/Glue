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
  
  // create duktape context
  duk_context *ctx = duk_create_heap(NULL, NULL, NULL, NULL, NULL);
  
  lars::DuktapeGlue duktape_glue(ctx);
  extension.connect(duktape_glue);
  
  auto javascript = [&](std::string str){
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
  
  // create lua context
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  
  lars::LuaGlue lua_glue(L);
  extension.connect(lua_glue);
  
  extension.add_function("rawprint", [](const std::string &str){ std::cout << str << std::endl; });
  
  auto lua = [&](std::string str){
    if(luaL_dostring(L, str.c_str())){
      std::cout << "lua error: " << lua_tostring(L,-1) << std::endl;
      return false;
    };
    return true;
  };

  // extensions extension
  auto extensions_extension = std::make_shared<lars::Extension>();
  extension.add_extension("extensions", extensions_extension);
  extensions_extension->add_function("create_extension", [&](std::string name){
    auto new_extension = std::make_shared<lars::Extension>();
    extension.add_extension(name, new_extension);
    return new_extension;
  });
  extensions_extension->add_function("add_function",[](std::shared_ptr<lars::Extension> extension,std::string name,lars::AnyFunction f){ extension->add_function(name, f); });
  
  assert(javascript("var javascript_extension = extensions.create_extension('javascript')"));
  assert(lua("lua_extension = extensions.create_extension('lua')"));

  assert(javascript("extensions.add_function(javascript_extension,'add',function(x,y){ return x + y; })"));
  assert(javascript("extensions.add_function(javascript_extension,'log',function(str){ rawprint('log: ' + str); })"));
  assert(lua("extensions.add_function(lua_extension,'add',function(x,y) return x + y end)"));
  assert(lua("extensions.add_function(lua_extension,'log',function(str) print('log: ' .. str) end)"));

  // call javascript from lua
  assert(lua("lua.log(javascript.add(4,2))")); // log: 6.0
  assert(lua("lua.log(javascript.add(4,'2'))")); // log: 42

  // call lua from javascript
  assert(javascript("javascript.log(lua.add(4,2))")); // log: 6
  assert(javascript("javascript.log(lua.add(4,'2'))")); // log: 6.0


  // call javascript and lua from c
  auto js_add = extension.get_extension("javascript")->get_function("add");
  auto lua_add = extension.get_extension("lua")->get_function("add");
  auto log = extension.get_extension("lua")->get_function("log");
  
  log(js_add(2,"3")); // log: 23
  log(lua_add(2,"3")); // log: 5.0

  return 0;
}
