#pragma once

#include <revisited/visitor.h>

namespace glue {

  namespace detail {

    template <typename... Args> using TypeList = revisited::TypeList<Args...>;

    /**
     * A custom visitable that can be converted to base types and captures value by references
     */
    template <class T, class B, class C, class CastType>
    class ReferenceVisitableWithBasesAndConversionsDefinition;

    template <class T, typename... Bases, typename... Conversions, class CastType>
    class ReferenceVisitableWithBasesAndConversionsDefinition<T, TypeList<Bases...>,
                                                              TypeList<Conversions...>, CastType> {
    private:
      using ConstTypes =
          typename TypeList<const T &, const Bases &..., Conversions...>::template Merge<
              typename TypeList<T, Bases...>::template Filter<std::is_copy_constructible>>;
      using Types = typename std::conditional<std::is_const<T>::value, TypeList<>,
                                              TypeList<T &, Bases &...>>::type;

    public:
      using type
          = revisited::DataVisitablePrototype<T &, typename Types::template Merge<ConstTypes>,
                                              ConstTypes, CastType>;
    };

  }  // namespace detail

}  // namespace glue