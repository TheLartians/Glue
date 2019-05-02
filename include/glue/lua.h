#pragma once

#include <glue/glue.h>

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>

struct lua_State;

namespace glue {
  
  class LuaGlue:public Glue{
    lua_State * L = nullptr;
    std::unordered_map<const Extension *, std::string> keys;
    const std::string & get_key(const Extension *parent)const;
    
  public:
  
    LuaGlue(lua_State * state);
    ~LuaGlue();
    
    void connect_function(const Extension *parent,const std::string &name,const lars::AnyFunction &f)override;
    void connect_extension(const Extension *parent,const std::string &name,const Extension &e)override;
  };
 
  class LuaState {
  private:
    lua_State * L;
    bool ownsState;
    LuaGlue glue;
    
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
    
    /**
     * Create a new lua state and destroys the state after use.
     */
    LuaState();
    
    /**
     * Borrow an existing lua state.
     */
    LuaState(lua_State * state);
    
    LuaState(const LuaState &other) = delete;

    /**
     * The glue associated with this lua state.
     * Will be destroyed with the state.
     */
    LuaGlue &getGlue() { return glue; };
    
    /**
     * Loads the standard lua libraries
     */
    void openLibs() const;
    
    /**
     * Runs the code. The name is used for debugging purposes.
     */
    void run(const std::string_view &code, const std::string &name = "anonymous lua code") const;
    
    /**
     * Runs the code and returns the result as a `lars::Any`
     */
    lars::Any get(const std::string &value, const std::string &name = "anonymous lua code") const;
    
    /**
     * Runs the code and returns the result as `T`
     */
    template <class T> T get(const std::string &code,const std::string &name = "anonymous lua code") const {
      return get(code, name).get<T>();
    }
    
    /**
     * Runs lua garbage collector
     */
    void collectGarbage() const;
    
    /**
     * return the lua stack size
     */
    unsigned stackSize() const;
    
    ~LuaState();
  };
  
}
