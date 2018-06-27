#pragma once

#include <duktape/duktape.h>
#include <lars/glue.h>

#include <string>
#include <unordered_map>

namespace lars {
  
  class DuktapeGlue:public Glue{
    duk_context * ctx = nullptr;
    std::unordered_map<const Extension *, std::string> keys;
  public:
    DuktapeGlue(duk_context * c);
    ~DuktapeGlue();
    
    std::string get_key(const Extension *parent)const;
    void connect_function(const Extension *parent,const std::string &name,const AnyFunction &f)override;
    void connect_extension(const Extension *parent,const std::string &name,const Extension &e)override;
  };

  
}
