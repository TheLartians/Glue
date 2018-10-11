#pragma once


#include <lua.hpp>
#include <lars/glue.h>

#include <string>
#include <unordered_map>

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
  
}
