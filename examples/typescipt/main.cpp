#include <glue/class_element.h>
#include <glue/declarations.h>
#include <glue/lua.h>
#include <iostream>
#include <string>
#include <cxxopts.hpp>

namespace lib {
  
  struct A {
    int data;
    explicit A(int x):data(x){}
    A add(const A &other){ return A(other.data + data); }
  };

  struct B: public A {
    std::string name;
    using A::A;
  };

}

// allow implicit casting of B to A in Any values
template <> struct lars::AnyVisitable<lib::B> {
  using type = lars::DataVisitableWithBases<lib::B, lib::A>;
};

int main(int argc, char ** argv) {

  // parse command line options

  cxxopts::Options options("Glue Typescript Example");
  options.add_options()
    ("h,help", "Show help")
    ("d,declarations", "Print typescript declarations")
    ("s,script", "path to main lua script", cxxopts::value<std::string>());

  auto opts = options.parse(argc, argv);

  if (opts.arguments().size() == 0 || opts["help"].as<bool>()) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  // define glue::Element

  glue::Element lib;

  lib["log"] = [](std::string message){ 
    std::cout << "logged: " << message << std::endl;
  };

  lib["A"] = glue::ClassElement<lib::A>()
  .addConstructor<int>("create")
  .addMember("data", &lib::A::data)
  .addMethod("add", &lib::A::add)
  .addFunction("next", [](const lib::A &a){ return lib::A(a.data+1); })
  .addFunction("__tostring", [](const lib::A &a){ return "lib::A(" + std::to_string(a.data) + ")"; })
  ;

  lib["B"] = glue::ClassElement<lib::B>()
  .addConstructor<int>("create")
  .addMember("name", &lib::B::name)
  .setExtends(lib["A"])
  ;

  if (opts["declarations"].as<bool>()) {
    std::cout << glue::getTypescriptDeclarations("lib", lib) << std::endl;
  }

  // run lua

  if (opts["script"].count() > 0) {
    glue::LuaState lua;
    lua.openStandardLibs();
    lua["lib"] = lib;
    lua.runFile(opts["script"].as<std::string>());
  }
  
  return 0;
}
