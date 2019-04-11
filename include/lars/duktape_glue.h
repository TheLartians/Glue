#pragma once

#include <lars/glue.h>

#include <string>
#include <unordered_map>

struct duk_hthread;
typedef struct duk_hthread duk_context;

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
  
  class DuktapeContext {
  protected:
    duk_context * ctx = nullptr;
    std::shared_ptr<DuktapeGlue> glue;
    
  public:
    struct Error: public std::exception {
      duk_context * ctx;
      Error(duk_context * c):ctx(c){}
      const char * what() const noexcept override;
      ~Error();
    };
    
    
    DuktapeContext();
    ~DuktapeContext();
    
    void run(const std::string_view &code);
    lars::Any get_value(const std::string_view &code, lars::TypeIndex type);
    
    template <class T> T get_value(const std::string &code){
      using RawType = typename std::remove_reference<typename std::remove_const<T>::type>::type;
      auto anyValue = get_value(code, lars::get_type_index<RawType>());
      return anyValue.template get<T>();
    }

    double get_numeric(const std::string &code){
      auto anyValue = get_value(code, lars::get_type_index<double>());
      return anyValue.template get_numeric<double>();
    }
    
    DuktapeGlue &get_glue();
    void collect_garbage();
  };

}
