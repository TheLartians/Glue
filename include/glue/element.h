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

  struct ElementInterface;
  class Map;
  class ElementMapEntry;
  
  namespace element_detail {
    /**
     * detect callable types.
     * source: https://stackoverflow.com/questions/15393938/
     */
    template <class... > using void_t = void;
    template <class T> using has_opr_t = decltype(&T::operator());
    template <class T, class = void> struct is_callable : std::false_type { };
    template <class T> struct is_callable<T, void_t<has_opr_t<typename std::decay<T>::type>>> : std::true_type { };
    
    template <class T> typename std::enable_if<
      std::is_base_of<ElementInterface, typename std::decay<T>::type>::value,
      AnyReference
    >::type convertArgument(T && arg) {
      return lars::AnyReference(arg.getValue());
    }
    template <class T> typename std::enable_if<
      !std::is_base_of<ElementInterface, typename std::decay<T>::type>::value,
      T &&
    >::type convertArgument(T && arg) {
      return std::forward<T>(arg);
    }

  }

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
      if constexpr (element_detail::is_callable<T>::value) {
        setValue(Any::create<AnyFunction>(value));
      } else {
        setValue(std::forward<T>(value));
      }
      return *this;
    }

    ElementInterface() = default;
    ElementInterface(const ElementInterface &) = default;
    ElementInterface(ElementInterface &&) = default;

    ElementInterface &operator=(const ElementInterface &e){
      setValue(e.getValue());
      return *this;
    }
    
    template <typename ... Args> Any operator()(Args && ... args) const {
      return getValue().get<AnyFunction>()(element_detail::convertArgument<Args>(std::forward<Args>(args))...);
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
    Element() = default;
    Element(const Element &) = default;
    Element(Element &&) = default;
    Element(const ElementInterface &e):data(e.getValue()){ }
    Element &operator=(const Element &) = default;
    Element &operator=(Element &&) = default;

    template <
      class T,
      typename = typename std::enable_if<
        !element_detail::is_callable<T>::value
        && !std::is_base_of<ElementInterface, typename std::decay<T>::type>::value
      >::type
    > Element(T && v):data(std::forward<T>(v)){}

    Element(AnyFunction && f):data(f){}
    
    using ElementInterface::operator=;
    
    AnyReference getValue() const final override;
    void setValue(Any&&) final override;
    Map& setToMap() final override;

    /**
     * Add values ky call chaining 
     * Note: overrided classes should implement this method
     */
    template <class T> Element & addValue(const std::string &key, T && value);

  };

  class Map: public lars::Visitable<Map>, public std::enable_shared_from_this<Map> {
  public:
    using Entry = ElementMapEntry;
    
    virtual AnyReference getValue(const std::string &key) const = 0;
    virtual void setValue(const std::string &key, Any && value) = 0;
    virtual std::vector<std::string> keys()const = 0;
    
    lars::Event<const std::string &, const Any &> onValueChanged;
    
    Map(){}
    Map(const Map &) = delete;
    
    Entry operator[](const std::string &key);

    virtual ~Map(){}
  };
  
  using ClassMaps = std::unordered_map<lars::TypeIndex, std::shared_ptr<Map>>;

  struct ElementMap: public lars::DerivedVisitable<ElementMap,Map> {
    AnyReference getValue(const std::string &key)const final override;
    void setValue(const std::string &key, Any && value)final override;
    std::vector<std::string> keys()const final override;
    std::unordered_map<std::string, Element> data;
    std::unordered_map<std::string, lars::Observer> elementObservers;
    lars::Event<const lars::TypeIndex &, const std::shared_ptr<Map> &> onClassAdded;
    void addClass(const lars::TypeIndex &, const std::shared_ptr<Map> &);
    ClassMaps classes;
  };
  
  class ElementMapEntry: public ElementInterface {
  private:
    std::shared_ptr<Map> parent;
    std::string key;
    Element &getOrCreateElement();
    
  public:
    ElementMapEntry(const std::shared_ptr<Map> &p, const std::string &k):parent(p), key(k){ }
    ElementMapEntry(const ElementMapEntry &) = default;
    ElementMapEntry(ElementMapEntry &&) = default;

    using ElementInterface::operator=;
    ElementMapEntry& operator=(const ElementMapEntry &other);

    AnyReference getValue() const final override;
    void setValue(Any&&) final override;
    Map& setToMap() final override;
  };

  /**
   * Internal keys
   */
  namespace keys {

    static const std::string constructorKey = "__new";
    static const std::string extendsKey = "__glue_extends";
    
    // TODO: unify class keys in a single classPropertyKey
    static const std::string classKey = "__glue_class";
    static const std::string constClassKey = "__glue_const_class";
    static const std::string sharedClassKey = "__glue_shared_class";
    static const std::string sharedConstClassKey = "__glue_const_shared_class";
    
    namespace operators{
      static const std::string eq = "__eq";
      static const std::string lt = "__lt";
      static const std::string le = "__le";
      static const std::string gt = "__gt";
      static const std::string ge = "__ge";
      static const std::string mul = "__mul";
      static const std::string div = "__div";
      static const std::string idiv = "__idiv";
      static const std::string add = "__add";
      static const std::string sub = "__sub";
      static const std::string mod = "__mod";
      static const std::string pow = "__pow";
      static const std::string unm = "__unm";
      static const std::string tostring = "__tostring";
    }
  }

  
  inline void setExtends(ElementInterface &extends, const ElementInterface &element){
    extends[keys::extendsKey] = element;
  }
  
  template <class T> void setClass(ElementInterface &element){
    element[keys::classKey] = lars::getTypeIndex<T>();
    element[keys::constClassKey] = lars::getTypeIndex<const T>();
    element[keys::sharedClassKey] = lars::getTypeIndex<std::shared_ptr<T>>();
    element[keys::sharedConstClassKey] = lars::getTypeIndex<std::shared_ptr<const T>>();
  }
  
  inline std::shared_ptr<lars::TypeIndex> getClass(ElementInterface &element){
    return element[keys::classKey].tryGet<lars::TypeIndex>();
  }

  template <class T> Element & Element::addValue(const std::string &key, T && value) {
    (*this)[key] = std::forward<T>(value);
    return *this;
  }

}

