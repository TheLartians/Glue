#include <easy_iterator.h>
#include <glue/declarations.h>
#include <glue/keys.h>

#include <algorithm>
#include <string>

using namespace glue;

void DeclarationPrinter::print(std::ostream &stream, const MapValue &value,
                               Context *context) const {
  State state;
  state.context = context;
  printInnerBlock(stream, value, state);
}

void DeclarationPrinter::printValue(std::ostream &stream, const std::string &name,
                                    const Value &value, State &state) const {
  if (state.depth == 0) {
    stream << "declare ";
  }
  stream << "const " << name << ": ";
  printTypeName(stream, value->type(), state);
}

std::string DeclarationPrinter::getLocalTypeName(const Context::TypeInfo &info, State &) const {
  std::string typeName;
  bool initial = true;
  for (auto &&p : info.path) {
    if (!initial) typeName += ".";
    initial = false;
    typeName += p;
  }
  return typeName;
}

std::string DeclarationPrinter::getUnknownTypeName(const TypeID &type, State &) const {
  return "any /* " + std::string(type.name) + " */";
}

void DeclarationPrinter::printTypeName(std::ostream &stream, const TypeID &type,
                                       State &state) const {
  if (auto name1 = easy_iterator::find(state.typeNames, type.index)) {
    stream << name1->second;
  } else if (auto name2 = easy_iterator::find(internalTypeNames, type.index)) {
    stream << name2->second;
  } else {
    auto info = state.context ? state.context->getTypeInfo(type.index) : nullptr;
    auto typeName = info ? getLocalTypeName(*info, state) : getUnknownTypeName(type, state);
    state.typeNames[type.index] = typeName;
    stream << typeName;
  }
}

void DeclarationPrinter::printFunction(std::ostream &stream, const std::string &name,
                                       const AnyFunction &f, State &state) const {
  if (state.depth == 0) {
    stream << "declare ";
  }
  stream << "const " << name << ": (this: void";
  auto N = f.argumentCount();
  for (size_t i = 0; i < N; ++i) {
    stream << ", ";
    stream << "arg" << i << ": ";
    printTypeName(stream, f.argumentType(i), state);
  }
  stream << ") => ";
  printTypeName(stream, f.returnType(), state);
}

void DeclarationPrinter::printMemberFunction(std::ostream &stream, const std::string &name,
                                             const AnyFunction &f, State &state) const {
  auto N = f.argumentCount();
  bool isStatic = N == 0
                  || (f.argumentType(0) != state.currentClass->typeID
                      && f.argumentType(0) != state.currentClass->constTypeID
                      && f.argumentType(0) != state.currentClass->sharedTypeID
                      && f.argumentType(0) != state.currentClass->sharedConstTypeID);
  bool initial = true;
  if (isStatic) {
    stream << "static " << name;
    stream << "(this: void";
    initial = false;
  } else {
    stream << name << '(';
  }
  for (size_t i = 1; i < N; ++i) {
    if (!initial) stream << ", ";
    initial = false;
    stream << "arg" << i << ": ";
    printTypeName(stream, f.argumentType(i), state);
  }
  stream << "): ";
  printTypeName(stream, f.returnType(), state);
}

void DeclarationPrinter::printConstructor(std::ostream &stream, const AnyFunction &f,
                                          State &state) const {
  stream << "constructor(";
  auto N = f.argumentCount();
  bool initial = true;
  for (size_t i = 0; i < N; ++i) {
    if (!initial) stream << ", ";
    initial = false;
    stream << "arg" << i << ": ";
    printTypeName(stream, f.argumentType(i), state);
  }
  stream << ")";
}

void DeclarationPrinter::printIndent(std::ostream &stream, State &state) const {
  stream << std::string(state.depth * 2, ' ');
}

void DeclarationPrinter::printLineBreak(std::ostream &stream, State &) const { stream << '\n'; }

void DeclarationPrinter::printMap(std::ostream &stream, const std::string &name,
                                  const MapValue &value, State &state) const {
  stream << "module " << name << " " << '{';
  printLineBreak(stream, state);
  state.depth++;
  printInnerBlock(stream, value, state);
  state.depth--;
  printLineBreak(stream, state);
  printIndent(stream, state);
  stream << '}';
}

void DeclarationPrinter::printClassMap(std::ostream &stream, const std::string &name,
                                       const MapValue &value, State &state) const {
  stream << "/** @customConstructor ";
  printTypeName(stream, value[keys::classKey]->get<ClassInfo>().typeID, state);
  stream << ".__new"
         << " */";
  printLineBreak(stream, state);
  printIndent(stream, state);
  if (state.depth == 0) {
    stream << "declare ";
  }
  stream << "class " << name;
  if (auto extends = value[keys::extendsKey]) {
    if (auto extendedMap = Value(*extends).asMap()) {
      if (auto extendedMapClass = extendedMap[keys::classKey]) {
        stream << " extends ";
        printTypeName(stream, extendedMapClass->get<ClassInfo>().typeID, state);
      }
    }
  }
  stream << " {";
  printLineBreak(stream, state);
  state.depth++;
  printInnerBlock(stream, value, state);
  state.depth--;
  printLineBreak(stream, state);
  printIndent(stream, state);
  stream << '}';
}

void DeclarationPrinter::printInnerBlock(std::ostream &stream, const MapValue &value,
                                         State &state) const {
  bool initial = true;
  bool needsBreak = true;

  auto keys = value.keys();
  std::sort(keys.begin(), keys.end());

  for (auto &&k : keys) {
    auto v = value[k];
    if (initial) {
      initial = false;
      printIndent(stream, state);
    } else if (needsBreak) {
      stream << '\n';
      printIndent(stream, state);
    }
    if (auto keyPrinter = easy_iterator::find(keyPrinters, k)) {
      needsBreak = keyPrinter->second(stream, k, v, state);
    } else {
      if (auto m = v.asMap()) {
        if (auto classInfo = m[keys::classKey]) {
          state.currentClass = classInfo->template get<ClassInfo>();
          printClassMap(stream, k, m, state);
          state.currentClass = std::nullopt;
        } else {
          printMap(stream, k, m, state);
        }
      } else if (auto f = v.asFunction()) {
        if (state.currentClass) {
          if (k == keys::constructorKey) {
            printConstructor(stream, f, state);
          } else {
            printMemberFunction(stream, k, f, state);
          }
        } else {
          printFunction(stream, k, f, state);
        }
      } else {
        printValue(stream, k, v, state);
      }
      needsBreak = true;
    }
  };
}

namespace {
  template <class T>
  void addInternalTypeName(DeclarationPrinter &printer, const std::string &name) {
    printer.internalTypeNames[getTypeIndex<T>()] = name;
    printer.internalTypeNames[getTypeIndex<const T>()] = name;
  }

  template <class T, class T2, class... Rest>
  void addInternalTypeName(DeclarationPrinter &printer, const std::string &name) {
    addInternalTypeName<T>(printer, name);
    addInternalTypeName<T2, Rest...>(printer, name);
  }
}  // namespace

void DeclarationPrinter::init() {
  addInternalTypeName<char, unsigned char, short int, unsigned short int, int, unsigned int,
                      long int, unsigned long int, long long int, unsigned long long int, float,
                      double, long double>(*this, "number");
  addInternalTypeName<std::string>(*this, "string");
  internalTypeNames[getTypeIndex<void>()] = "void";

  keyPrinters[keys::classKey] = [](auto &&, auto &&, auto &&, auto &&) { return false; };
  keyPrinters[keys::extendsKey] = [](auto &&, auto &&, auto &&, auto &&) { return false; };
}
