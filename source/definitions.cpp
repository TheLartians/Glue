#include <glue/definitions.h>
#include <lars/type_index.h>
#include <lars/iterators.h>
#include <easy_iterator.h>

#include <unordered_map>
#include <sstream>

using namespace glue;

namespace {
  std::vector<std::string> sorted(std::vector<std::string> && values){
    std::sort(values.begin(), values.end());
    return std::move(values);
  }
}

TypeScriptDefinitions::TypeScriptDefinitions(){
  typenames[lars::getTypeIndex<bool>()] = "bool";
  typenames[lars::getTypeIndex<int>()] = "number";
  typenames[lars::getTypeIndex<unsigned>()] = "number";
  typenames[lars::getTypeIndex<size_t>()] = "number";
  typenames[lars::getTypeIndex<float>()] = "number";
  typenames[lars::getTypeIndex<double>()] = "number";
  typenames[lars::getTypeIndex<void>()] = "void";
  typenames[lars::getTypeIndex<std::string>()] = "string";
}

void TypeScriptDefinitions::addMap(const std::shared_ptr<Map> &map, std::vector<std::string> &context) {
  if (context.size() > 0) {
    if (auto c = (*map)[classKey]) {
      std::string name;
      for (auto &str: context) {
        name += str;
        name += ".";
      }
      name.pop_back();
      typenames[c.get<lars::TypeIndex>()] = name;
    }
  }
  for (auto key: map->keys()) {
    if (key == extendsKey || key == classKey) {
      continue;
    }

    if (auto m = (*map)[key].asMap()) {
      context.push_back(key);
      addMap(m, context);
      context.pop_back();
    }
  }
}

void TypeScriptDefinitions::addElement(const ElementInterface &e, std::vector<std::string> &context) {
  if (auto map = e.asMap()) {
    addMap(map,context);
  }
}

void TypeScriptDefinitions::addElement(const ElementInterface &e) {
  std::vector<std::string> context;
  addElement(e, context);
}

void TypeScriptDefinitions::printNamespace(std::ostream &stream, const std::string &name, const std::shared_ptr<Map> &map, Context & context)const{
  stream << context.indent() << "namespace " << name << " {\n";
  ++context.depth;
  for (auto key: sorted(map->keys())) {
    printElement(stream,key,(*map)[key],context);
  }
  --context.depth;
  stream << context.indent() << "}";
}

void TypeScriptDefinitions::printClass(std::ostream &stream, const std::string &name, const std::shared_ptr<Map> &map, Context & context)const{  
  assert(context.hasType());
  stream << context.indent() << "class " << name;
  if (auto et = (*map)[extendsKey]) {
    stream << " extends ";
    printType(stream, et[classKey].get<lars::TypeIndex>());
  }
  stream << " {\n";
  ++context.depth;
  for (auto key: sorted(map->keys())) {
    if (key != extendsKey && key != classKey) {
      printElement(stream,key,(*map)[key], context);
    }
  }
  --context.depth;
  stream << context.indent() << "}";
}

std::ostream &TypeScriptDefinitions::printElement(std::ostream &stream, const std::string &name, const ElementInterface &element, const Context & context) const {
  if (context.depth == 0) {
    stream << context.indent() << "declare ";
  }

  if (auto map = element.asMap()) {
    Context innerContext = context;
    if (auto c = (*map)[classKey]) {
      innerContext.type = c.get<lars::TypeIndex>();
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

void TypeScriptDefinitions::printType(std::ostream &stream, const lars::TypeIndex &type)const {
  auto it = typenames.find(type);
  if (it != typenames.end()) {
    stream << it->second;
  } else {
    stream << "any /* (" << type << ") */";
  }
}

void TypeScriptDefinitions::printFunction(std::ostream &stream, const std::string &name, const lars::AnyFunction &f, const Context & context)const {
  bool memberFunction = false;
  if (context.hasType()) {
    if (f.argumentCount() > 0 && f.argumentType(0) == context.type) {
      memberFunction = true;
    } else {
      stream << "static ";
    }
  } else {
    stream << "function ";
  }
  stream << name;
  stream << "(";
  auto N = f.argumentCount();
  if (!memberFunction) {
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
  stream << "): ";
  printType(stream, f.returnType());
}

void TypeScriptDefinitions::printValue(std::ostream &stream, const std::string &name, const lars::AnyReference &v, const Context &)const {
  stream << "let " << name << ": ";
  printType(stream, v.type());
}
