#pragma once

#include <lars/any.h>
#include <lars/any_function.h>

#include <lars/mutator.h>
#include <lars/visitor.h>
#include <lars/event.h>

#include <map>
#include <unordered_map>
#include <variant>
#include <vector>
#include <exception>
#include <memory>

namespace glue{
  
  class Extension;
  
  class Glue{
  private:
    friend Extension;
    std::vector<lars::Observer> observers;
  public:
    virtual void connect_function(const Extension *parent,const std::string &name,const lars::AnyFunction &f) = 0;
    virtual void connect_extension(const Extension *parent,const std::string &name,const Extension &e) = 0;
    virtual ~Glue(){}
  };
  
  class Extension{
  private:
    using FunctionMap = std::map<std::string, lars::AnyFunction>;
    using ExtensionMap = std::map<std::string, std::shared_ptr<Extension>>;
    
    MUTATOR_MEMBER_PROTECTED(FunctionMap, functions, , );
    MUTATOR_MEMBER_PROTECTED(ExtensionMap, extensions, , );
    MUTATOR_MEMBER_PROTECTED(std::unique_ptr<lars::TypeIndex>, class_type, , );
    MUTATOR_MEMBER_PROTECTED(std::unique_ptr<lars::TypeIndex>, shared_class_type, , );
    MUTATOR_MEMBER_PROTECTED(std::unique_ptr<lars::TypeIndex>, base_class_type, , );
    
  public:

    class MemberNotFoundException:public std::exception{
    private:
      mutable std::string buffer;
    public:
      std::string name;
      MemberNotFoundException(const std::string &_name):name(_name){}
      const char * what()const noexcept override;
    };
    
    lars::Event<std::string,lars::AnyFunction> on_function_added;
    lars::Event<std::string,const Extension &> on_extension_added;

    void add_function(const std::string &name,const lars::AnyFunction &f);
    void add_extension(const std::string &name,const std::shared_ptr<Extension> &);

    const lars::AnyFunction &get_function(const std::string &name){
      auto it = functions().find(name);
      if(it == functions().end()){
        throw MemberNotFoundException(name);
      }
      return it->second;
    }
    
    const std::shared_ptr<Extension> &get_extension(const std::string &name){
      auto it = extensions().find(name);
      if(it == extensions().end()) throw std::runtime_error("extension not found");
      return it->second;
    }
      
    void connect(Glue & glue)const;
    
    template <class T> void set_class(){
      set_class_type(new lars::TypeIndex(lars::getTypeIndex<T>()));
      set_shared_class_type(new lars::TypeIndex(lars::getTypeIndex<std::shared_ptr<T>>()));
    }
    
    template <class T> void set_base_class(){
      set_base_class_type(new lars::TypeIndex(lars::getTypeIndex<T>()));
    }
    
    virtual ~Extension(){}
  };

  
  
  
}

