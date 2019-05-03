#pragma once

#include <lars/any.h>
#include <lars/any_function.h>
#include <lars/make_function.h>
#include <lars/event.h>

#include <unordered_map>
#include <variant>
#include <type_traits>

namespace glue{
  
  using Any = lars::Any;
  using AnyFunction = lars::AnyFunction;
  
  class NewExtension {
  private:
    struct Data;
    std::shared_ptr<Data> data;

  public:
    struct MemberNotFoundException;
    struct Member;
    struct MemberDelegate;

    lars::EventReference<const std::string &, const Member &> onMemberChanged;
    
    NewExtension();
    
    NewExtension::Member * getMember(const std::string &key);
    const NewExtension::Member * getMember(const std::string &key)const;
    MemberDelegate operator[](const std::string &key);
    const Member &operator[](const std::string &key) const ;
    
  protected:
    Member &getOrCreateMember(const std::string &key);
  };
  
  struct NewExtension::MemberNotFoundException:public std::exception{
  private:
    mutable std::string buffer;
  public:
    std::string name;
    MemberNotFoundException(const std::string &_name):name(_name){}
    const char * what()const noexcept override;
  };
  
  /**
   * detect callable types.
   * source: https://stackoverflow.com/questions/15393938/
   */
  template <class... > using void_t = void;
  template <class T> using has_opr_t = decltype(&T::operator());
  template <class T, class = void> struct is_callable : std::false_type { };
  template <class T> struct is_callable<T, void_t<has_opr_t<typename std::decay<T>::type>>> : std::true_type { };
  
  struct NewExtension::Member{
  private:
    void setValue(lars::Any &&);
    void setFunction(const lars::AnyFunction &);
    void setExtension(const NewExtension &);

    std::variant<
      lars::Any,
      lars::AnyFunction,
      NewExtension
    > data;

  public:
    /** Custom exception for invalid variant access (to be able to target iOS < 11) */
    struct InvalidCastException: std::exception {
      const char * what()const noexcept override{ return "invalid extension member cast"; }
    };
    
    lars::Any & asAny();
    const lars::Any & asAny() const;
    
    const lars::AnyFunction & asFunction() const;
    const NewExtension & asExtension() const;
    
    template <typename ... Args> lars::Any operator()(Args && ... args)const{
      return asFunction()(std::forward<Args>(args)...);
    }
    
    const Member &operator[](const std::string &key)const{
      return asExtension()[key];
    }

    Member &operator=(const lars::AnyFunction &);
    Member &operator=(const NewExtension &);
    
    template <class T> typename std::enable_if<
      !is_callable<T>::value && !std::is_base_of<
        NewExtension, typename std::remove_reference<T>::type
      >::value,
      Member &
    >::type operator=(T && v){
      setValue(std::forward<T>(v));
      return *this;
    }
    
    template <class T> T get()const{ return asAny().get<T>(); }
    template <class T> T get(){ return asAny().get<T>(); }
    operator const lars::AnyFunction &()const{ return asFunction(); }
    operator const NewExtension &()const{ return asExtension(); }
    operator const lars::Any &()const{ return asAny(); }
    operator lars::Any &(){ return asAny(); }
  };
  
  struct NewExtension::MemberDelegate {
  private:
    NewExtension * parent;
    std::string key;
    NewExtension::Member * member;
    
    Member &getMember()const{
      if (member){
        return *member;
      } else {
        throw MemberNotFoundException(key);
      }
    }
    
  public:
    
    MemberDelegate(NewExtension * p, const std::string &k):parent(p),key(k){
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
    operator const NewExtension &()const{ return getMember(); }
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
  template <class T> void setClass(NewExtension &extension){
    extension[classKey] = lars::getTypeIndex<T>();
  }
  
  /**
   * Tells a language binding that members may be inherited from the other extension.
   */
  static const std::string extendsKey = "__extends";
  inline void setExtends(NewExtension &extension, const NewExtension &other){
    extension[extendsKey] = other;
  }
  
}
