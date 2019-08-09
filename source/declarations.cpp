#include <glue/declarations.h>
#include <lars/type_index.h>
#include <lars/iterators.h>
#include <easy_iterator.h>

#include <unordered_map>
#include <sstream>
#include <assert.h>

using namespace glue;

namespace {
  std::vector<std::string> sorted(std::vector<std::string> && values){
    std::sort(values.begin(), values.end());
    return std::move(values);
  }
}

TypescriptDeclarations::TypescriptDeclarations(){
  typenames[lars::getTypeIndex<bool>()] = "boolean";
  typenames[lars::getTypeIndex<short>()] = "number";
  typenames[lars::getTypeIndex<int>()] = "number";
  typenames[lars::getTypeIndex<unsigned>()] = "number";
  typenames[lars::getTypeIndex<size_t>()] = "number";
  typenames[lars::getTypeIndex<float>()] = "number";
  typenames[lars::getTypeIndex<double>()] = "number";
  typenames[lars::getTypeIndex<void>()] = "void";
  typenames[lars::getTypeIndex<std::string>()] = "string";
  typenames[lars::getTypeIndex<lars::AnyFunction>()] = "(this: void, ...args: any[]) => any";
}

void TypescriptDeclarations::addMap(const std::shared_ptr<Map> &map, std::vector<std::string> &context) {
  if (context.size() > 0) {
    auto addClassKey = [&](const std::string &key){
      if (auto c = (*map)[key]) {
        std::string name;
        for (auto &str: context) {
          name += str;
          name += ".";
        }
        name.pop_back();
        typenames[c.get<lars::TypeIndex>()] = name;
      }
    };
    addClassKey(keys::classKey);
    addClassKey(keys::sharedClassKey);
    addClassKey(keys::constClassKey);
    addClassKey(keys::sharedConstClassKey);
  }
  for (auto key: map->keys()) {
    if (auto m = (*map)[key].asMap()) {
      context.push_back(key);
      addMap(m, context);
      context.pop_back();
    }
  }
}

void TypescriptDeclarations::addElement(const ElementInterface &e, std::vector<std::string> &context) {
  if (auto map = e.asMap()) {
    addMap(map,context);
  }
}

void TypescriptDeclarations::addElement(const ElementInterface &e) {
  std::vector<std::string> context;
  addElement(e, context);
}

void TypescriptDeclarations::printNamespace(std::ostream &stream, const std::string &name, const std::shared_ptr<Map> &map, Context & context)const{
  stream << context.indent() << "module " << name << " {\n";
  ++context.depth;
  for (auto key: sorted(map->keys())) {
    printElement(stream,key,(*map)[key],context);
  }
  --context.depth;
  stream << context.indent() << "}";
}

void TypescriptDeclarations::printClass(std::ostream &stream, const std::string &name, const std::shared_ptr<Map> &map, Context & context)const{  
  assert(context.hasType());
  
  if (auto et = (*map)[keys::constructorKey]) {
    stream << context.indent() << "/** @customConstructor ";
    printType(stream,context.type);
    stream << "." << keys::constructorKey << " */\n";
  }

  stream << context.indent() << "class " << name;

  if (auto et = (*map)[keys::extendsKey]) {
    stream << " extends ";
    printType(stream, et[keys::classKey].get<lars::TypeIndex>());
  }
  stream << " {\n";
  ++context.depth;

  for (auto key: sorted(map->keys())) {
    if (key == keys::extendsKey || key == keys::classKey || key == keys::constClassKey || key == keys::sharedConstClassKey || key == keys::sharedClassKey) {
      continue;
    }

    if (key == keys::constructorKey) { // print constructor
      auto f = (*map)[key].get<lars::AnyFunction>(); 
      stream << context.indent() << "constructor(";
      printFunctionArguments(stream, f, false, true);
      stream << ")\n";
    } else {
      printElement(stream,key,(*map)[key], context);
    }
  }
  --context.depth;
  stream << context.indent() << "}";
}

std::ostream &TypescriptDeclarations::printElement(std::ostream &stream, const std::string &name, const ElementInterface &element, const Context & context) const {
  if (context.depth == 0) {
    stream << context.indent() << "declare ";
  }

  if (auto map = element.asMap()) {
    Context innerContext = context;
    if (auto c = (*map)[keys::classKey]) {
      innerContext.type = c.get<lars::TypeIndex>();
      if (auto sc = (*map)[keys::sharedClassKey]) {
        innerContext.sharedType = sc.get<lars::TypeIndex>();
      }
      if (auto sc = (*map)[keys::constClassKey]) {
        innerContext.constType = sc.get<lars::TypeIndex>();
      }
      if (auto sc = (*map)[keys::sharedConstClassKey]) {
        innerContext.sharedConstType = sc.get<lars::TypeIndex>();
      }
      printClass(stream, name, map, innerContext);
    } else {
      printNamespace(stream, name, map, innerContext);
    }
  } else {
    auto value = element.getValue();
    stream << context.indent();
    if (auto f = value.tryGet<lars::AnyFunction>()) {
      printFunction(stream,name,*f, context);
    } else {
      printValue(stream,name, value, context);
    }
    stream << ";";
  }
  stream << "\n";
  return stream;
}

void TypescriptDeclarations::printType(std::ostream &stream, const lars::TypeIndex &type)const {
  auto it = typenames.find(type);
  if (it != typenames.end()) {
    stream << it->second;
  } else {
    stream << "any /* (" << type << ") */";
  }
}

void TypescriptDeclarations::printFunction(std::ostream &stream, const std::string &name, const lars::AnyFunction &f, const Context & context)const {
  bool memberFunction = false;
  if (context.hasType()) {
    if (
      f.argumentCount() > 0 
      && (
        f.argumentType(0) == context.type 
        || f.argumentType(0) == context.sharedType 
        || f.argumentType(0) == context.constType 
        || f.argumentType(0) == context.sharedConstType
      )
    ) {
      memberFunction = true;
    } else if (!f.isVariadic()) {
      stream << "static ";
    }
  } else {
    stream << "function ";
  }
  stream << name;
  stream << "(";
  printFunctionArguments(stream, f, memberFunction);
  stream << "): ";
  printType(stream, f.returnType());
}

void TypescriptDeclarations::printFunctionArguments(std::ostream &stream, const lars::AnyFunction &f, bool memberFunction, bool constructor)const {
  if (f.isVariadic()) {
   stream << "...args: any[]"; 
  } else {
    auto N = f.argumentCount();
    if (!memberFunction && !constructor) {
      stream << "this: void";
      if (N > 0) { stream << ", "; }
    }
    for (auto i: easy_iterator::range(N)) {
      if (i == 0 && memberFunction) {
        continue;
      }
      stream << "arg" << i << ": ";
      printType(stream, f.argumentType(i));
      if (i+1 != N) { stream << ", "; }
    }
  }
}

void TypescriptDeclarations::printValue(std::ostream &stream, const std::string &name, const lars::AnyReference &v, const Context &context)const {
  if (context.hasType()) {
    stream << "static " << name << ": ";  
  } else {
    stream << "let " << name << ": ";
  }
  printType(stream, v.type());
}

std::string glue::getTypescriptDeclarations(const std::string &name, const ElementInterface &element){
  TypescriptDeclarations definitions;
  std::vector<std::string> context{name};
  definitions.addElement(element, context);
  std::stringstream stream;
  definitions.printElement(stream, name, element);
  return stream.str();
}
