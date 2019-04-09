#pragma once

#include <lars/glue.h>

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>

struct lua_State;

namespace lars {
  
  class LuaGlue:public Glue{
    lua_State * L = nullptr;
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
    std::shared_ptr<LuaGlue> glue;
    
  public:
    
    struct Error: public std::exception {
      lua_State * L;
      unsigned stackSize;
      std::shared_ptr<bool> valid;
      
      Error(lua_State * l, unsigned stackSize);
      Error();

      const char * what() const noexcept override;
      ~Error();
    };
    
    LuaState();
    LuaState(lua_State * state);
    LuaState(const LuaState &other) = delete;

    void openLibs();
    
    LuaGlue &getGlue();
    
    void run(const std::string_view &code, const std::string &name = "anonymous lua code");
    lars::Any getValue(const std::string &value, lars::TypeIndex type, const std::string &name = "anonymous lua code");
    
    template <class T> T getValue(const std::string &value){
      return getValue(value, lars::get_type_index<typename std::remove_reference<typename std::remove_const<T>::type>::type>()).template get<T>();
    }

    void collectGarbage();
    void invalidate();
    unsigned stackSize();
    
    ~LuaState();
  };
  
}
