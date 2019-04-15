#include <lars/glue/glue.h>

namespace lars{
 
  void Extension::add_extension(const std::string &name,const std::shared_ptr<Extension> &e){
    _extensions[name] = e;
    on_extension_added.notify(name,*_extensions[name]);
  }
  
  void Extension::add_function(const std::string &name,const AnyFunction &f){
    _functions[name] = f;
    on_function_added.notify(name,_functions[name]);
  }
  
  void Extension::connect(Glue &glue)const{
    for(auto && f:functions()) glue.connect_function(this,f.first, f.second);
    for(auto && e:extensions()) glue.connect_extension(this,e.first, *e.second);

    glue.function_added_observer.add_observer(on_function_added.create_observer([&](const std::string &name,const AnyFunction &f){
      glue.connect_function(this,name, f);
    }));

    glue.extension_added_observer.add_observer(on_extension_added.create_observer([&](const std::string &name,const Extension &e){
      glue.connect_extension(this,name, e);
    }));
  }
  
}
