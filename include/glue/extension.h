#pragma once

#include <lars/any.h>
#include <lars/any_function.h>
#include <lars/make_function.h>
#include <lars/event.h>
#include <glue/element.h>

#include <unordered_map>
#include <variant>
#include <type_traits>

namespace glue{
  using Any = lars::Any;
  using AnyFunction = lars::AnyFunction;
  
  class Extension {
  private:
    struct Data;
    std::shared_ptr<Data> data;

  public:
    struct MemberNotFoundException;
    struct Member;
    struct MemberDelegate;

    lars::EventReference<const std::string &, const Member &> onMemberChanged;
    
    Extension();
    
    Extension::Member * getMember(const std::string &key);
    const Extension::Member * getMember(const std::string &key)const;
    MemberDelegate operator[](const std::string &key);
    const Member &operator[](const std::string &key) const ;
    
  protected:
    Member &getOrCreateMember(const std::string &key);
  };
  
  struct Extension::MemberNotFoundException:public std::exception{
  private:
    mutable std::string buffer;
  public:
    std::string name;
    MemberNotFoundException(const std::string &_name):name(_name){}
    const char * what()const noexcept override;
  };
  
  struct Extension::Member{
  private:
    void setValue(lars::Any &&);
    void setFunction(const lars::AnyFunction &);
    void setExtension(const Extension &);

    std::variant<
      lars::Any,
      lars::AnyFunction,
      Extension
    > data;

  public:
    /** Custom exception for invalid variant access (to be able to target iOS < 11) */
    struct InvalidCastException: std::exception {
      const char * what()const noexcept override{ return "invalid extension member cast"; }
    };
    
    lars::Any & asAny();
    const lars::Any & asAny() const;
    
    const lars::AnyFunction & asFunction() const;
    const Extension & asExtension() const;
    
    template <typename ... Args> lars::Any operator()(Args && ... args)const{
      return asFunction()(std::forward<Args>(args)...);
    }
    
    const Member &operator[](const std::string &key)const{
      return asExtension()[key];
    }

    Member &operator=(const lars::AnyFunction &);
    Member &operator=(const Extension &);
    
    template <class T> typename std::enable_if<
      !detail::is_callable<T>::value && !std::is_base_of<
        Extension, typename std::remove_reference<T>::type
      >::value,
      Member &
    >::type operator=(T && v){
      setValue(std::forward<T>(v));
      return *this;
    }
    
    template <class T> T get()const{ return asAny().get<T>(); }
    template <class T> T get(){ return asAny().get<T>(); }
    operator const lars::AnyFunction &()const{ return asFunction(); }
    operator const Extension &()const{ return asExtension(); }
    operator const lars::Any &()const{ return asAny(); }
    operator lars::Any &(){ return asAny(); }
  };
  
  struct Extension::MemberDelegate {
  private:
    Extension * parent;
    std::string key;
    Extension::Member * member;
    
    Member &getMember()const{
      if (member){
        return *member;
      } else {
        throw MemberNotFoundException(key);
      }
    }
    
  public:
    
    MemberDelegate(Extension * p, const std::string &k):parent(p),key(k){
      member = parent->getMember(key);
    }
    MemberDelegate(const MemberDelegate &) = delete;
    
    template <class T> MemberDelegate &operator=(T && value){
      if (!member) {
        member = &parent->getOrCreateMember(key);
      }
      *member = value;
      parent->onMemberChanged.emit(key, *member);
      return *this;
    }
    
    template <class T> T get(){ return getMember().get<T>(); }
    explicit operator bool(){ return member != nullptr; }
    
    operator const lars::AnyFunction &()const{ return getMember(); }
    operator const Extension &()const{ return getMember(); }
    operator lars::Any &()const{ return getMember(); }
    
    template <typename ... Args> lars::Any operator()(Args && ... args)const{
      return getMember()(args...);
    }
    
    const Member &operator[](const std::string &key)const{
      return getMember()[key];
    }
  };
  
  /**
   * Tells a language binding that extension methods may be interpreted as class methods.
   */
  static const std::string classKey = "__class";
  template <class T> void setClass(Extension &extension){
    extension[classKey] = lars::getTypeIndex<T>();
  }
  
  /**
   * Tells a language binding that members may be inherited from the other extension.
   */
  static const std::string extendsKey = "__extends";
  inline void setExtends(Extension &extension, const Extension &other){
    extension[extendsKey] = other;
  }
  
}
