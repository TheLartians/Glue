
#include <lars/duktape_glue.h>
#include <lars/log.h>
#include <lars/to_string.h>
#include <lars/destructor.h>
#include <duktape.h>

#include <unordered_map>
#include <duktape.h>

//#define LARS_GLUE_DUK_DEBUG

#ifdef LARS_GLUE_DUK_DEBUG
#define DUK_VERBOSE_LOG(X) LARS_LOG_WITH_PROMPT(X,"duk glue: ")
#define DUK_MEMORY_DEBUG_LOG(X) LARS_LOG_WITH_PROMPT(X,"duk glue: ")
#define DUK_DETAILED_ERRORS
#else
#define DUK_VERBOSE_LOG(X)
#define DUK_MEMORY_DEBUG_LOG(X)
#endif

namespace {
  
  namespace duk_glue{
    std::shared_ptr<bool> get_duktape_active_flag(duk_context *ctx);
    
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
#ifdef DUK_DETAILED_ERRORS
      if(duk_is_undefined(ctx, idx)){
        return nullptr;
      }
#endif
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
      if(!ptr) throw std::runtime_error("cannot extract c++ object of type " + lars::get_type_name<T>());
      return *ptr;
    }
    
    template <class T> T & create_and_push_object(duk_context *ctx){
      push_class<T>(ctx);
      duk_new(ctx, 0);
      return get_object<T>(ctx);
    }
    
    std::string UNUSED as_string(duk_context * ctx,int idx = -1){
      duk_dup(ctx, idx);
      std::string result = duk_to_string(ctx, -1);
      duk_pop(ctx);
      return result;
    }
    
    std::string add_to_stash(duk_context * ctx, const std::string &key,int idx = -1){
      DUK_VERBOSE_LOG("add " << key << " to stash: " << as_string(ctx, idx));
      duk_push_global_stash(ctx);
      duk_dup(ctx, idx < 0 ? idx-1 : idx);
      duk_put_prop_string(ctx, -2, key.c_str());
      duk_pop(ctx);
      return key;
    }

    std::string add_to_stash(duk_context * ctx,int idx = -1){
      static int obj_id = 0;
      auto key = "lars.glue.stash." + std::to_string(obj_id);
      obj_id++;
      return add_to_stash(ctx, key, idx);
    }
    
    void push_from_stash(duk_context * ctx,const std::string &key){
      duk_push_global_stash(ctx);
      duk_get_prop_string(ctx, -1, key.c_str());
      duk_replace(ctx, -2);
      DUK_VERBOSE_LOG("push " << key << " from stash: " << as_string(ctx, -1));
    }
    
    void remove_from_stash(duk_context * ctx,const std::string &key){
      DUK_VERBOSE_LOG("remove " << key << " from stash");
      duk_push_global_stash(ctx);
      duk_del_prop_string(ctx, -1, key.c_str());
      duk_pop(ctx);
    }
    
    struct StashedObject{
      duk_context * ctx;
      std::string key;
      StashedObject(duk_context * c,const std::string k):ctx(c),key(k){ DUK_VERBOSE_LOG("create stashed object: " << key); }
      StashedObject(const StashedObject &) = delete;
      ~StashedObject(){ DUK_VERBOSE_LOG("delete stashed object: " << key); remove_from_stash(ctx, key); }
      void push()const{ push_from_stash(ctx, key); }
    };
    
    void push_function(duk_context * ctx,const lars::AnyFunction &f);
    
    void push_value(duk_context * ctx,const lars::Any &value){
      using namespace lars;
      
      if(!value){
        duk_push_undefined(ctx);
        return;
      }
      
      struct PushVisitor:public ConstVisitor<lars::VisitableType<double>,lars::VisitableType<std::string>,lars::VisitableType<lars::AnyFunction>,lars::VisitableType<int>,lars::VisitableType<bool>,lars::VisitableType<StashedObject>>{
        duk_context * ctx;
        bool push_any = false;
        void visit_default(const lars::VisitableBase &data)override{ DUK_VERBOSE_LOG("push any<" << data.type().name() << ">"); push_any = true; }
        void visit(const lars::VisitableType<bool> &data)override{ DUK_VERBOSE_LOG("push bool"); duk_push_boolean(ctx, data.data); }
        void visit(const lars::VisitableType<int> &data)override{ DUK_VERBOSE_LOG("push int"); duk_push_int(ctx, data.data); }
        void visit(const lars::VisitableType<double> &data)override{ DUK_VERBOSE_LOG("push double"); duk_push_number(ctx, data.data); }
        void visit(const lars::VisitableType<std::string> &data)override{ DUK_VERBOSE_LOG("push string"); duk_push_string(ctx, data.data.c_str()); }
        void visit(const lars::VisitableType<lars::AnyFunction> &data)override{ DUK_VERBOSE_LOG("push function"); push_function(ctx,data.data); }
        void visit(const lars::VisitableType<StashedObject> &data)override{ DUK_VERBOSE_LOG("push object"); data.data.push(); }
      } visitor;
      
      visitor.ctx = ctx;
      value.accept_visitor(visitor);
      if(visitor.push_any) create_and_push_object<lars::Any>(ctx) = value;
    }
    
    lars::Any extract_value(duk_context * ctx,duk_idx_t idx,lars::TypeIndex type){
      auto assert_value_exists = [](auto && v){ if(!v) throw std::runtime_error("invalid argument type"); return v; };
      DUK_VERBOSE_LOG("extract " << type.name());
      
      if(auto ptr = get_object_ptr<lars::Any>(ctx,idx)){
        if(ptr->type() == type || type == lars::get_type_index<lars::Any>()){
          DUK_VERBOSE_LOG("extracted any<" << ptr->type().name() << ">");
          return *ptr;
        }
        else{
          DUK_VERBOSE_LOG("unsafe extracted any<" << ptr->type().name() << ">");
          return *ptr;
        }
      }
      
      if(type == lars::get_type_index<lars::Any>()){
        switch (duk_get_type(ctx, idx)) {
          case DUK_TYPE_NUMBER: type = lars::get_type_index<double>(); break;
          case DUK_TYPE_STRING: type = lars::get_type_index<std::string>(); break;
          case DUK_TYPE_BOOLEAN: type = lars::get_type_index<bool>(); break;
          case DUK_TYPE_OBJECT: type = lars::get_type_index<StashedObject>(); break;
          default: break;
        }
      }
      
      if(type == lars::get_type_index<std::string>()){ return lars::make_any<std::string>(assert_value_exists(duk_to_string(ctx, idx))); }
      else if(type == lars::get_type_index<double>()){ return lars::make_any<double>(assert_value_exists(duk_to_number(ctx, idx))); }
      else if(type == lars::get_type_index<int>()){ return lars::make_any<int>(assert_value_exists(duk_to_int(ctx, idx))); }
      else if(type == lars::get_type_index<bool>()){ return lars::make_any<bool>(assert_value_exists(duk_to_boolean(ctx, idx))); }
      else if(type == lars::get_type_index<StashedObject>()){ return lars::make_any<StashedObject>(ctx,add_to_stash(ctx,idx)); }
      else if(type == lars::get_type_index<lars::AnyFunction>()){
        auto key = add_to_stash(ctx,idx);
        auto active = get_duktape_active_flag(ctx);
        lars::AnyFunction f = [=,destructor = lars::make_shared_destructor([=](){
          if (*active) remove_from_stash(ctx, key);
        })](lars::AnyArguments &args){
          if (!*active) {
            throw std::runtime_error("calling function with invalid duktape state");
          }
          push_from_stash(ctx, key.c_str());
          DUK_VERBOSE_LOG("calling captured function " << key << ": " << as_string(ctx));
          for(auto && arg:args) push_value(ctx, arg);
          duk_call(ctx, args.size());
          if(duk_is_undefined(ctx, -1)){ DUK_VERBOSE_LOG("call has no return value"); return lars::Any(); }
          auto result = extract_value(ctx, -1, lars::get_type_index<lars::Any>());
          DUK_VERBOSE_LOG("result type: " << result.type().name());
          duk_pop(ctx);
          return result;
        };
        return lars::make_any<lars::AnyFunction>(f);
      }
      else{
        DUK_VERBOSE_LOG("cannot extract from: " << as_string(ctx, idx));
#ifdef DUK_DETAILED_ERRORS
        throw std::runtime_error("cannot convert argument " + std::to_string(idx) + ": \"" + lars::stream_to_string(type.name()) + "\" from \"" + as_string(ctx, idx) + "\"");
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
        
        lars::AnyArguments args;
        
        auto argc = f.argument_count();
        if(argc == -1) argc = duk_get_top(ctx);
        DUK_VERBOSE_LOG("calling function " << &f << " with " << argc << " arguments");
        
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
    
    const std::string DUKTAPE_ACTIVE_FLAG_KEY = "lars.glue.duktape_active";
    
    std::shared_ptr<bool> get_duktape_active_flag(duk_context *ctx){
      push_from_stash(ctx, DUKTAPE_ACTIVE_FLAG_KEY);
      if (!duk_is_undefined(ctx, -1)) {
        auto result = get_object<std::shared_ptr<bool>>(ctx);
        duk_pop(ctx);
        return result;
      }
      duk_pop(ctx);
      auto &result = create_and_push_object<std::shared_ptr<bool>>(ctx);
      result = std::make_shared<bool>(true);
      add_to_stash(ctx, DUKTAPE_ACTIVE_FLAG_KEY);
      duk_pop(ctx);
      return result;
    }
    
  }
}
  
namespace lars {
  
  DuktapeGlue::DuktapeGlue(duk_context * c):ctx(c){
    duk_push_global_object(ctx);
    keys[nullptr] = duk_glue::add_to_stash(ctx);
    duk_pop(ctx);
  }
  
  DuktapeGlue::~DuktapeGlue(){

  }
  
  std::string DuktapeGlue::get_key(const Extension *parent)const{
    auto it = keys.find(parent);
    if(it != keys.end()) return it->second;
    return keys.find(nullptr)->second;
  }
  
  void DuktapeGlue::connect_function(const Extension *parent,const std::string &name,const lars::AnyFunction &f){
    duk_glue::push_from_stash(ctx, get_key(parent));
    duk_glue::push_function(ctx,f);
    duk_put_prop_string(ctx, -2, name.c_str());
    duk_pop(ctx);
  }
  
  void DuktapeGlue::connect_extension(const Extension *parent,const std::string &name,const Extension &e){
    duk_glue::push_from_stash(ctx, get_key(parent));
    duk_push_object(ctx);
    keys[&e] = duk_glue::add_to_stash(ctx);
    duk_put_prop_string(ctx, -2, name.c_str());
    duk_pop(ctx);
    e.connect(*this);
  }
  
}

namespace lars {
  
  const char * DuktapeContext::Error::what() const noexcept {
    return duk_safe_to_string(ctx, -1);
  }
  
  DuktapeContext::DuktapeContext::Error::~Error(){
    duk_pop(ctx);
  }

  DuktapeContext::DuktapeContext(){
    ctx = duk_create_heap(NULL, NULL, NULL, NULL, NULL);
  }
  
  DuktapeContext::~DuktapeContext(){
    *duk_glue::get_duktape_active_flag(ctx) = false;
    duk_destroy_heap(ctx);
  }
  
  DuktapeGlue &DuktapeContext::getGlue(){
    if (glue) {
      return *glue;
    }
    else {
      glue = std::make_shared<DuktapeGlue>(ctx);
      return *glue;
    }
  }
  
  void DuktapeContext::run(const std::string_view &code){
    duk_push_lstring(ctx, code.data(), code.size());
    if (duk_peval(ctx) != 0) { throw Error(ctx); }
    duk_pop(ctx);
  }
  
  lars::Any DuktapeContext::getValue(const std::string_view &code, lars::TypeIndex type){
    duk_push_lstring(ctx, code.data(), code.size());
    if (duk_peval(ctx) != 0) { throw Error(ctx); }
    auto result = duk_glue::extract_value(ctx, -1, type);
    duk_pop(ctx);
    return result;
  }
  
  void DuktapeContext::collectGarbage(){
    duk_gc(ctx, 0);
  }

}
