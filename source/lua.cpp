#include <lars/lua.h>

#include <lars/log.h>

#include <lua.hpp>

//#define LARS_LUA_GLUE_DEBUG

#ifdef LARS_LUA_GLUE_DEBUG
#define LARS_LUA_GLUE_LOG(X) LARS_LOG_WITH_PROMPT(X,"lua glue: ")
#else
#define LARS_LUA_GLUE_LOG(X)
#endif

namespace {
  namespace lua_glue{
    
    template <class T> using internal_type = std::pair<lars::TypeIndex,T>;

    std::string UNUSED as_string(lua_State * L,int idx = -1){
      auto res = luaL_tolstring(L,idx,nullptr);
      std::string result = res ? res : "nullptr";
      lua_pop(L, 1);
      return result;
    }
    
    void UNUSED print_stack(lua_State * L){
      LARS_LOG("Dumping stack...");
      for(int i=lua_gettop(L);i>=1;--i){
        LARS_LOG("Stack at " << i << ": " << as_string(L,i));
      }
    }
    
    using RegistryKey = std::string;
    
    RegistryKey add_to_registry(lua_State * L,int idx = -1){
      static int obj_id = 0;
      obj_id++;
      auto key = "GLUE_REGISTRY_" + std::to_string(obj_id);
      lua_pushstring(L, key.c_str());
      lua_pushvalue(L, idx > 0 ? idx : idx - 1);
      LARS_LUA_GLUE_LOG("add " << key << " to registry: " << as_string(L));
      lua_rawset(L, LUA_REGISTRYINDEX);
      return key;
    }
    
    void push_from_registry(lua_State * L,const RegistryKey &key){
      lua_pushstring(L, key.c_str());
      lua_rawget(L, LUA_REGISTRYINDEX);
      LARS_LUA_GLUE_LOG("push " << key << " from registry: " << as_string(L));
    }
    
    void remove_from_registry(lua_State * L,const RegistryKey &key){
#ifdef LARS_LUA_GLUE_DEBUG
      lua_pushstring(L, key.c_str());
      lua_rawget(L, LUA_REGISTRYINDEX);
      LARS_LUA_GLUE_LOG("remove " << key << " from registry: " << as_string(L));
      lua_pop(L, 1);
#endif
      lua_pushstring(L, key.c_str());
      lua_pushnil(L);
      lua_rawset(L, LUA_REGISTRYINDEX);
    }
    
    struct RegistryObject{
      lua_State * L;
      RegistryKey key;
      RegistryObject(lua_State * l,const RegistryKey &k):L(l),key(k){ LARS_LUA_GLUE_LOG("create registry object: " << key); }
      RegistryObject(const RegistryObject &) = delete;
      RegistryObject(RegistryObject &&other):L(other.L),key(other.key){ other.L = nullptr; }
      ~RegistryObject(){ if(L){ LARS_LUA_GLUE_LOG("delete RegistryObject(" << key << ")"); remove_from_registry(L, key); } }
      void push()const{ push_from_registry(L, key); }
    };
    
    const int OBJECT_POINTER_INDEX = 0;
    
    template <class T> T & get_object(lua_State *L,int idx = -1);
    lars::Any extract_value(lua_State * L,int idx,lars::TypeIndex type);
    void push_value(lua_State * L,const lars::Any &value);
    
    template <class T> void push_class_metatable(lua_State * L){
      auto key = "GlueMetatable<" + lars::get_type_name<T>() + ">";
      lua_pushstring(L, key.c_str());
      lua_rawget(L, LUA_REGISTRYINDEX);
      if(!lua_isnil(L, -1)) return;
      lua_pop(L, 1); // nil is on top

      LARS_LUA_GLUE_LOG("creating metatable for class " << lars::get_type_name<T>());
      
      lua_pushstring(L, key.c_str());
      lua_newtable(L);

      lua_pushcfunction(L, +[](lua_State *L){
        lua_rawgeti(L, -1, OBJECT_POINTER_INDEX);
        auto * data = static_cast<internal_type<T>*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        lua_pushstring(L, ("Glued<" + lars::get_type_name<T>() + ">(" + std::to_string((size_t)data) + ")").c_str());
        return 1;
      });
      lua_setfield(L, -2, "__tostring");

      lua_pushcfunction(L, +[](lua_State *L){
        lua_rawgeti(L, -1, OBJECT_POINTER_INDEX);
        auto * data = static_cast<internal_type<T>*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        assert(data->first == lars::get_type_index<T>());
        delete data;
        return 0;
      });
      
      lua_setfield(L, -2, "__gc");

      if(lars::get_type_index<T>() == lars::get_type_index<lars::AnyFunction>()){
        lua_pushcfunction(L, +[](lua_State *L){
          try{
            LARS_LUA_GLUE_LOG("calling " << as_string(L,1) << " with " << lua_gettop(L)-1 << " arguments");
            auto & f = get_object<lars::AnyFunction>(L,1);
            lars::AnyArguments args;
            auto argc = f.argument_count();
            if(argc == -1) argc = lua_gettop(L)-1;
            LARS_LUA_GLUE_LOG("calling function " << &f << " with " << argc << " arguments");
            for(int i=0;i<argc;++i) args.emplace_back(extract_value(L, i+2, f.argument_type(i)));
            auto result = f.call(args);
            if(result){
              LARS_LUA_GLUE_LOG("returning " << result.type().name());
              push_value(L, result);
              return 1;
            }
            LARS_LUA_GLUE_LOG("returning nothing");
            return 0;
          }
          catch(std::exception &e){
            throw luaL_error(L, e.what());
          }
          catch(const char *s){
            throw luaL_error(L, s);
          }
          catch(...){
            throw luaL_error(L, "unknown exception");
          }
        });
        lua_setfield(L, -2, "__call");
      }
      
      lua_rawset(L, LUA_REGISTRYINDEX);
      
      lua_pushstring(L, key.c_str());
      lua_rawget(L, LUA_REGISTRYINDEX);
      LARS_LUA_GLUE_LOG("completed metatable: " << as_string(L));
      return;
    }
    
    template <class T> T & create_and_push_object(lua_State * L){
      lua_newtable(L);
      LARS_LUA_GLUE_LOG("creating object for type " << lars::get_type_name<T>() << ":" << as_string(L));
      push_class_metatable<T>(L);
      lua_setmetatable(L, -2);
      auto data = new internal_type<T>(lars::get_type_index<T>(),T());
      lua_pushlightuserdata(L, data);
      lua_rawseti(L, -2, OBJECT_POINTER_INDEX);
      LARS_LUA_GLUE_LOG("after setting metatable: " << as_string(L));
      return data->second;
    }
    
    template <class T> T * get_object_ptr(lua_State *L,int idx = -1){
      LARS_LUA_GLUE_LOG("getting object pointer for " << lars::get_type_name<T>() << " from " << as_string(L,idx));
      if(!lua_istable(L, idx)){ LARS_LUA_GLUE_LOG("not a table"); return nullptr; }
      lua_rawgeti(L, idx, OBJECT_POINTER_INDEX);
      auto * data = static_cast<internal_type<T>*>(lua_touserdata(L, -1));
      lua_pop(L, 1);
      if(!data || data->first != lars::get_type_index<T>()){
        if(data) LARS_LUA_GLUE_LOG("type mismatch: " << data->first.name());
        else LARS_LUA_GLUE_LOG("received nullptr");
        return nullptr;
      }
      return &data->second;
    }
    
    template <class T> T & get_object(lua_State *L,int idx){
      auto * data = get_object_ptr<T>(L,idx);
      if(!data){
        // never returns
        throw luaL_error(L,("cannot extract c++ object of type " + lars::get_type_name<T>() + " from " + as_string(L,idx)).c_str());
      }
      return *data;
    }
    
    void push_function(lua_State * L,const lars::AnyFunction &f);
    
    void push_value(lua_State * L,const lars::Any &value){
      using namespace lars;
      
      if(!value){
        LARS_LUA_GLUE_LOG("push nil");
        return lua_pushnil(L);
      }
      
      struct PushVisitor:public ConstVisitor<lars::VisitableType<double>,lars::VisitableType<std::string>,lars::VisitableType<lars::AnyFunction>,lars::VisitableType<int>,lars::VisitableType<bool>,lars::VisitableType<RegistryObject>>{
        lua_State * L;
        bool push_any = false;
        void visit_default(const lars::VisitableBase &data)override{ LARS_LUA_GLUE_LOG("push any<" << data.type().name() << ">"); push_any = true; }
        void visit(const lars::VisitableType<bool> &data)override{ LARS_LUA_GLUE_LOG("push bool"); lua_pushboolean(L, data.data); }
        void visit(const lars::VisitableType<int> &data)override{ LARS_LUA_GLUE_LOG("push int"); lua_pushinteger(L, data.data); }
        void visit(const lars::VisitableType<double> &data)override{ LARS_LUA_GLUE_LOG("push double"); lua_pushnumber(L, data.data); }
        void visit(const lars::VisitableType<std::string> &data)override{ LARS_LUA_GLUE_LOG("push string"); lua_pushstring(L, data.data.c_str()); }
        void visit(const lars::VisitableType<lars::AnyFunction> &data)override{ LARS_LUA_GLUE_LOG("push function"); push_function(L,data.data); }
        void visit(const lars::VisitableType<RegistryObject> &data)override{ LARS_LUA_GLUE_LOG("push object"); data.data.push(); }
      } visitor;
      
      visitor.L = L;
      value.accept_visitor(visitor);
      if(visitor.push_any) create_and_push_object<lars::Any>(L) = value;
    }
    
    lars::Any extract_value(lua_State * L,int idx,lars::TypeIndex type){
      LARS_LUA_GLUE_LOG("extract " << type.name() << " from " << as_string(L,idx));
      
      auto assert_value_exists = [&](auto && v){ if(!v) throw luaL_error(L,("invalid argument type for argument " + std::to_string(idx+1) + ". Expected " + std::string(type.name().begin(),type.name().end()) + ", got " + as_string(L,idx)).c_str()); return v; };
      
      if(auto ptr = get_object_ptr<lars::Any>(L,idx)){
        if(ptr->type() == type || type == lars::get_type_index<lars::Any>()){
          LARS_LUA_GLUE_LOG("extracted any<" << ptr->type().name() << ">");
          return *ptr;
        }
        else{
          LARS_LUA_GLUE_LOG("unsafe extracted any<" << ptr->type().name() << ">");
          return *ptr;
        }
      }
      
      if(type == lars::get_type_index<lars::Any>()){
        switch (lua_type(L, idx)) {
          case LUA_TNUMBER: type = lars::get_type_index<LUA_NUMBER>(); break;
          case LUA_TSTRING: type = lars::get_type_index<std::string>(); break;
          case LUA_TBOOLEAN: type = lars::get_type_index<bool>(); break;
          case LUA_TTABLE: type = lars::get_type_index<RegistryObject>(); break;
          case LUA_TFUNCTION: type = lars::get_type_index<lars::AnyFunction>(); break;
          default: break;
        }
      }
      
      if(type == lars::get_type_index<std::string>()){ return lars::make_any<std::string>(assert_value_exists(lua_tostring(L, idx))); }
      else if(type == lars::get_type_index<double>()){ return lars::make_any<double>(assert_value_exists(lua_tonumber(L, idx))); }
      else if(type == lars::get_type_index<int>()){ return lars::make_any<int>(assert_value_exists(lua_tointeger(L, idx))); }
      else if(type == lars::get_type_index<bool>()){ return lars::make_any<bool>(assert_value_exists(lua_toboolean(L, idx))); }
      else if(type == lars::get_type_index<RegistryObject>()){ return lars::make_any<RegistryObject>(L,add_to_registry(L,idx)); }
      else if(type == lars::get_type_index<lars::AnyFunction>()){
        auto captured = std::make_shared<RegistryObject>(L,add_to_registry(L,idx));
        lars::AnyFunction f = [captured](lars::AnyArguments &args){
          auto L = captured->L;
          captured->push();
          LARS_LUA_GLUE_LOG("calling registry " << captured->key << " with " << args.size() << " arguments: " << as_string(L));
          for(auto && arg:args) push_value(L, arg);
          lua_call(L, args.size(), 1);
          LARS_LUA_GLUE_LOG("return " << as_string(L));
          if(lua_isnil(L, -1)){ lua_pop(L, 1); return lars::Any(); }
          auto result = extract_value(L, -1, lars::get_type_index<lars::Any>());
          lua_pop(L, 1);
          return result;
        };
        return lars::make_any<lars::AnyFunction>(f);
      }
      else{
        throw luaL_error(L,("cannot extract value <" + std::string(type.name().begin(),type.name().end()) + "> from \"" + as_string(L, idx) + "\"").c_str());
      }
    }
    
    void push_function(lua_State * L,const lars::AnyFunction &f){
      create_and_push_object<lars::AnyFunction>(L) = f;
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

  const std::string &LuaGlue::get_key(const Extension *parent)const{
    auto it = keys.find(parent);
    if(it != keys.end()) return it->second;
    return keys.find(nullptr)->second;
  }
  
  void LuaGlue::connect_function(const Extension *parent,const std::string &name,const AnyFunction &f){
    LARS_LUA_GLUE_LOG("connecting function " << name);
    lua_glue::push_from_registry(state, get_key(parent));
    lua_glue::push_function(state,f);
    lua_setfield(state, -2, name.c_str());
    lua_pop(state, 1);
  }
  
  void LuaGlue::connect_extension(const Extension *parent,const std::string &name,const Extension &e){
    LARS_LUA_GLUE_LOG("connecting extension " << name);
    lua_glue::push_from_registry(state, get_key(parent));
    lua_newtable(state);
    //lua_pushvalue(state, -1);
    keys[&e] = lua_glue::add_to_registry(state);
    lua_setfield(state, -2, name.c_str());
    lua_pop(state, 1);
    e.connect(*this);
  }
  
}
