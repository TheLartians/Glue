#pragma once

#include <glue/class.h>
#include <glue/context.h>
#include <glue/value.h>

#include <ostream>

namespace glue {

  struct DeclarationPrinter {
    struct State {
      size_t depth = 0;
      Context *context = nullptr;
      std::unordered_map<TypeIndex, std::string> typeNames;
      std::optional<ClassInfo> currentClass;
    };

    std::unordered_map<TypeIndex, std::string> internalTypeNames;

    /**
     * For special keys inside a map.
     * returns `false` if nothing printed.
     */
    using KeyPrinter = std::function<bool(std::ostream &stream, const std::string &, const Value &,
                                          State &state)>;
    std::unordered_map<std::string, KeyPrinter> keyPrinters;

    /**
     * must be called after creating the printer
     */
    virtual void init();

    virtual std::string getLocalTypeName(const Context::TypeInfo &, State &state) const;
    virtual std::string getUnknownTypeName(const TypeID &, State &state) const;

    virtual void printLineBreak(std::ostream &stream, State &state) const;
    virtual void printIndent(std::ostream &stream, State &state) const;
    virtual void printValue(std::ostream &stream, const std::string &name, const Value &,
                            State &state) const;
    virtual void printTypeName(std::ostream &stream, const TypeID &, State &state) const;
    virtual void printFunction(std::ostream &stream, const std::string &name, const AnyFunction &,
                               State &state) const;
    virtual void printConstructor(std::ostream &stream, const AnyFunction &, State &state) const;
    virtual void printMemberFunction(std::ostream &stream, const std::string &name,
                                     const AnyFunction &, State &state) const;
    virtual void printMap(std::ostream &stream, const std::string &name, const MapValue &,
                          State &state) const;
    virtual void printInnerBlock(std::ostream &stream, const MapValue &, State &state) const;
    virtual void printClassMap(std::ostream &stream, const std::string &name, const MapValue &,
                               State &state) const;

    virtual void print(std::ostream &stream, const MapValue &value,
                       Context *context = nullptr) const;

    virtual ~DeclarationPrinter() {}
  };

}  // namespace glue
