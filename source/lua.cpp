#include <lars/lua.h>

#include <lars/log.h>

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#define LARS_LUA_GLUE_DEBUG

#ifdef LARS_LUA_GLUE_DEBUG
#define LARS_LUA_GLUE_LOG(X) LARS_LOG_WITH_PROMPT(X,"lua glue: ")
#else
#define LARS_LUA_GLUE_LOG(X)
#endif

namespace {
  namespace lua_glue{
    
    template <class T> using internal_type = std::pair<lars::TypeIndex,T>;

    std::string UNUSED as_string(lua_State * L,int idx = -1){
      lua_getglobal(L, "tostring");
      lua_pushvalue(L, idx >= 0 ? idx : idx-1);
      lua_call(L, 1, 1);
      auto res = lua_tostring(L, -1);
      std::string result = res ? res : "";
      lua_pop(L, 1);
      return result;
    }
    
    int add_to_registry(lua_State * L,int idx = -1){
      static int key = 0;
      key++;
      lua_pushvalue(L, idx);
      LARS_LUA_GLUE_LOG("add " << key << " to registry: " << as_string(L,idx));
      lua_rawseti(L, LUA_REGISTRYINDEX, key);
      lua_pop(L, 1);
      return key;
    }
    
    void push_from_registry(lua_State * L,int key){
      lua_rawgeti(L, LUA_REGISTRYINDEX, key);
      LARS_LUA_GLUE_LOG("push " << key << " from registry.");
    }
    
    void remove_from_registry(lua_State * L,int key){
      LARS_LUA_GLUE_LOG("remove " << key << " from registry.");
      lua_pushnil(L);
      lua_rawseti(L, LUA_REGISTRYINDEX, key);
    }
    
    struct RegistryObject{
      lua_State * L;
      int key;
      RegistryObject(lua_State * l,int k):L(l),key(k){ LARS_LUA_GLUE_LOG("create registry object: " << key); }
      RegistryObject(const RegistryObject &) = delete;
      ~RegistryObject(){ LARS_LUA_GLUE_LOG("delete registry object: " << key); remove_from_registry(L, key); }
      void push()const{ push_from_registry(L, key); }
    };
    
    const int OBJECT_POINTER_INDEX = -1;
    
    template <class T> bool push_class_metatable(lua_State * L){
      auto key = "__glue_constructor_" + lars::get_type_name<T>();
      lua_pushstring(L, key.c_str());
      lua_rawget(L, LUA_REGISTRYINDEX);
      if(!lua_isnil(L, -1)) return false;
      lua_pop(L, 1); // nil is on top
      LARS_LUA_GLUE_LOG("creatign metatable for class " << lars::get_type_name<T>());
      lua_pushstring(L, key.c_str());
      lua_newtable(L);
      lua_pushcfunction(L, +[](lua_State *L){
        lua_rawgeti(L, -1, OBJECT_POINTER_INDEX);
        auto * data = static_cast<internal_type<T>*>(lua_touserdata(L, -1));
        assert(data->first == lars::get_type_index<T>());
        delete data;
        return 0;
      });
      lua_setfield(L, -2, "__gc");
      
      lua_rawset(L, LUA_REGISTRYINDEX);
      push_class_metatable<T>(L);
      return true;
    }
    
    template <class T> T & create_and_push_object(lua_State * L){
      lua_newtable(L);
      push_class_metatable<T>(L);
      lua_setmetatable(L, -2);
      auto data = new internal_type<T>();
      lua_pushlightuserdata(L, data);
      lua_rawseti(L, -2, OBJECT_POINTER_INDEX);
      return *data;
    }
    
    template <class T> T & get_object(lua_State *L,int idx = -1){
      lua_rawgeti(L, idx, OBJECT_POINTER_INDEX);
      auto * data = static_cast<internal_type<T>*>(lua_touserdata(L, -1));
      if(!data || data->first != lars::get_type_index<T>()) throw std::runtime_error("cannot extract c++ object of type " + lars::get_type_name<T>());
      return *data;
    }
    
    
    void push_function(lua_State * L,const lars::AnyFunction &f){
      lua_newtable(L);
      if( push_class_metatable<lars::AnyFunction>(L) ){
        // set __call metamethod
      }
      lua_setmetatable(L, -2);
    }

    
  }
}

namespace lars {

  LuaGlue::LuaGlue(lua_State * s):state(s){
    lua_pushglobaltable(state);
    keys[nullptr] = lua_glue::add_to_registry(s);
  }

  LuaGlue::~LuaGlue(){
    
  }

  int LuaGlue::get_key(const Extension *parent)const{
    auto it = keys.find(parent);
    if(it != keys.end()) return it->second;
    return keys.find(nullptr)->second;
  }
  
  void LuaGlue::connect_function(const Extension *parent,const std::string &name,const AnyFunction &f){
    LARS_LUA_GLUE_LOG("connecting function " << name);
    lua_glue::push_from_registry(state, get_key(parent));
    LARS_LOG("top at " << lua_gettop(state) << ": " << lua_glue::as_string(state));
    lua_glue::push_function(state,f);
    LARS_LOG("top at " << lua_gettop(state) << ": " << lua_glue::as_string(state));
    lua_setfield(state, -2, name.c_str());
    lua_pop(state, 1);
  }
  
  void LuaGlue::connect_extension(const Extension *parent,const std::string &name,const Extension &e){
    LARS_LUA_GLUE_LOG("connecting extension " << name);
    lua_glue::push_from_registry(state, get_key(parent));
    lua_newtable(state);
    lua_pushvalue(state, -1);
    keys[&e] = lua_glue::add_to_registry(state);
    lua_setfield(state, -2, name.c_str());
    e.connect(*this);
  }
  
}
