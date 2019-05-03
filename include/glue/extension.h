#pragma once

#include <lars/any.h>
#include <lars/any_function.h>
#include <lars/make_function.h>

#include <unordered_map>
#include <variant>
#include <type_traits>

namespace glue{
  
  using Any = lars::Any;
  using AnyFunction = lars::AnyFunction;
  
  class NewExtension {
  public:
    struct MemberNotFoundException;
    struct Member;
    
    NewExtension():data(std::make_shared<Data>()){}
    
    Member &getMember();
    const Member &getMember()const;
    
    NewExtension::Member * getMember(const std::string &key);
    const NewExtension::Member * getMember(const std::string &key)const;
    Member &operator[](const std::string &key){ return data->members[key]; }
    const Member &operator[](const std::string &key) const ;
    
  private:
    struct Data {
      std::unordered_map<std::string, Member> members;
    };
    
    std::shared_ptr<Data> data;
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
    /** Custom exception for invalid variant access (to be able to target iOS < 11) */
    struct InvalidCastException: std::exception {
      const char * what()const noexcept override{ return "invalid extension member cast"; }
    };
    
    std::variant<
      lars::Any,
      lars::AnyFunction,
      NewExtension
    > data;
    
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
    
    void setValue(lars::Any &&);
    void setFunction(const lars::AnyFunction &);
    void setExtension(const NewExtension &);

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

    explicit operator bool()const{ return data.index() != std::variant_npos; }
    operator const lars::AnyFunction &()const{ return asFunction(); }
    operator const NewExtension &()const{ return asExtension(); }
    operator const lars::Any &()const{ return asAny(); }
    operator lars::Any &(){ return asAny(); }
  };
  
  template <class T> void setClass(NewExtension &extension){
    extension["__class"] = lars::getTypeIndex<T>();
  }
  
  inline void setExtends(NewExtension &extension, const NewExtension &other){
    extension["__extends"] = other;
  }
  
}
