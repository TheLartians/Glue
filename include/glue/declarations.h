#pragma once

#include <glue/element.h>

#include <ostream>
#include <string>
#include <unordered_map>

namespace glue{

  class TypescriptDeclarations {
  public:
    TypescriptDeclarations();
    void addMap(const std::shared_ptr<Map> &, std::vector<std::string> &context);
    void addElement(const ElementInterface &, std::vector<std::string> &context);
    void addElement(const ElementInterface &);

    struct Context {
      unsigned depth = 0;
      lars::TypeIndex type = lars::getTypeIndex<void>();
      lars::TypeIndex sharedType = lars::getTypeIndex<void>();
      Context(){}
      std::string indent()const{ return std::string(depth*2,' '); }
      bool hasType() const { return type != lars::getTypeIndex<void>(); }
    };

    std::ostream &printElement(std::ostream &, const std::string &name, const ElementInterface &, const Context & = Context())const;
    void printNamespace(std::ostream &, const std::string &, const std::shared_ptr<Map> &, Context &)const;
    void printClass(std::ostream &, const std::string &, const std::shared_ptr<Map> &, Context &)const;
    void printFunction(std::ostream &, const std::string &name, const lars::AnyFunction &f, const Context & context)const;
    void printFunctionArguments(std::ostream &, const lars::AnyFunction &f, bool memberFunction = false, bool constructor = false)const;
    void printValue(std::ostream &, const std::string &name, const lars::AnyReference &v, const Context & context)const;
    void printType(std::ostream &, const lars::TypeIndex &type)const;

  private:
    std::unordered_map<lars::TypeIndex, std::string> typenames;
  };

  std::string getTypescriptDeclarations(const std::string &, const ElementInterface &);

}
