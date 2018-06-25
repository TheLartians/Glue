/* test.c */

#include <iostream>

#include <duktape/duktape.h>

#include <lars/any.h>
#include <lars/mutator.h>
#include <lars/component_visitor.h>

#include <unordered_map>

namespace lars{
  
  class Glue{
  public:
    virtual void connect_function(const std::string &name,const lars::AnyFunction &f) = 0;
  };

  class Module{
    using FunctionMap = std::unordered_map<std::string, lars::AnyFunction>;
    MUTATOR_MEMBER_PROTECTED(FunctionMap, functions, , );
  public:
    
    void add_function(const std::string &name,const lars::AnyFunction &f){ _functions[name] = f; }
    
    const lars::AnyFunction &get_function(const std::string &name){
      auto it = functions().find(name);
      if(it == functions().end()) throw std::runtime_error("function not found");
      return it->second;
    }
    
    void connect(Glue & glue){
      for(auto && f:functions()) glue.connect_function(f.first, f.second);
    }
    
  };
  
}


class DukTapeSticker:public lars::Glue{
public:
  
  duk_context * context = nullptr;
  
  static lars::Any extract_value(duk_context * ctx,duk_idx_t idx,lars::TypeIndex type){
    if(type == lars::get_type_index<std::string>()){
      auto value = duk_to_string(ctx, idx);
      if(!value) throw std::runtime_error("invalid argument type");
      return lars::make_any<std::string>(value);
    }
    else if(type == lars::get_type_index<double>()){
      auto value = duk_to_number(ctx, idx);
      if(!value) throw std::runtime_error("invalid argument type");
      return lars::make_any<double>(value);
    }
    else{ throw std::runtime_error("cannot convert argument"); }
  }
  
  static void push_value(duk_context * ctx,lars::Any value){
    using namespace lars;
    
    struct PushVisitor:public ConstVisitor<AnyScalarData<double>,AnyScalarData<std::string>>{
      duk_context * ctx;
      void visit(const AnyScalarData<double> &data){ duk_push_number(ctx, data.data); }
      void visit(const AnyScalarData<std::string> &data){ duk_push_string(ctx, data.data.c_str()); }
    } visitor;
    
    visitor.ctx = ctx;
    
    value.accept_visitor(visitor);
  }
  
  void connect_function(const std::string &name,const lars::AnyFunction &f)override{
    
    duk_push_c_function(context, +[](duk_context *ctx)->duk_ret_t{
      duk_push_current_function(ctx);
      duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("data"));
      const lars::AnyFunction *f = static_cast<const lars::AnyFunction *>(duk_get_pointer(ctx, -1));
      duk_pop(ctx);
      duk_pop(ctx);

      std::vector<lars::Any> args;
      for(int i=0;i<f->argument_count();++i) args.emplace_back(extract_value(ctx, i, f->argument_type(i)));
      auto result = f->call(args);
      
      if(result){
        push_value(ctx, result);
        return 1;
      }
      
      return 0;
    }, f.argument_count());
    
    duk_push_pointer(context, const_cast<void*>(static_cast<const void*>(&f)));
    duk_put_prop_string(context, -2, DUK_HIDDEN_SYMBOL("data"));
    
    duk_put_global_string(context, name.c_str());
    
  }
  
};

void log(std::string str){
  std::cout << "log: " << str << std::endl;
}


int main(int argc, char *argv[]) {
  
  lars::Module module;
  module.add_function("log",[](const std::string &str){ std::cout << "log: " << str << std::endl; });
  module.add_function("add",[](double a,double b){ return a+b; });

  duk_context *ctx = duk_create_heap(NULL, NULL, NULL, NULL, NULL);
  
  DukTapeSticker sticker;
  sticker.context = ctx;
  
  module.connect(sticker);

  duk_eval_string_noresult(ctx, "log('2+3=' + add(2, 3)); log('hello world!');");

  return 0;
}
