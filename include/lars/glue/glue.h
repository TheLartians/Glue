#pragma once

#include <lars/any.h>
#include <lars/mutator.h>
#include <lars/visitor.h>
#include <lars/event.h>

#include <map>

namespace lars{
  
  class Extension;
  
  class Glue{
  private:
    friend Extension;
    MultiOberserver function_added_observer;
    MultiOberserver extension_added_observer;
  public:
    virtual void connect_function(const Extension *parent,const std::string &name,const AnyFunction &f) = 0;
    virtual void connect_extension(const Extension *parent,const std::string &name,const Extension &e) = 0;
    virtual ~Glue(){}
  };
  
  class Extension{
    using FunctionMap = std::map<std::string, AnyFunction>;
    using ExtensionMap = std::map<std::string, std::shared_ptr<Extension>>;
    
    MUTATOR_MEMBER_PROTECTED(FunctionMap, functions, , );
    MUTATOR_MEMBER_PROTECTED(ExtensionMap, extensions, , );
    MUTATOR_MEMBER_PROTECTED(std::unique_ptr<lars::TypeIndex>, class_type, , );
    MUTATOR_MEMBER_PROTECTED(std::unique_ptr<lars::TypeIndex>, shared_class_type, , );
    MUTATOR_MEMBER_PROTECTED(std::unique_ptr<lars::TypeIndex>, base_class_type, , );
    public:
    
    Event<std::string,AnyFunction> on_function_added;
    Event<std::string,const Extension &> on_extension_added;

    void add_function(const std::string &name,const AnyFunction &f);
    void add_extension(const std::string &name,const std::shared_ptr<Extension> &);

    const AnyFunction &get_function(const std::string &name){
      auto it = functions().find(name);
      if(it == functions().end()) throw std::runtime_error("function not found");
      return it->second;
    }
    
    const std::shared_ptr<Extension> &get_extension(const std::string &name){
      auto it = extensions().find(name);
      if(it == extensions().end()) throw std::runtime_error("extension not found");
      return it->second;
    }
      
    void connect(Glue & glue)const;
    
    template <class T> void set_class(){
      set_class_type(new TypeIndex(get_type_index<T>()));
      set_shared_class_type(new TypeIndex(get_type_index<std::shared_ptr<T>>()));
    }
    template <class T> void set_base_class(){ set_base_class_type(new TypeIndex(get_type_index<T>())); }
    
    virtual ~Extension(){}
  };
  
  
}

