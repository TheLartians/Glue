#include <glue/glue.h>

namespace glue{
  
  const char * Extension::MemberNotFoundException::what()const noexcept{
    if (buffer.size() == 0) {
      buffer = "Extension contains no function '" + name + "'.";
    }
    return buffer.c_str();
  }
  
  void Extension::add_extension(const std::string &name,const std::shared_ptr<Extension> &e){
    _extensions[name] = e;
    on_extension_added.emit(name,*_extensions[name]);
  }
  
  void Extension::add_function(const std::string &name,const lars::AnyFunction &f){
    _functions[name] = f;
    on_function_added.emit(name,_functions[name]);
  }
  
  void Extension::connect(Glue &glue)const{
    for(auto && f:functions()) glue.connect_function(this,f.first, f.second);
    for(auto && e:extensions()) glue.connect_extension(this,e.first, *e.second);
    
    glue.observers.emplace_back(on_function_added.createObserver([&](const std::string &name,const lars::AnyFunction &f){
      glue.connect_function(this,name, f);
    }));
    
    glue.observers.emplace_back(on_extension_added.createObserver([&](const std::string &name,const Extension &e){
      glue.connect_extension(this,name, e);
    }));
  }

}
