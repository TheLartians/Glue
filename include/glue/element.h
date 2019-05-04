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
  
  class Map;
  class ElementMapEntry;
  
  struct ElementInterface {
    virtual AnyReference getValue() const = 0;
    virtual void setValue(Any &&) = 0;
    virtual Map& setToMap() = 0;
    
    template <
      class T,
      typename = typename std::enable_if<
        !std::is_base_of<ElementInterface, typename std::decay<T>::type>::value
      >::type
    > ElementInterface & operator=(T && value){
      if constexpr (detail::is_callable<T>::value) {
        setValue(Any::create<AnyFunction>(value));
      } else {
        setValue(lars::Any(value));
      }
      return *this;
    }
    
    ElementInterface &operator=(const ElementInterface &e){
      setValue(e.getValue());
      return *this;
    }
    
    template <typename ... Args> Any operator()(Args && ... args) const {
      return getValue().get<AnyFunction>()(std::forward<Args>(args)...);
    }
    
    template <class T, typename ... Args> auto & set(Args && ... args) {
      Any value;
      auto &res = value.set<T>(std::forward<Args>(args)...);
      setValue(std::move(value));
      return res;
    }
    
    template <class T> T get() const { return getValue().get<T>(); }
    template <class T> std::shared_ptr<T> tryGet() const { return getValue().getShared<T>(); }

    std::shared_ptr<Map> asMap() const;
    ElementMapEntry operator[](const std::string &key);
    
    explicit operator bool() const { return bool(getValue()); }
    
    virtual ~ElementInterface(){}
  };
  
  class Element: public ElementInterface {
  private:
    mutable AnyReference data;
    friend ElementMapEntry;
    
  public:
    using Map = Map;
    
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
    
    AnyReference getValue() const final override;
    void setValue(Any&&) final override;
    Map& setToMap() final override;
  };

  class Map: public lars::Visitable<Map>, public std::enable_shared_from_this<Map> {
  public:
    using Entry = ElementMapEntry;
    
    virtual AnyReference getValue(const std::string &key) const = 0;
    virtual void setValue(const std::string &key, Any && value) = 0;
    virtual std::vector<std::string> keys()const = 0;
    
    lars::Event<const std::string &, const ElementInterface &> onValueChanged;
    
    Map(){}
    Map(const Map &) = delete;
    Entry operator[](const std::string &key);
    
    virtual ~Map(){}
  };
  
  class ElementMap: public Map {
    AnyReference getValue(const std::string &key)const final override;
    void setValue(const std::string &key, Any && value)final override;
    std::vector<std::string> keys()const final override;

    std::unordered_map<std::string, Element> data;
  };
  
  class ElementMapEntry: public ElementInterface {
  private:
    std::shared_ptr<Map> parent;
    std::string key;
    Element &getOrCreateElement();
    
  public:
    ElementMapEntry(const std::shared_ptr<Map> &p, const std::string &k):parent(p), key(k){
    }

    using ElementInterface::operator=;

    AnyReference getValue() const final override;
    void setValue(Any&&) final override;
    Map& setToMap() final override;
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
  
  inline std::shared_ptr<lars::TypeIndex> getClass(ElementInterface &element){
    return element[classKey].tryGet<lars::TypeIndex>();
  }
  
}
