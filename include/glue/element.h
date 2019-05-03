#pragma once

#include <lars/any.h>
#include <lars/any_function.h>
#include <lars/event.h>

#include <type_traits>
#include <unordered_map>
#include <string>
#include <initializer_list>
#include <utility>

namespace glue{
  
  using Any = lars::Any;
  using AnyReference = lars::AnyReference;
  using AnyFunction = lars::AnyFunction;
  
  namespace detail {
    /**
     * detect callable types.
     * source: https://stackoverflow.com/questions/15393938/
     */
    template <class... > using void_t = void;
    template <class T> using has_opr_t = decltype(&T::operator());
    template <class T, class = void> struct is_callable : std::false_type { };
    template <class T> struct is_callable<T, void_t<has_opr_t<typename std::decay<T>::type>>> : std::true_type { };
  }
  
  class ElementMap;
  class ElementMapEntry;
  
  class Element {
  private:
    AnyReference data;
    friend ElementMapEntry;
    
  public:
    using Map = ElementMap;
    
    Element() = default;
    Element(const Element &) = default;
    Element(Element &&) = default;

    template <
      class T,
      typename = typename std::enable_if<
        !detail::is_callable<T>::value
        && !std::is_same<typename std::decay<T>::type, Element>::value
      >::type
    > Element(T && v):data(std::forward<T>(v)){}

    Element(AnyFunction && f):data(f){}
    
    Element &operator=(const Element &) = default;

    template <
      class T,
      typename = typename std::enable_if<
        !detail::is_callable<T>::value
        && !std::is_same<typename std::decay<T>::type, Element>::value
      >::type
    > Element & operator=(T && value){
      data.set<T>(value);
      return *this;
    }
    
    Element & operator=(AnyFunction && f);
    
    template <typename ... Args> Any operator()(Args && ... args)const{
      return data.get<AnyFunction>()(std::forward<Args>(args)...);
    }
    
    template <class T, typename ... Args> auto & set(Args && ... args){
      return data.set<T>(std::forward<Args>(args)...);
    }
    
    template <class T> T get()const{
      return data.get<T>();
    }
    
    ElementMapEntry operator[](const std::string &key);
    
    explicit operator bool()const{ return bool(data); }
    Map * asMap()const{ return data.tryGet<Map>(); }
  };

  class ElementMap: public lars::Visitable<ElementMap> {
  private:
    std::unordered_map<std::string, Element> data;
    Element *getElement(const std::string &key);
    Element &getOrCreateElement(const std::string &key);
    
  public:
    using Entry = ElementMapEntry;
    friend Entry;
    
    lars::Event<const std::string &, const Element &> onValueChanged;
    
    ElementMap(){}
    ElementMap(const ElementMap &) = delete;
    
    Entry operator[](const std::string &key);
    
    ~ElementMap(){}
  };
  
  class ElementMapEntry {
  private:
    ElementMap * parent;
    std::string key;
    Element &getOrCreateElement();
    
  public:
    ElementMapEntry(ElementMap * p, const std::string &k):parent(p), key(k){
    }
    
    explicit operator bool()const{ return parent->getElement(key) != nullptr; }
    
    template <class T> ElementMapEntry &operator=(T && value){
      auto &element = getOrCreateElement();
      element = std::forward<T>(value);
      parent->onValueChanged.emit(key, element);
      return *this;
    }
    
    template <class T, typename ... Args> auto & set(Args && ... args){
      auto &element = getOrCreateElement();
      auto &value = element.set<T>(std::forward<Args>(args)...);
      parent->onValueChanged.emit(key, element);
      return value;
    }

    template <typename ... Args> Any operator()(Args && ... args) {
      return getOrCreateElement()(std::forward<Args>(args)...);
    }
    
    ElementMapEntry operator[](const std::string &key);
    
    template <class T> T get(){
      return getOrCreateElement().get<T>();
    }
    
  };

  /**
   * Internal keys
   */
  static const std::string extendsKey = "__extends";
  static const std::string classKey = "__class";

}
