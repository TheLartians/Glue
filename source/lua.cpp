
#include <glue/lua.h>
#include <lars/unused.h>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

//#define LARS_LUA_GLUE_DEBUG
//#include <lars/log.h>

#ifdef LARS_LUA_GLUE_DEBUG
#include <lars/log.h>
#define INCREASE_INDENT ____indent += "  "
#define DECREASE_INDENT ____indent.erase(____indent.end() - 2, ____indent.end())
namespace {
  std::string ____indent;
  struct Indenter {
    Indenter(){ INCREASE_INDENT; }
    ~Indenter(){ DECREASE_INDENT; }
  };
}
#define AUTO_INDENT Indenter indenter
#define LUA_GLUE_LOG(X) LARS_LOG("lua glue[" << (L ? lua_gettop(L) : -1) << "]: " << ____indent << X)
#else
#define LUA_GLUE_LOG(X)
#define INCREASE_INDENT
#define DECREASE_INDENT
#define AUTO_INDENT
#endif

namespace {
  namespace lua_glue{
    
    void push_value(lua_State * L,const lars::Any &value);
    
    template <class T> struct internal_type{
      lars::TypeIndex type;
      T data;
      template <typename ... Args> internal_type(const lars::TypeIndex &t, Args && ... args):type(t),data(std::forward<Args>(args)...){}
      operator T&(){ return data; }
    };
    
    std::string UNUSED as_string(lua_State * L,int idx = -1){
      lua_pushvalue(L, idx);
      auto res = luaL_tolstring(L,-1,nullptr);
      std::string result = res ? res : "undefined";
      lua_pop(L, 2);
      return result;
    }
    
#ifdef LARS_LUA_GLUE_DEBUG
    void UNUSED print_stack(lua_State * L){
      LUA_GLUE_LOG("Dumping stack...");
      for(int i=lua_gettop(L);i>=1;--i){
        LUA_GLUE_LOG("Stack at " << i << ": " << as_string(L,i));
      }
    }
#endif
    
    using RegistryKey = std::string;
    
    RegistryKey add_to_registry(lua_State * L,int idx = -1){
      static int obj_id = 0;
      obj_id++;
      auto key = "lars.glue.registry." + std::to_string(obj_id);
      LUA_GLUE_LOG("adding " << key << " to registry: " << as_string(L));
      lua_pushstring(L, key.c_str());
      lua_pushvalue(L, idx > 0 ? idx : idx - 1);
      lua_rawset(L, LUA_REGISTRYINDEX);
      return key;
    }
    
    RegistryKey add_to_registry(lua_State * L, const std::string &key,int idx = -1){
      lua_pushstring(L, key.c_str());
      lua_pushvalue(L, idx > 0 ? idx : idx - 1);
      LUA_GLUE_LOG("add " << key << " to registry: " << as_string(L));
      lua_rawset(L, LUA_REGISTRYINDEX);
      return key;
    }
    
    void push_from_registry(lua_State * L,const RegistryKey &key){
      lua_pushstring(L, key.c_str());
      lua_rawget(L, LUA_REGISTRYINDEX);
      LUA_GLUE_LOG("push " << key << " from registry: " << as_string(L));
    }
    
    void push_from_registry_i(lua_State * L, lua_Integer key){
      lua_rawgeti(L, LUA_REGISTRYINDEX, key);
      LUA_GLUE_LOG("push " << key << " from registry: " << as_string(L));
    }
    
    void remove_from_registry(lua_State * L,const RegistryKey &key){
#ifdef LARS_LUA_GLUE_DEBUG
      lua_pushstring(L, key.c_str());
      lua_rawget(L, LUA_REGISTRYINDEX);
      LUA_GLUE_LOG("remove " << key << " from registry: " << as_string(L));
      lua_pop(L, 1);
#endif
      lua_pushstring(L, key.c_str());
      lua_pushnil(L);
      lua_rawset(L, LUA_REGISTRYINDEX);
    }
    
    lua_State * get_main_thread(lua_State * L){
      push_from_registry_i(L, LUA_RIDX_MAINTHREAD); // push the main thread
      auto main_L = lua_tothread(L,-1); // get the main state
      lua_pop(L, 1); // pop the main thread object
      if(!main_L) throw std::runtime_error("glue: cannot get main thread object");
      return main_L;
    }
    
    std::shared_ptr<bool> get_lua_active_ptr(lua_State * L);
    
    struct RegistryObject: public lars::Visitable<RegistryObject>{
      lua_State * L;
      RegistryKey key;
      std::shared_ptr<bool> lua_active;
      RegistryObject(lua_State * l,const RegistryKey &k):L(get_main_thread(l)),key(k), lua_active(get_lua_active_ptr(L)){
        LUA_GLUE_LOG("create registry object: " << key << " with main context " << L);
      }
      RegistryObject(const RegistryObject &) = delete;
      RegistryObject(RegistryObject &&other):L(other.L),key(other.key),lua_active(other.lua_active){
        other.L = nullptr;
      }
      ~RegistryObject(){
        if(L && *lua_active){
          LUA_GLUE_LOG("delete RegistryObject(" << key << ")");
          remove_from_registry(L, key);
        }
      }
      void push_into(lua_State * L)const{ push_from_registry(L, key); }
    };
    
    struct Map: public lars::VirtualVisitable<lua_glue::RegistryObject, glue::Map> {
      explicit Map(lua_glue::RegistryObject &&o): lua_glue::RegistryObject(std::forward<lua_glue::RegistryObject>(o)){}
      glue::AnyReference getValue(const std::string &key) const final override;
      void setValue(const std::string &key, glue::Any && value) final override;
      std::vector<std::string> keys()const final override;
    };
    
    template <class T> internal_type<T> * get_internal_object_ptr(lua_State *L,int idx);
    template <class T> T * get_object_ptr(lua_State *L,int idx = -1);
    template <class T> T & get_object(lua_State *L,int idx = -1);
    void push_value(lua_State * L,const lars::Any &value);
    
    // WARNING: contains longjmp. No exception is thrown and destructors are not called.
    __attribute__ ((noreturn)) void throw_lua_error(lua_State * L, const std::string_view &err) {
      LUA_GLUE_LOG("internal error: " << err);
      lua_pushlstring(L, err.data(), err.length());
      lua_error(L);
      throw "internal glue error"; // should never throw as luaL_error does a longjmp.
    }
    
    __attribute__ ((noreturn)) void throw_lua_exception(lua_State * L, const std::string_view &err){
      lua_pushlstring(L, err.data(), err.size());
      throw glue::LuaState::Error(L, lua_gettop(L)-1);
    }
    
    __attribute__ ((noreturn)) void pop_and_throw_lua_exception(lua_State * L, const std::string_view &err){
      lua_pop(L, 1);
      throw_lua_exception(L, err);
    }
    
    // pushes the subclass table of the object at idx, if it exists
    bool getSubclassTable(lua_State * L, int idx, const lars::TypeIndex &type){
      //AUTO_INDENT;
      //LUA_GLUE_LOG("getSubclassTable");
      lua_getmetatable(L, idx); // push metatable
      auto t = lua_rawgeti(L, -1, type.hash() ); // push subclass table
      if(t == LUA_TNIL){
        //LUA_GLUE_LOG("unavailable");
        lua_pop(L, 2); // leave stack unchagned
        return false;
      }
      lua_copy(L, -1, -2); // replace metatable with subclass table
      lua_pop(L, 1); // pops the top
      return true;
    }
    
    // pushes the subclass field of the object at idx, if it exists
    bool getSubclassField(lua_State * L, int idx, const lars::TypeIndex &type, const char *name){
      //AUTO_INDENT;
      //LUA_GLUE_LOG("getSubclassField " << name);
      if(!getSubclassTable(L, idx, type)){ return false; } // push subclass table
      auto t = lua_getfield(L, -1, name); // get the field
      if(t == LUA_TNIL){ // field type is nil
                         //LUA_GLUE_LOG("unavailable");
        lua_pop(L, 2); // leave stack unchagned
        return false;
      }
      lua_copy(L, -1, -2); // replace metatable with subclass table
      lua_pop(L, 1); // pops the top
      return true;
    }
    
    // calls the subclass metamethod with arguments on the stack, if it exist
    bool forwardSubclassCall(lua_State * L, int idx, const lars::TypeIndex &type, const char *name, int argc, int retc = 1){
      //AUTO_INDENT;
      //LUA_GLUE_LOG("forwardSubclassCall " << name);
      if(!getSubclassField(L, idx, type, name)){ // push the method
                                                 //LUA_GLUE_LOG("unavailable");
        return false;
      }
      //LUA_GLUE_LOG("will call " << as_string(L));
      for(auto i = 0; i<argc; ++i){
        lua_pushvalue(L, -argc-1); // push the arguments
      }
      lua_call(L, argc, retc); // call the method. the result is now on top of the stack
      return true;
    }
    
    template <class T,const char *f, unsigned argc, unsigned resc> int forwardedClassMetamethod(lua_State * L){
      LUA_GLUE_LOG("calling forwarded metamethod");
      auto * data = get_internal_object_ptr<T>(L,1);
      if(!data) throw_lua_error(L, "glue error: corrupted internal pointer");
      if(forwardSubclassCall(L, -1, data->type, f, argc, resc)) return resc;
      throw_lua_error(L, "glue error: call undefined method " + std::string(f) + " on class object.");
    }
    
    namespace metamethd_names {
#define LARS_GLUE_ADD_FORWARDED_METAMETHOD(name) lua_pushcfunction(L, (forwardedClassMetamethod<T, metamethd_names::name, 2, 1>)); lua_setfield(L, -2, #name)
      char __eq[] = "__eq";
      char __lt[] = "__lt";
      char __le[] = "__le";
      char __gt[] = "__gt";
      char __ge[] = "__ge";
      char __mul[] = "__mul";
      char __div[] = "__div";
      char __idiv[] = "__idiv";
      char __add[] = "__add";
      char __sub[] = "__sub";
      char __mod[] = "__mod";
      char __pow[] = "__pow";
      char __unm[] = "__unm";
      char __index[] = "__index";
      char __newindex[] = "__newindex";
    }
    
    std::string class_mt_key(const lars::TypeIndex &idx){
      auto key = "lars.glue." + idx.name();
      return key;
    }
    
    template <class T> auto class_mt_key(){
      static auto key = class_mt_key(lars::getTypeIndex<T>());
      return key.c_str();
    }
    
    lars::Any extract_value(
                            lua_State * L,
                            int idx,
                            lars::TypeIndex type = lars::getTypeIndex<lars::Any>(),
                            __attribute__ ((noreturn)) void (*error_handler)(lua_State *, const std::string_view &) = throw_lua_error
                            );
    
    template <class T> void push_class_metatable(lua_State * L){
      static auto key = class_mt_key<T>();
      
      luaL_getmetatable(L, key);
      if(!lua_isnil(L, -1)) return;
      lua_pop(L, 1);
      
      LUA_GLUE_LOG("creating metatable for class " << lars::get_type_name<T>());
      
      luaL_newmetatable(L, key);
      
      lua_pushcfunction(L, +[](lua_State *L){
        auto * data = get_internal_object_ptr<T>(L,1);
        if(!data) throw_lua_error(L, "glue error: corrupted internal pointer");
        if(forwardSubclassCall(L, -1, data->type, "__tostring", 1, 1)){
          return 1;
        }
        auto type_name = data->type.name();
        lua_pushstring(L, (type_name + std::string("(") + std::to_string((size_t)&data->data) + ")").c_str());
        return 1;
      });
      lua_setfield(L, -2, "__tostring");
      
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__eq);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__lt);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__le);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__gt);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__ge);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__mul);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__div);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__idiv);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__add);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__sub);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__mod);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__pow);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__unm);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__index);
      LARS_GLUE_ADD_FORWARDED_METAMETHOD(__newindex);
      
      lua_pushcfunction(L, +[](lua_State *L){
        LUA_GLUE_LOG("calling class destructor");
        auto * data = get_object_ptr<T>(L,1);
        if(!data) throw_lua_error(L, "glue error: corrupted internal pointer");
        data->~T();
        return 0;
      });
      lua_setfield(L, -2, "__gc");
      
      if(lars::getTypeIndex<T>() == lars::getTypeIndex<lars::AnyFunction>()){
        lua_pushcfunction(L, +[](lua_State *L){
          try{
            LUA_GLUE_LOG("will call " << as_string(L,1) << " with " << lua_gettop(L)-1 << " arguments");
            INCREASE_INDENT;
            auto & f = get_object<lars::AnyFunction>(L,1);
            lars::AnyArguments args;
            auto argc = f.isVariadic() ? lua_gettop(L)-1 : f.argumentCount();
            for(size_t i=0;i<argc;++i) args.emplace_back(extract_value(L, i+2, f.argumentType(i)));
            INCREASE_INDENT;
            try{
              DECREASE_INDENT;
              DECREASE_INDENT;
              auto result = f.call(args);
              if(result){
                LUA_GLUE_LOG("returning type " << result.type().name());
                push_value(L, result);
                return 1;
              }
              LUA_GLUE_LOG("returning nothing");
              return 0;
            } catch (...) {
              throw;
            }
          }
          catch(std::exception &e){
            throw_lua_error(L, std::string("caught exception: ") + e.what());
          }
          catch(const char *s){
            throw_lua_error(L, std::string("caught exception: ") + s);
          }
          catch(...){
            throw_lua_error(L, "caught unknown exception");
          }
        });
        lua_setfield(L, -2, "__call");
      }
      if constexpr (std::is_same<T, std::shared_ptr<glue::Map>>::value) {
        lua_pushcfunction(L, +[](lua_State *L){
          std::string key = as_string(L,2);
          LUA_GLUE_LOG("indexing glue::Map." << key);
          auto ptr = get_internal_object_ptr<std::shared_ptr<glue::Map>>(L,1);
          if (!ptr) throw_lua_error(L, "glue error: corrupted internal pointer");
          push_value(L, ptr->data->getValue(key));
          return 1;
        });
        lua_setfield(L, -2, "__index");
        
        lua_pushcfunction(L, +[](lua_State *L){
          std::string key = as_string(L,2);
          LUA_GLUE_LOG("setting glue::Map." << key);
          auto ptr = get_internal_object_ptr<std::shared_ptr<glue::Map>>(L,1);
          if (!ptr) throw_lua_error(L, "glue error: corrupted internal pointer");
          ptr->data->setValue(key, RegistryObject(L, add_to_registry(L,3)));
          return 0;
        });
        lua_setfield(L, -2, "__newindex");
        
      } else {
        lua_pushcfunction(L, +[](lua_State *L){
          LUA_GLUE_LOG("indexing: " << as_string(L,1) << "." << as_string(L,2));
          lua_getmetatable(L,1);
          auto ptr = get_internal_object_ptr<T>(L,1);
          if (!ptr) throw_lua_error(L, "glue error: corrupted internal pointer");
          lua_rawgeti(L, -1, ptr->type.hash() );
          if(lua_isnil(L, -1)){
            LUA_GLUE_LOG("no class table exists for  " << ptr->type.name() << "(" << ptr->type.hash() << ")");
            return 1;
          }
          lua_pushvalue(L, 2);
          lua_gettable(L,-2);
          return 1;
        });
        lua_setfield(L, -2, "__index");
      }
      
      LUA_GLUE_LOG("completed metatable: " << as_string(L));
      return;
    }
    
    // sets metatable for subclass. leaves stack unchanged.
    template <class T> void set_subclass_table(lua_State * L,const lars::TypeIndex &type){
      LUA_GLUE_LOG("setting subclass table for " << type.name() << "(" << type.hash() << ")" << ": " << as_string(L));
      push_class_metatable<T>(L);
      lua_pushvalue(L, -2);
      lua_rawseti(L, -2, type.hash());
      lua_pop(L, 1);
    }
    
    template <class T> size_t buffer_size_for_type(){
      return sizeof(internal_type<T>) + alignof(T) - 1;
    }
    
    template <class T> T * align_buffer_pointer(void * ptr){
      auto size = buffer_size_for_type<T>();
      if (std::align(alignof(T), sizeof(T), ptr, size)){
        T * result = reinterpret_cast<T*>(ptr);
        return result;
      }
      throw std::runtime_error("glue: internal pointer alignment error");
    }
    
    // does not lua_error
    template <class T,class ... Args> T & create_and_push_object(lua_State * L,lars::TypeIndex type_index,Args && ... args){
      LUA_GLUE_LOG("creating object of type " << type_index.name());
      auto ptr = lua_newuserdata(L,buffer_size_for_type<internal_type<T>>());
      if(!ptr) throw std::runtime_error("glue: cannot allocate memory");
      auto data = align_buffer_pointer<internal_type<T>>(ptr);
      new(data) internal_type<T>(type_index,std::forward<Args>(args)...);
      LUA_GLUE_LOG("before setting metatable: " << as_string(L));
      push_class_metatable<T>(L);
      lua_setmetatable(L, -2);
      LUA_GLUE_LOG("after setting metatable: " << as_string(L));
      return *data;
    }
    
    template <class T> internal_type<T> * get_internal_object_ptr(lua_State *L,int idx){
      // LUA_GLUE_LOG("getting object pointer for " << lars::get_type_name<T>());
      auto * ptr = luaL_testudata(L,idx,class_mt_key<T>());
      if(!ptr){
        // LUA_GLUE_LOG("cannot get internal pointer of type " << lars::get_type_name<T>() << ": incorrect type");
        return nullptr;
      }
      return align_buffer_pointer<internal_type<T>>(ptr);
    }
    
    template <class T> T * get_object_ptr(lua_State *L,int idx){
      internal_type<T> * data = get_internal_object_ptr<T>(L,idx);
      return data ? &data->data : nullptr;
    }
    
    template <class T> T & get_object(lua_State *L,int idx){
      auto * data = get_object_ptr<T>(L,idx);
      if(!data){
        throw_lua_error(L,("cannot extract c++ object of type " + lars::get_type_name<T>() + " from " + as_string(L,idx)).c_str());
      }
      return *data;
    }
    
    void push_function(lua_State * L,const lars::AnyFunction &f);
    
    void push_value(lua_State * L,const lars::Any &value){
      using namespace glue;
      
      if(!value){
        LUA_GLUE_LOG("push nil");
        return lua_pushnil(L);
      }
      
      struct PushVisitor:public lars::RecursiveVisitor<
      bool,
      const char &,
      const int &,
      double,
      const std::string &,
      const lars::AnyFunction &,
      const RegistryObject &,
      const glue::Map &,
      const glue::ElementMap &
      >{
        lua_State * L;
        bool visit(bool data)override{ LUA_GLUE_LOG("push bool"); lua_pushboolean(L, data); return true; }
        bool visit(const int &data)override{ LUA_GLUE_LOG("push int"); lua_pushinteger(L, data); return true; }
        bool visit(const char &data)override{ LUA_GLUE_LOG("push int"); lua_pushinteger(L, data); return true; }
        bool visit(double data)override{ LUA_GLUE_LOG("push char"); lua_pushnumber(L, data); return true; }
        bool visit(const std::string &data)override{ LUA_GLUE_LOG("push string"); lua_pushstring(L, data.c_str()); return true; }
        bool visit(const lars::AnyFunction &data)override{ LUA_GLUE_LOG("push function"); push_function(L,data); return true; }
        bool visit(const RegistryObject &data)override{
          LUA_GLUE_LOG("push registry from " << data.L << " into " << L);
          data.push_into(L);
          return true;
        }
        bool visit(const ElementMap &data)override{
          for (auto& type: data.classes) {
            LUA_GLUE_LOG("add class: " << type.first);
            AUTO_INDENT;
            create_and_push_object<std::shared_ptr<glue::Map>>(L,lars::getTypeIndex<glue::Map>(),type.second);
            set_subclass_table<glue::Any>(L,type.first);
            lua_pop(L, 1);
          }
          visit((const glue::Map &)data);
          auto observer = data.onClassAdded.createObserver([L=L](auto &type, auto &map){
            LUA_GLUE_LOG("add class from observer: " << type);
            create_and_push_object<std::shared_ptr<glue::Map>>(L,lars::getTypeIndex<glue::Map>(),map);
            set_subclass_table<glue::Any>(L,type);
          });
          create_and_push_object<decltype(observer)>(L, lars::getTypeIndex<decltype(observer)>(), std::move(observer));
          lua_setfield(L, -2, "__type_observer");
          return true;
        }
        bool visit(const glue::Map &data)override{
          LUA_GLUE_LOG("push map");
          std::shared_ptr<glue::Map> sharedMap = const_cast<glue::Map &>(data).shared_from_this();
          create_and_push_object<std::shared_ptr<glue::Map>>(L,lars::getTypeIndex<glue::Map>(),sharedMap);
          return true;
        }
      } visitor;
      
      visitor.L = L;
      if(!value.accept(visitor)){
        create_and_push_object<lars::Any>(L,value.type(),lars::AnyReference(value));
        LUA_GLUE_LOG("pushed any: " << value.type().name());
      }
    }
    
    lars::Any extract_value(
                            lua_State * L,
                            int idx = -1,
                            lars::TypeIndex type,
                            __attribute__ ((noreturn)) void (*error_handler)(lua_State *, const std::string_view &)
                            ){
      LUA_GLUE_LOG("extract " << type.name() << " from " << as_string(L,idx));
      
      auto assert_value_exists = [&](auto && v){
        if(!v){
          error_handler(L, "invalid argument type for argument " + std::to_string(idx-1) + ". Expected " + type.name() + ", got " + as_string(L,idx));
        }
        return v;
      };
      
      auto luaType = lua_type(L, idx);
      
      if (luaType == LUA_TUSERDATA){
        if(auto ptr = get_object_ptr<lars::Any>(L,idx)){
          if(ptr->type() == type || type == lars::getTypeIndex<lars::Any>()){
            LUA_GLUE_LOG("extracted any<" << ptr->type().name() << ">");
            return lars::AnyReference(*ptr);
          }
          else{
            LUA_GLUE_LOG("unsafe extracted any<" << ptr->type().name() << ">");
            return lars::AnyReference(*ptr);
          }
        }
      }
      
      if(type == lars::getTypeIndex<lars::Any>()){
        switch (luaType) {
          case LUA_TNUMBER: type = lars::getTypeIndex<LUA_NUMBER>(); break;
          case LUA_TSTRING: type = lars::getTypeIndex<std::string>(); break;
          case LUA_TBOOLEAN: type = lars::getTypeIndex<bool>(); break;
          case LUA_TFUNCTION: type = lars::getTypeIndex<lars::AnyFunction>(); break;
          case LUA_TTABLE: type = lars::getTypeIndex<glue::Map>(); break;
          case LUA_TNONE: return lars::Any();
          case LUA_TNIL: return lars::Any();
          case LUA_TUSERDATA: {
            if(auto ptr = get_object_ptr<std::shared_ptr<glue::Map>>(L,idx)){
              LUA_GLUE_LOG("extracted map");
              return lars::Any(*ptr);
            }
            if(auto ptr = get_object_ptr<lars::AnyFunction>(L,idx)){
              LUA_GLUE_LOG("extracted lars::AnyFunction");
              return lars::makeAny<lars::AnyFunction>(*ptr);
            }
          }
          default: break;
        }
      }
      
      if(type == lars::getTypeIndex<std::string>()){ return lars::makeAny<std::string>(assert_value_exists(lua_tostring(L, idx))); }
      else if(type == lars::getTypeIndex<double>()){ return lars::makeAny<double>((lua_tonumber(L, idx))); }
      else if(type == lars::getTypeIndex<float>()){ return lars::makeAny<float>((lua_tonumber(L, idx))); }
      else if(type == lars::getTypeIndex<char>()){ return lars::makeAny<char>((lua_tonumber(L, idx))); }
      else if(type == lars::getTypeIndex<int>()){ return lars::makeAny<int>((lua_tointeger(L, idx))); }
      else if(type == lars::getTypeIndex<unsigned>()){ return lars::makeAny<int>((lua_tointeger(L, idx))); }
      else if(type == lars::getTypeIndex<bool>()){ return lars::makeAny<bool>((lua_toboolean(L, idx))); }
      else if(type == lars::getTypeIndex<lars::AnyFunction>()){
        
        if(auto ptr = get_object_ptr<lars::AnyFunction>(L,idx)){
          LUA_GLUE_LOG("extracted lars::AnyFunction");
          return lars::makeAny<lars::AnyFunction>(*ptr);
        }
        
        LUA_GLUE_LOG("captured function " << as_string(L,idx) << " with context " << L);
        auto captured = std::make_shared<RegistryObject>(L,add_to_registry(L,idx));
        
        lars::AnyFunction f = [captured](const lars::AnyArguments &args){
          if (!*captured->lua_active) {
            throw std::runtime_error("calling function with invalid lua context");
          }
          lua_State * L = captured->L;
          LUA_GLUE_LOG("using captured context: " << L);
          auto status = lua_status(L);
          if(status != LUA_OK){
            LUA_GLUE_LOG("calling function with invalid status: " << status);
            throw std::runtime_error("glue: calling function with invalid lua status");
          }
          captured->push_into(L);
          LUA_GLUE_LOG("calling registry " << captured->key << ": " << as_string(L) << " with " << args.size() << " arguments");
          for(auto && arg:args) push_value(L, arg);
          // dummy continuation k to allow yielding
          auto k = +[](lua_State*, int , lua_KContext ){
            return 0;
          };
          INCREASE_INDENT;
          status = lua_pcallk(L, static_cast<int>(args.size()), 1, 0, 0, k);
          DECREASE_INDENT;
          if(status == LUA_OK){
            LUA_GLUE_LOG("return " << as_string(L));
            if(lua_isnil(L, -1)){
              lua_pop(L, 1);
              return lars::Any();
            }
            auto result = extract_value(L, -1, lars::getTypeIndex<lars::Any>());
            lua_pop(L, 1);
            return result;
          } else{
            LUA_GLUE_LOG("error: " << status);
            if(status == LUA_ERRRUN){
              throw std::runtime_error("glue: lua runtime error in callback: " + std::string(lua_tostring(L,-1)));
            } else {
              throw std::runtime_error("glue: lua error in callback: " + std::to_string(status));
            }
          }
        };
        return lars::makeAny<lars::AnyFunction>(f);
      } else if (type == lars::getTypeIndex<glue::Map>()) {
        return lars::makeAny<Map>(RegistryObject(L,add_to_registry(L,idx)));
      } else{
        auto msg = "cannot extract type '" + type.name() + "' from \"" + as_string(L, idx) + "\"";
        LUA_GLUE_LOG(msg);
        error_handler(L, msg);
      }
    }
    
    void push_function(lua_State * L,const lars::AnyFunction &f){
      create_and_push_object<lars::AnyFunction>(L, lars::getTypeIndex<internal_type<lars::AnyFunction>>(),f);
    }
    
    const std::string LUA_VALID_REGISTRY_KEY = "LARS_GLUE_LUA_IS_VALID";
    
    struct LuaActivePtrOwner{
      std::shared_ptr<bool> value;
      LuaActivePtrOwner():value(std::make_shared<bool>(true)){}
      LuaActivePtrOwner(const LuaActivePtrOwner &other) = delete;
      ~LuaActivePtrOwner(){ *value = false; }
    };
    
    std::shared_ptr<bool> get_lua_active_ptr(lua_State * L){
      LUA_GLUE_LOG("get lua active ptr");
      INCREASE_INDENT;
      push_from_registry(L, LUA_VALID_REGISTRY_KEY);
      if (!lua_isnil(L, -1)) {
        auto &result = get_object<LuaActivePtrOwner>(L);
        lua_pop(L, 1);
        DECREASE_INDENT;
        return result.value;
      }
      LUA_GLUE_LOG("create lua active ptr");
      lua_pop(L, 1);
      lars::Any value;
      auto &result = create_and_push_object<LuaActivePtrOwner>(L, lars::getTypeIndex<LuaActivePtrOwner>());
      add_to_registry(L, LUA_VALID_REGISTRY_KEY);
      lua_pop(L, 1);
      DECREASE_INDENT;
      return result.value;
    }
  }
  
}

glue::AnyReference lua_glue::Map::getValue(const std::string &key) const {
  push_into(L);
  lua_getfield(L, -1, key.c_str());
  auto value = lua_glue::extract_value(L);
  lua_pop(L, 2);
  return value;
}

void lua_glue::Map::setValue(const std::string &key, glue::Any && value) {
  push_into(L);
  lua_glue::push_value(L, value);
  lua_setfield(L, -2, key.c_str());
  lua_pop(L, 1);
}

std::vector<std::string> lua_glue::Map::keys() const {
  std::vector<std::string> result;
  push_into(L);
  if (!lua_istable(L, -1)) { // something went wrong
    lua_pop(L, 1);
    return result;
  }
  lua_pushnil(L);
  while (lua_next(L, -2)) {
    // stack now contains: -1 => value; -2 => key; -3 => table
    lua_pop(L, 1);
    if (lua_isstring(L, -1)) {
      result.emplace_back(lua_glue::as_string(L, -1));
    }
  }
  lua_pop(L, 1);
  return result;
}

namespace glue {
  
  
  LuaState::Error::Error(lua_State * l, unsigned s):L(l), stackSize(s), valid(lua_glue::get_lua_active_ptr(L)){
    LUA_GLUE_LOG("error: " << lua_glue::as_string(L));
  }
  
  LuaState::Error::Error():L(nullptr){
    
  }
  
  const char * LuaState::Error::what() const noexcept {
    if (L && *valid) {
      auto str = lua_tostring(L,-1);
      if (str) {
        return str;
      } else {
        return "unknown lua error";
      }
    } else {
      return "unknown lua error (lost lua context)";
    }
  }
  
  LuaState::Error::~Error(){
    if(L && *valid){
      lua_settop(L, stackSize);
    }
  }
  
  std::shared_ptr<Map> getGlobalMap(lua_State *L){
    lua_pushglobaltable(L);
    lua_glue::RegistryObject global(L, lua_glue::add_to_registry(L));
    lua_pop(L, 1);
    return std::make_shared<lua_glue::Map>(std::move(global));
  }
  
  LuaState::LuaState(lua_State * s): L(s), ownsState(false), globalTable(getGlobalMap(L)){
  }
  
  LuaState::LuaState(): L(luaL_newstate()), ownsState(true), globalTable(getGlobalMap(L)){
  }
  
  LuaState::~LuaState(){
    if (ownsState) {
      LUA_GLUE_LOG("destroying lua");
      INCREASE_INDENT;
      lua_close(L);
      DECREASE_INDENT;
      L = nullptr;
      LUA_GLUE_LOG("lua destroyed");
    }
  }
  
  void LuaState::openStandardLibs()const{
    luaL_openlibs(L);
  }
  
  lars::Any LuaState::run(const std::string_view &code, const std::string &name)const{
    LUA_GLUE_LOG("running code: " << code);
    
    auto N = lua_gettop(L);
    
    INCREASE_INDENT;
    if (luaL_loadbuffer(L, code.data(), code.size(), name.c_str())) {
      throw Error(L, N);
    }
    DECREASE_INDENT;
    
    if(lua_pcall(L, 0, LUA_MULTRET, 0)) {
      throw Error(L, N);
    }
    
    lars::Any result;
    if (lua_gettop(L) > 0) {
      result = lua_glue::extract_value(L, -1, lars::getTypeIndex<lars::Any>(), lua_glue::pop_and_throw_lua_exception);
    }
    
    LUA_GLUE_LOG("finished running code");
    lua_settop(L, N);
    return result;
  }
  
  lars::Any LuaState::runFile(const std::string &path)const{
    LUA_GLUE_LOG("running code: " << code);
    
    auto N = lua_gettop(L);
    
    if(luaL_dofile(L, path.c_str())) {
      throw Error(L, N);
    }
    
    lars::Any result;
    if (lua_gettop(L) > 0) {
      result = lua_glue::extract_value(L, -1, lars::getTypeIndex<lars::Any>(), lua_glue::pop_and_throw_lua_exception);
    }
    
    LUA_GLUE_LOG("finished running code");
    lua_settop(L, N);
    return result;
  }
  
  void LuaState::set(const std::string &name, const lars::AnyReference &value)const{
    lua_glue::push_value(L, value);
    lua_setglobal(L, name.c_str());
  }
  
  void LuaState::collectGarbage()const{
    lua_gc(L, LUA_GCCOLLECT, 0);
  }
  
  ElementMapEntry LuaState::operator[](const std::string &key)const{
    return (*globalTable)[key];
  }
}
