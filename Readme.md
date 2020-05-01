[![Actions Status](https://github.com/TheLartians/Glue/workflows/MacOS/badge.svg)](https://github.com/TheLartians/Glue/actions)
[![Actions Status](https://github.com/TheLartians/Glue/workflows/Windows/badge.svg)](https://github.com/TheLartians/Glue/actions)
[![Actions Status](https://github.com/TheLartians/Glue/workflows/Ubuntu/badge.svg)](https://github.com/TheLartians/Glue/actions)
[![Actions Status](https://github.com/TheLartians/Glue/workflows/Style/badge.svg)](https://github.com/TheLartians/Glue/actions)
[![Actions Status](https://github.com/TheLartians/Glue/workflows/Install/badge.svg)](https://github.com/TheLartians/Glue/actions)
[![codecov](https://codecov.io/gh/TheLartians/Glue/branch/master/graph/badge.svg)](https://codecov.io/gh/TheLartians/Glue)

# Glue

A common interface for C++ to other language bindings.

## Motivation

C++ is a great language for writing algorithms and high-performance code. 
However, once it comes to building applications, servers or websites, simple things become complex and we settle for a scripting language to glue C++ components together.
The bindings usually need to be hand-crafted for every exposed type and language.
This project aims to create a generic language binding interface in pure standard C++, which allows two-way interactions, automatic declaration creation and relatively short compile times.

## API

### Maps and values

Glue is based on the [Revisited framework](https://github.com/thelartians/Revisited) and provides convenient wrapper methods.
The most important types for creating the API and are `glue::Value` and `glue::MapValue`.

```cpp
#include <glue/value.h>
#include <iostream>

void valueExample() {
  // `glue::Value`s hold an `revisited::Any` type that can store any type of value
  glue::Value value = "Hello glue!";

  // access the any type through the `->` or `*` operator
  // `as<T>()` returns an `std::optional<T>` that is defined if the cast is possible
  std::cout << *value->as<std::string>() << std::endl;

  // `glue::MapValue` is a wrapper for a container that maps strings to values
  // `glue::createAnyMap()` creates a map based on `std::unordered_set`
  // Values also accept lambda functions
  glue::MapValue map = glue::createAnyMap();
  map["myNumber"] = 42;
  map["myString"] = value;
  map["myCallback"] = [](bool a, float b){ return a ? b : -b; };
  map["myMap"] = glue::createAnyMap();

  // use helper functions to cast to maps or callbacks.
  // the result will evaluate to `false` if no cast is possible.
  if (auto f = map["myCallback"].asFunction()) {
    // callbacks are stored as a `revisited::AnyFunction` and can accept both values or `Any` arguments
    // (here `map["myNumber"]` is casted to an `Any` through the `*` operator)
    // `get<T>` returns casts the value to `T` or throws an exception if not possible
    std::cout << f(false, *map["myNumber"]).get<int>() << std::endl;
  }

  // inner maps are also `glue::MapValue`s.
  map["myMap"].asMap()["inner"] = "inner value";
}
```

### Classes

Glue also has built-in support for maps representing classes and inheritance.

```cpp
#include <glue/class.h>
#include <glue/context.h>
#include <glue/view.h>
#include <iostream>

struct A {
  std::string member;
};

struct B: public A {
  B(std::string value) : A{value} {}
  int method(int v) { return int(member.size()) + v; }
};

void classExample() {
  auto map = glue::createAnyMap();

  // `glue::createClass<T>` is a convience function for declaring maps for class APIs
  // `addConstructor<Args...>()` adds a function that constructs `A` given the argument types `Args...`
  // `.addMember("name", &T::member)` adds a setter (`member`) and getter (`setMember`) function
  map["A"] = glue::createClass<A>()
    .addConstructor<>()
    .addMember("member", &A::member)
  ;

  // classes can be made inheritance aware
  // `setExtends(map)` uses the argument map or callback to retrieve undefined keys
  // `glue::WithBases<A>()` adds implicit conversions to the listed base classes
  // `addMethod("name", &T::method)` adds a method that calls a member function or lambda
  map["B"] = glue::createClass<B>(glue::WithBases<A>())
    .setExtends(map["A"])
    .addConstructor<std::string>()
    .addMethod("method", &B::method)
    .addMethod("lambda", [](const B &b, int x){ return b.member.size() + x; })
  ;

  // contexts collect map class data and can be used to test instance creation
  glue::Context context;
  context.addRootMap(map);

  // `glue::View` allows convinient introspection into maps
  // `glue::Instance` captures a value and behaves as a class instance
  glue::View view(map);
  auto b = context.createInstance(view["B"][glue::keys::constructorKey]("arg"));

  // calls will be treated as member functions
  std::cout << b["member"]().get<std::string>() << std::endl;
  b["setMember"]("new value");
  std::cout << b["lambda"](10).get<int>() << std::endl;
}
```

### Declarations

Glue can automatically generate declarations for type-safe scripting using TypeScript or [TypeScriptToLua](https://typescripttolua.github.io).

```cpp
glue::DeclarationPrinter printer;
printer.init();
printer.print(std::cout, map, &context);
```

In the example above, this would result in the following declarations.

```ts
/** @customConstructor A.__new */
declare class A {
  constructor()
  member(): string
  setMember(arg1: string): void
}
/** @customConstructor B.__new */
declare class B extends A {
  constructor(arg0: string)
  lambda(arg1: number): number
  method(arg1: number): number
}
```

## Supported bindings

Here you can find current planned and supported bindings.

- [x] Lua: [GlueLua](https://github.com/TheLartians/LuaGlue)
- [ ] Emscripten
- [ ] Duktape
- [ ] Python
- [ ] Node

## Usage

Glue can be easily added to your project through [CPM.cmake](https://github.com/TheLartians/CPM.cmake).

```cmake
CPMAddPackage(
  NAME Glue
  VERSION 1.0
  GIT_REPOSITORY https://github.com/TheLartians/Glue.git
)

target_link_libraries(myLibrary Glue)
```
