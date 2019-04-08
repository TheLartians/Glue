#pragma once

#include <lars/glue.h>

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>

struct lua_State;

namespace lars {
  
  class LuaGlue:public Glue{
    lua_State * state = nullptr;
    std::unordered_map<const Extension *, std::string> keys;
    const std::string & get_key(const Extension *parent)const;
    
  public:
  
    LuaGlue(lua_State * state);
    ~LuaGlue();
    
    void connect_function(const Extension *parent,const std::string &name,const AnyFunction &f)override;
    void connect_extension(const Extension *parent,const std::string &name,const Extension &e)override;
  };
 
  class LuaState {
  private:
    lua_State * L;
    bool owns_state;
    
  public:
    
    struct Error: public std::exception {
      lua_State * L;
      Error(lua_State * l):L(l){}
      const char * what() const noexcept override;
      ~Error();
    };
    
    LuaState();
    LuaState(lua_State * state);
    LuaState(const LuaState &other) = delete;

    void openLibs();
    
    LuaGlue getGlue();
    
    void run(const std::string_view &code, const std::string &name = "anonymous lua code");
    lars::Any runValue(const std::string_view &code, lars::TypeIndex type, const std::string &name = "anonymous lua code");
    
    template <class T> T runValue(const std::string_view &code, const std::string &name = "anonymous lua code"){
      return runValue(code, lars::get_type_index<typename std::remove_reference<typename std::remove_const<T>::type>::type>(), name);
    }
    
    void collectGarbage();
    
    ~LuaState();
  };
  
}
