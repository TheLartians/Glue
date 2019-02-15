#include <lars/lua.h>

#include <lars/log.h>

#include <lua.hpp>

// #define LARS_LUA_GLUE_DEBUG

#ifdef LARS_LUA_GLUE_DEBUG
#define LARS_LUA_GLUE_LOG(X) LARS_LOG_WITH_PROMPT(X,"lua glue: ")
#else
#define LARS_LUA_GLUE_LOG(X)
#endif

namespace {
  namespace lua_glue{
    
    template <class T> struct internal_type{
      lars::TypeIndex type;
      T data;
      template <typename ... Args> internal_type(const lars::TypeIndex &t, Args...args):type(t),data(args...){}
      operator T&(){ return data; }
    };

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
      auto key = "lars.glue.registry." + std::to_string(obj_id);
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
    
    void push_from_registry_i(lua_State * L, lua_Integer key){
      lua_rawgeti(L, LUA_REGISTRYINDEX, key);
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
    
    lua_State * get_main_thread(lua_State * L){
      push_from_registry_i(L, LUA_RIDX_MAINTHREAD); // push the main thread
      auto main_L = lua_tothread(L,-1); // get the main state
      lua_pop(L, 1); // pop the main thread object
      if(!main_L) throw std::runtime_error("glue: cannot get main thread object");
      return main_L;
    }
    
    struct RegistryObject{
      lua_State * L;
      RegistryKey key;
      RegistryObject(lua_State * l,const RegistryKey &k):L(get_main_thread(l)),key(k){ LARS_LUA_GLUE_LOG("create registry object: " << key); }
      RegistryObject(const RegistryObject &) = delete;
      RegistryObject(RegistryObject &&other):L(other.L),key(other.key){ other.L = nullptr; }
      ~RegistryObject(){ if(L){ LARS_LUA_GLUE_LOG("delete RegistryObject(" << key << ")"); remove_from_registry(L, key); } }
      void push(lua_State * L)const{ push_from_registry(L, key); }
    };
    
    template <class T> internal_type<T> * get_internal_object_ptr(lua_State *L,int idx);
    template <class T> T * get_object_ptr(lua_State *L,int idx = -1);
    template <class T> T & get_object(lua_State *L,int idx = -1);
    lars::Any extract_value(lua_State * L,int idx,lars::TypeIndex type);
    void push_value(lua_State * L,const lars::Any &value);
    
    // WARNING: contains longjmp. No exception is thrown and destructors are not called.
    __attribute__ ((noreturn)) void throw_lua_error(lua_State * L, const std::string &err) {
      luaL_error(L, ("glue: " + err).c_str());
      throw "internal glue error"; // should never throw as luaL_error does a longjmp.
    }
    
    // pushes the subclass table of the object at idx, if it exists
    bool getSubclassTable(lua_State * L, int idx, const lars::TypeIndex &type){
      lua_getmetatable(L, idx); // push metatable
      auto t = lua_rawgeti(L, -1, type.hash() ); // push subclass table
      if(t == LUA_TNIL){
        lua_pop(L, 2); // leave stack unchagned
        return false;
      }
      lua_copy(L, -1, -2); // replace metatable with subclass table
      lua_pop(L, 1); // pops the top
      return true;
    }
    
    // pushes the subclass field of the object at idx, if it exists
    bool getSubclassField(lua_State * L, int idx, const lars::TypeIndex &type, const std::string &name){
      if(!getSubclassTable(L, idx, type)){ return false; } // push subclass table
      auto t = lua_getfield(L, -1, name.c_str()); // get the fiels
      if(t == LUA_TNIL){ // field type is nil
        lua_pop(L, 2); // leave stack unchagned
        return false;
      }
      lua_copy(L, -1, -2); // replace metatable with subclass table
      lua_pop(L, 1); // pops the top
      return true;
    }
    
    // calls the subclass metamethod with arguments on the stack, if it exist
    bool forwardSubclassCall(lua_State * L, int idx, const lars::TypeIndex &type, const std::string &name, int argc, int retc = 1){
      if(!getSubclassField(L, idx, type, name)) return false; // push the method
      for(auto i = 0; i<argc; ++i){
        lua_pushvalue(L, -argc-1); // push the arguments
      }
      lua_call(L, argc, retc); // call the method. the result is now on top of the stack
      return true;
    }
    
    template <class T,const char *f, unsigned argc, unsigned resc> int forwardedClassMetamethod(lua_State * L){
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
    }

    std::string class_mt_key(const lars::TypeIndex &idx){
      auto name = idx.name();
      auto key = "lars.glue." + std::string(name.begin(),name.end());
      return key;
    }
    
    template <class T> auto class_mt_key(){
      static auto key = class_mt_key(lars::get_type_index<T>());
      return key.c_str();
    }
    
    template <class T> void push_class_metatable(lua_State * L){
      static auto key = class_mt_key<T>();
      
      luaL_getmetatable(L, key);
      if(!lua_isnil(L, -1)) return;
      lua_pop(L, 1);

      LARS_LUA_GLUE_LOG("creating metatable for class " << lars::get_type_name<T>());
      
      luaL_newmetatable(L, key);
      
      lua_pushcfunction(L, +[](lua_State *L){
        auto * data = get_internal_object_ptr<T>(L,1);
        if(!data) throw_lua_error(L, "glue error: corrupted internal pointer");
        if(forwardSubclassCall(L, -1, data->type, "__tostring", 1, 1)) return 1;
        auto type_name = std::string(data->type.name().begin(),data->type.name().end());
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

      lua_pushcfunction(L, +[](lua_State *L){
        auto * data = get_object_ptr<T>(L,1);
        if(!data) throw_lua_error(L, "glue error: corrupted internal pointer");
        data->~T();
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
              LARS_LUA_GLUE_LOG("returning type " << result.type().name());
              push_value(L, result);
              return 1;
            }
            LARS_LUA_GLUE_LOG("returning nothing");
            return 0;
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
      
      lua_pushcfunction(L, +[](lua_State *L){
        LARS_LUA_GLUE_LOG("indexing: " << as_string(L,1) << "." << as_string(L,2));
        lua_getmetatable(L,1);
        lua_rawgeti(L, -1, get_internal_object_ptr<T>(L,1)->type.hash() );
        if(lua_isnil(L, -1)){
          LARS_LUA_GLUE_LOG("no class table exists");
          return 1;
        }
        lua_pushvalue(L, 2);
        lua_gettable(L,-2);
        return 1;
      });
      lua_setfield(L, -2, "__index");
    
      LARS_LUA_GLUE_LOG("completed metatable: " << as_string(L));
      return;
    }
    
    // sets metatable for subclass. leaves stack unchanged.
    template <class T> void set_subclass_metatable(lua_State * L,const lars::TypeIndex &type){
      push_class_metatable<T>(L);
      lua_pushvalue(L, -2);
      lua_rawseti(L, -2, type.hash());
      lua_pop(L, 1);
    }
    
    template <class T> void push_subclass_metatable(lua_State * L,const lars::TypeIndex &type){
      push_class_metatable<T>(L); // get parent metatable
      lua_rawgeti(L, -1, type.hash()); // get base metatable if it exists
      if(lua_isnil(L, -1)){
        lua_pop(L, 1); // pop nil
        lua_newtable(L); // create subclass metatable
        lua_rawseti(L, -2, type.hash()); // assign base metatble to parent;
        lua_rawgeti(L, -1, type.hash()); // get base metatable
      }
      lua_insert(L,-2); // set base metatable at -2
      lua_pop(L, 1); // pop current and parent metatable
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
    template <class T,class ... Args> T & create_and_push_object(lua_State * L,lars::TypeIndex type_index,Args ... args){
      LARS_LUA_GLUE_LOG("creating object of type " << type_index.name());
      auto ptr = lua_newuserdata(L,buffer_size_for_type<internal_type<T>>());
      if(!ptr) throw std::runtime_error("glue: cannot allocate memory");
      auto data = align_buffer_pointer<internal_type<T>>(ptr);
      new(data) internal_type<T>(type_index,args...);
      LARS_LUA_GLUE_LOG("before setting metatable: " << as_string(L));
      push_class_metatable<T>(L);
      lua_setmetatable(L, -2);
      LARS_LUA_GLUE_LOG("after setting metatable: " << as_string(L));
      return *data;
    }
    
    template <class T> internal_type<T> * get_internal_object_ptr(lua_State *L,int idx){
      // LARS_LUA_GLUE_LOG("getting object pointer for " << lars::get_type_name<T>());
      auto * ptr = luaL_testudata(L,idx,class_mt_key<T>());
      if(!ptr){
        LARS_LUA_GLUE_LOG("cannot get internal pointer of type " << lars::get_type_name<T>() << ": incorrect type");
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
      using namespace lars;
      
      if(!value){
        LARS_LUA_GLUE_LOG("push nil");
        return lua_pushnil(L);
      }
      
      struct PushVisitor:public ConstVisitor<lars::VisitableType<double>,lars::VisitableType<char>,lars::VisitableType<float>,lars::VisitableType<std::string>,lars::VisitableType<lars::AnyFunction>,lars::VisitableType<int>,lars::VisitableType<bool>,lars::VisitableType<unsigned>,lars::VisitableType<RegistryObject>>{
        lua_State * L;
        bool push_any = false;
        void visit_default(const lars::VisitableBase &data)override{ LARS_LUA_GLUE_LOG("push any"); push_any = true; }
        void visit(const lars::VisitableType<bool> &data)override{ LARS_LUA_GLUE_LOG("push bool"); lua_pushboolean(L, data.data); }
        void visit(const lars::VisitableType<int> &data)override{ LARS_LUA_GLUE_LOG("push int"); lua_pushinteger(L, data.data); }
        void visit(const lars::VisitableType<char> &data)override{ LARS_LUA_GLUE_LOG("push char"); lua_pushnumber(L, data.data); }
        void visit(const lars::VisitableType<float> &data)override{ LARS_LUA_GLUE_LOG("push float"); lua_pushnumber(L, data.data); }
        void visit(const lars::VisitableType<double> &data)override{ LARS_LUA_GLUE_LOG("push double"); lua_pushnumber(L, data.data); }
        void visit(const lars::VisitableType<unsigned> &data)override{ LARS_LUA_GLUE_LOG("push double"); lua_pushnumber(L, data.data); }
        void visit(const lars::VisitableType<std::string> &data)override{ LARS_LUA_GLUE_LOG("push string"); lua_pushstring(L, data.data.c_str()); }
        void visit(const lars::VisitableType<lars::AnyFunction> &data)override{ LARS_LUA_GLUE_LOG("push function"); push_function(L,data.data); }
        void visit(const lars::VisitableType<RegistryObject> &data)override{ LARS_LUA_GLUE_LOG("push object"); data.data.push(L); }
      } visitor;
      
      visitor.L = L;
      value.accept_visitor(visitor);
      if(visitor.push_any){
        create_and_push_object<lars::Any>(L,value.type(),value);
        LARS_LUA_GLUE_LOG("pushed any: " << value.type().name());
      }
    }
    
    lars::Any extract_value(lua_State * L,int idx,lars::TypeIndex type){
      LARS_LUA_GLUE_LOG("extract " << type.name() << " from " << as_string(L,idx));
      auto assert_value_exists = [&](auto && v){
        if(!v){
          throw std::runtime_error("invalid argument type for argument " + std::to_string(idx-1) + ". Expected " + std::string(type.name().begin(),type.name().end()) + ", got " + as_string(L,idx));
        }
        return v;
      };
      
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
      else if(type == lars::get_type_index<double>()){ return lars::make_any<double>((lua_tonumber(L, idx))); }
      else if(type == lars::get_type_index<float>()){ return lars::make_any<float>((lua_tonumber(L, idx))); }
      else if(type == lars::get_type_index<char>()){ return lars::make_any<char>((lua_tonumber(L, idx))); }
      else if(type == lars::get_type_index<int>()){ return lars::make_any<int>((lua_tointeger(L, idx))); }
      else if(type == lars::get_type_index<unsigned>()){ return lars::make_any<int>((lua_tointeger(L, idx))); }
      else if(type == lars::get_type_index<bool>()){ return lars::make_any<bool>((lua_toboolean(L, idx))); }
      else if(type == lars::get_type_index<RegistryObject>()){ return lars::make_any<RegistryObject>(L,add_to_registry(L,idx)); }
      else if(type == lars::get_type_index<lars::AnyFunction>()){
        if(auto ptr = get_object_ptr<lars::AnyFunction>(L,idx)){
          LARS_LUA_GLUE_LOG("extracted lars::AnyFunction");
          return lars::make_any<lars::AnyFunction>(*ptr);
        }
        
        auto captured = std::make_shared<RegistryObject>(L,add_to_registry(L,idx));
        
        lars::AnyFunction f = [captured](lars::AnyArguments &args){
          lua_State * L = captured->L;
          auto status = lua_status(L);
          if(status != LUA_OK){
            LARS_LUA_GLUE_LOG("calling function with invalid status: " << status);
            throw std::runtime_error("glue: calling function with invalid lua status");
          }
          captured->push(L);
          LARS_LUA_GLUE_LOG("calling registry " << captured->key << " with " << args.size() << " arguments: " << as_string(L, -(args.size()+1)));
          for(auto && arg:args) push_value(L, arg);
          // dummy continuation k to allow yielding
          auto k = +[](lua_State*L, int status, lua_KContext ctx){ return 0; };
          status = lua_pcallk(L, static_cast<int>(args.size()), 1, 1, 0, k);
          if(status == LUA_OK){
            LARS_LUA_GLUE_LOG("return " << as_string(L));
            if(lua_isnil(L, -1)){
              lua_pop(L, 1);
              return lars::Any();
            }
            auto result = extract_value(L, -1, lars::get_type_index<lars::Any>());
            lua_pop(L, 1);
            return result;
          } else if(status == LUA_ERRRUN){
            throw std::runtime_error("glue: lua runtime error in callback: " + std::string(lua_tostring(L,-1)));
          } else {
            throw std::runtime_error("glue: lua error in callback: " + std::to_string(status));
          }
        };
        return lars::make_any<lars::AnyFunction>(f);
      }
      else{
        throw std::runtime_error("cannot extract type '" + std::string(type.name().begin(),type.name().end()) + "' from \"" + as_string(L, idx) + "\"");
      }
    }
    
    void push_function(lua_State * L,const lars::AnyFunction &f){
      create_and_push_object<lars::AnyFunction>(L, lars::get_type_index<internal_type<lars::AnyFunction>>(),f);
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
    LARS_LUA_GLUE_LOG("connecting function " << name << " with parent " << &parent);
    lua_glue::push_from_registry(state, get_key(parent));
    lua_glue::push_function(state,f);
    lua_setfield(state, -2, name.c_str());
    lua_pop(state, 1);
    LARS_LUA_GLUE_LOG("finished connecting function " << name << " with parent " << &parent);
  }
  
  void LuaGlue::connect_extension(const Extension *parent,const std::string &name,const Extension &e){
    LARS_LUA_GLUE_LOG("connecting extension " << name << " with parent " << &parent);
    
    lua_newtable(state); // create and push extension table
    lua_glue::push_from_registry(state, get_key(parent)); // push parent
    lua_pushvalue(state, -2); // push extension table again
    keys[&e] = lua_glue::add_to_registry(state); // store extension table in registry
    lua_setfield(state, -2, name.c_str()); // assign extension to parent
    lua_pop(state, 1); // pop parent
    
    // extension table at top.
    
    if(e.class_type()){
      if(e.base_class_type()){
        lua_newtable(state); // create class metatable
        lua_pushstring(state, "__index"); // push __index
        lua_glue::push_subclass_metatable<Any>(state, *e.base_class_type()); // push base class table
        lua_rawset(state, -3); // set base class table as __index
        // metatable at top
        lua_setmetatable(state, -2); // set as metatable for extension table
      }
      // extension table at top
      lua_glue::set_subclass_metatable<Any>(state, *e.class_type());
    }

    // extension table at top

    if(e.shared_class_type()){
      lua_glue::push_from_registry(state, get_key(&e));
      lua_glue::set_subclass_metatable<Any>(state, *e.shared_class_type());
      lua_pop(state, 1);
    }
    
    e.connect(*this);
    lua_pop(state, 1); // pop extension table
    
    LARS_LUA_GLUE_LOG("finished connecting extension " << name << " with parent " << &parent);
  }
  
}
