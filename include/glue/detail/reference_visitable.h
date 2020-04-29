#pragma once

#include <revisited/visitor.h>

#include <memory>

namespace glue {

  namespace detail {

    /**
     * Capture a shared pointer but act like a reference
     * Should only be used for the storage container in the visitable type below.
     */
    template <class T> struct SharedReference {
      std::shared_ptr<T> ptr;
      SharedReference(std::shared_ptr<T> p) : ptr(std::move(p)) {}
      operator T &() { return *ptr; }
      operator const T &() const { return *ptr; }
    };

    template <typename... Args> using TypeList = revisited::TypeList<Args...>;

    /**
     * A custom visitable that can be converted to base types and captures value by references
     * @param T the type to hold a shared reference to
     * @param B the visitable base classes of the type
     * @param C supported implicit conversion types
     * @param PublicType the outside type stored by the visitable
     */
    template <class T, class B, class C, class PublicType> class SharedReferenceVisitable;

    template <class T, typename... Bases, typename... Conversions, class PublicType>
    class SharedReferenceVisitable<T, TypeList<Bases...>, TypeList<Conversions...>, PublicType> {
    private:
      using ConstTypes =
          typename TypeList<const T &, const Bases &..., Conversions...>::template Merge<
              typename TypeList<T, Bases...>::template Filter<std::is_copy_constructible>>;
      using Types = typename std::conditional<std::is_const<T>::value, TypeList<>,
                                              TypeList<T &, Bases &...>>::type;

    public:
      using type = revisited::DataVisitablePrototype<
          SharedReference<T>, typename Types::template Merge<ConstTypes>, ConstTypes, PublicType>;
    };

  }  // namespace detail

}  // namespace glue