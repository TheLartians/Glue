#include <duktape/duktape.h>

#include <lars/any.h>
#include <lars/mutator.h>
#include <lars/component_visitor.h>
#include <lars/destructor.h>

#include <lars/log.h>
#include <lars/to_string.h>

#include <unordered_map>

#define DUK_MEMORY_DEBUG_LOG(X)
#define DUK_DETAILED_ERRORS

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


namespace duk_glue{
  
  template <class T> using internal_type = std::pair<lars::TypeIndex,T>;
  
  namespace class_helper{
  
    template <class T> duk_ret_t class_destructor(duk_context *ctx){
      // The object to delete is passed as first argument
      duk_get_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("deleted"));
      bool deleted = duk_to_boolean(ctx, -1);
      duk_pop(ctx);
      
      if (!deleted) {
        duk_get_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("data"));
        auto ptr = duk_to_pointer(ctx, -1);
        delete static_cast<internal_type<T> *>(ptr);
        duk_pop(ctx);
        DUK_MEMORY_DEBUG_LOG("deleted " << ptr);
        
        // Mark as deleted
        duk_push_boolean(ctx, true);
        duk_put_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("deleted"));
      }
      
      return 0;
    }

    template <class T> duk_ret_t class_constructor(duk_context *ctx){
      duk_push_this(ctx);
      
      // Store the underlying object
      auto ptr = new internal_type<T>(lars::get_type_index<T>(),T());
      duk_push_pointer(ctx, ptr);
      DUK_MEMORY_DEBUG_LOG("created " << ptr);
      
      duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("data"));
      
      // Store a boolean flag to mark the object as deleted because the destructor may be called several times
      duk_push_boolean(ctx, false);
      duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("deleted"));
      
      // Store the function destructor
      duk_push_c_function(ctx, &class_destructor<T>, 1);
      duk_set_finalizer(ctx, -2);
      
      return 0;
    }
    
  }
  
  template <class T> void push_class(duk_context *ctx){
    auto internal_class_name = std::string( DUK_HIDDEN_SYMBOL() ) + lars::get_type_name<T>();
    auto exists = duk_get_global_string(ctx, internal_class_name.c_str());
    if(exists){ return; }
    
    duk_pop(ctx);
    duk_push_c_function(ctx, &class_helper::class_constructor<T>, 0);
    
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, "prototype");
    
    duk_dup_top(ctx);
    duk_put_global_string(ctx, internal_class_name.c_str() );
  }
  
  template <class T> T * get_object_ptr(duk_context *ctx,int idx = -1){
    auto has_data = duk_get_prop_string(ctx, idx, DUK_HIDDEN_SYMBOL("data"));
    if(!has_data){ duk_pop(ctx); return nullptr; }
    auto ptr = duk_to_pointer(ctx, -1);
    duk_pop(ctx);
    DUK_MEMORY_DEBUG_LOG("get: " << ptr);
    if(!ptr) return nullptr;
    auto * res = static_cast<internal_type<T> *>(ptr);
    if(res->first != lars::get_type_index<T>()) return nullptr;
    return &res->second;
  }

  template <class T> T & get_object(duk_context *ctx,int idx = -1){
    auto ptr = get_object_ptr<T>(ctx,idx);
    if(!ptr) throw std::runtime_error("cannot extract c++ object");
    return *ptr;
  }

  template <class T> T & create_and_push_object(duk_context *ctx){
    push_class<T>(ctx);
    duk_new(ctx, 0);
    return get_object<T>(ctx);
  }
  
  std::string add_to_stash(duk_context * ctx){
    static int obj_id = 0;
    auto key = std::to_string(obj_id);
    duk_push_global_stash(ctx);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, key.c_str());
    duk_pop(ctx);
    return key;
  }
  
  void push_from_stash(duk_context * ctx,const std::string &key){
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, key.c_str());
  }
  
  void remove_from_stash(duk_context * ctx,const std::string &key){
    duk_push_global_stash(ctx);
    duk_del_prop_string(ctx, -1, key.c_str());
    duk_pop(ctx);
  }
  
  void push_function(duk_context * ctx,const lars::AnyFunction &f);
  
  void push_value(duk_context * ctx,lars::Any value){
    using namespace lars;
    
    struct PushVisitor:public ConstVisitor<AnyScalarData<double>,AnyScalarData<std::string>,AnyScalarData<lars::AnyFunction>,AnyScalarData<int>>{
      duk_context * ctx;
      void visit(const AnyScalarData<int> &data)override{ duk_push_int(ctx, data.data); }
      void visit(const AnyScalarData<double> &data)override{ duk_push_number(ctx, data.data); }
      void visit(const AnyScalarData<std::string> &data)override{ duk_push_string(ctx, data.data.c_str()); }
      void visit(const AnyScalarData<lars::AnyFunction> &data)override{ push_function(ctx,data.data); }
    } visitor;
    
    visitor.ctx = ctx;
    
    try{
      value.accept_visitor(visitor);
    }catch(lars::IncompatibleVisitorException){
      create_and_push_object<Any>(ctx) = value;
    };
    
  }
  
  lars::Any extract_value(duk_context * ctx,duk_idx_t idx,lars::TypeIndex type){
    auto assert_value_exists = [](auto && v){ if(!v) throw std::runtime_error("invalid argument type"); return v; };
    
    if(auto ptr = get_object_ptr<lars::Any>(ctx,idx)) return *ptr;
    
    if(type == lars::get_type_index<std::string>()){ return lars::make_any<std::string>(assert_value_exists(duk_to_string(ctx, idx))); }
    else if(type == lars::get_type_index<double>()){ return lars::make_any<double>(assert_value_exists(duk_to_number(ctx, idx))); }
    else if(type == lars::get_type_index<int>()){ return lars::make_any<int>(assert_value_exists(duk_to_int(ctx, idx))); }
    else if(type == lars::get_type_index<lars::AnyFunction>()){
      auto key = add_to_stash(ctx);
      lars::AnyFunction f = [=,destructor = lars::make_shared_destructor([=](){ remove_from_stash(ctx, key); })](lars::AnyArguments &args){
        push_from_stash(ctx, key.c_str());
        for(auto && arg:args) push_value(ctx, arg);
        duk_call(ctx, args.size());
      };
      return lars::make_any<lars::AnyFunction>(f);
    }
    else{
#ifdef DUK_DETAILED_ERRORS
      throw std::runtime_error("cannot convert argument: " + lars::stream_to_string(type.name()) + " from " + duk_to_string(ctx, idx));
#else
      throw std::runtime_error("cannot convert argument");
#endif
    }
  }

  void push_function(duk_context * ctx,const lars::AnyFunction &f){
    
    duk_push_c_function(ctx, +[](duk_context *ctx)->duk_ret_t{
      duk_push_current_function(ctx);
      duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("data"));
      auto &f = get_object<lars::AnyFunction>(ctx);
      duk_pop(ctx);
      duk_pop(ctx);
       
      std::vector<lars::Any> args;
      
      auto argc = f.argument_count();
      if(argc == -1) argc = duk_get_top(ctx);
      
      LARS_LOG("call with args: " << argc);
      for(int i=0;i<argc;++i) args.emplace_back(extract_value(ctx, i, f.argument_type(i)));

      auto result = f.call(args);
      
      if(result){
        push_value(ctx, result);
        return 1;
      }
      
      return 0;
    }, f.argument_count() != -1 ? f.argument_count() : DUK_VARARGS);
    
    create_and_push_object<lars::AnyFunction>(ctx) = f;
    duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("data"));
  }
  

}



class DuktapeGlue:public lars::Glue{
public:
  
  duk_context * ctx = nullptr;
  
  void connect_function(const std::string &name,const lars::AnyFunction &f)override{
    duk_glue::push_function(ctx,f);
    duk_put_global_string(ctx, name.c_str());
  }
  
};


int main(int argc, char *argv[]) {
  lars::Module module;
  
  module.add_function("meaning_of_life",[](){ return 42; });
  module.add_function("log",[](const std::string &str){ std::cout << "log: " << str << std::endl; });
  module.add_function("add",[](double a,int b){ return a+b; });
  module.add_function("get_log_function",[](const std::string &prompt){ return lars::AnyFunction([=](std::string str){ std::cout << prompt << ": " << str << std::endl; }); });
  module.add_function("call_callback",[](const lars::AnyFunction &f){ f(42); });
  
  static unsigned my_class_instances = 0;
  struct MyClass{ std::string data; MyClass(){ my_class_instances++; } ~MyClass(){ my_class_instances--; } };
  module.add_function("create_my_class",[](){ return MyClass(); });
  module.add_function("get_my_class_data",[](const MyClass &my_class){ return my_class.data; });
  module.add_function("set_my_class_data",[](MyClass &my_class,const std::string &str){ my_class.data = str; });

  duk_context *ctx = duk_create_heap(NULL, NULL, NULL, NULL, NULL);
  
  DuktapeGlue glue;
  glue.ctx = ctx;
  
  module.connect(glue);

  duk_eval_string_noresult(ctx, "var value = meaning_of_life(); if(value != 42) throw 'wrong universe!'");
  duk_eval_string_noresult(ctx, "log('hello world!')");
  duk_eval_string_noresult(ctx, "log('2+3=' + add(2, 3))");
  duk_eval_string_noresult(ctx, "var new_log = get_log_function('test'); new_log(add(2,40))");
  duk_eval_string_noresult(ctx, "function myLog(x){ log('myLog: ' + x); }; call_callback(myLog)");
  duk_eval_string_noresult(ctx, "var my_class = create_my_class()");
  duk_eval_string_noresult(ctx, "set_my_class_data(my_class,'data')");
  duk_eval_string_noresult(ctx, "log(get_my_class_data(my_class))");
  duk_eval_string_noresult(ctx, "set_my_class_data()");

  assert(my_class_instances == 0);
  
  return 0;
}
