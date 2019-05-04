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
  
  struct ElementInterface {
    virtual Any & getValue() = 0;
    virtual AnyReference getValue() const = 0;
    virtual void valueChanged() = 0;
    
    template <
      class T,
      typename = typename std::enable_if<
        !std::is_base_of<ElementInterface, typename std::decay<T>::type>::value
      >::type
    > ElementInterface & operator=(T && value){
      if constexpr (detail::is_callable<T>::value) {
        getValue().set<AnyFunction>(value);
      } else {
        getValue().set<T>(value);
      }
      valueChanged();
      return *this;
    }
    
    ElementInterface &operator=(const ElementInterface &e){
      getValue().setReference(e.getValue());
      return *this;
    }
    
    template <typename ... Args> Any operator()(Args && ... args) const {
      return getValue().get<AnyFunction>()(std::forward<Args>(args)...);
    }
    
    template <class T, typename ... Args> auto & set(Args && ... args) {
      auto &res = getValue().set<T>(std::forward<Args>(args)...);
      valueChanged();
      return res;
    }
    
    template <class T> T get() const { return getValue().get<T>(); }
    template <class T> T * tryGet() const { return getValue().tryGet<T>(); }

    ElementMap * asMap() const;
    ElementMapEntry operator[](const std::string &key);
    
    explicit operator bool() const { return bool(getValue()); }
    
    virtual ~ElementInterface(){}
  };
  
  class Element: public ElementInterface {
  private:
    AnyReference data;
    friend ElementMapEntry;
    
  public:
    using Map = ElementMap;
    
    Element() = default;
    Element(const ElementInterface &e):data(e.getValue()){ }

    template <
      class T,
      typename = typename std::enable_if<
        !detail::is_callable<T>::value
        && !std::is_base_of<ElementInterface, typename std::decay<T>::type>::value
      >::type
    > Element(T && v):data(std::forward<T>(v)){}

    Element(AnyFunction && f):data(f){}
    
    using ElementInterface::operator=;
    
    Any & getValue() final override { return data; }
    AnyReference getValue() const final override { return data; }
    void valueChanged() final override {};
  };

  class ElementMap: public lars::Visitable<ElementMap> {
  private:
    std::unordered_map<std::string, Element> data;
    Element *getElement(const std::string &key);
    Element &getOrCreateElement(const std::string &key);
    
  public:
    using Entry = ElementMapEntry;
    friend Entry;
    
    lars::Event<const std::string &, const ElementInterface &> onValueChanged;
    
    ElementMap(){}
    ElementMap(const ElementMap &) = delete;
    
    Entry operator[](const std::string &key);
    
    ~ElementMap(){}
  };
  
  class ElementMapEntry: public ElementInterface {
  private:
    ElementMap * parent;
    std::string key;
    Element &getOrCreateElement();
    
  public:
    ElementMapEntry(ElementMap * p, const std::string &k):parent(p), key(k){
    }

    using ElementInterface::operator=;

    Any & getValue() final override { return parent->getOrCreateElement(key).getValue(); }
    AnyReference getValue() const final override { if(auto e = parent->getElement(key)) return e->getValue(); return AnyReference(); }
    void valueChanged() final override { parent->onValueChanged.emit(key, *this); };
  };

  /**
   * Internal keys
   */
  static const std::string extendsKey = "__glue_extends";
  static const std::string classKey = "__glue_class";
  
  inline void setExtends(ElementInterface &extends, const ElementInterface &element){
    extends[extendsKey] = element;
  }
  
  template <class T> void setClass(ElementInterface &element){
    element[classKey] = lars::getTypeIndex<T>();
  }
  
  inline lars::TypeIndex * getClass(ElementInterface &element){
    return element.tryGet<lars::TypeIndex>();
  }
  
}
