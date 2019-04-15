#include <lars/glue/definitions.h>
#include <lars/type_index.h>
#include <lars/iterators.h>
#include <unordered_map>
#include <sstream>

namespace {
  void add_typescript_type_name(std::ostream &stream, const lars::TypeIndex &type, const std::unordered_map<lars::TypeIndex, std::string> &typenames){
    if (
      type == lars::get_type_index<bool>()
      || type == lars::get_type_index<int>()
      || type == lars::get_type_index<unsigned>()
      || type == lars::get_type_index<size_t>()
      || type == lars::get_type_index<float>()
      || type == lars::get_type_index<double>()
    ) {
      stream << "number";
    } else if (type == lars::get_type_index<std::string>()) {
      stream << "string";
    } else {
      auto it = typenames.find(type);
      if (it != typenames.end()) {
        stream << it->second;
      } else {
        stream << type.name();
      }
    }
  }


  void add_typenames(const lars::Extension &extension, std::unordered_map<lars::TypeIndex, std::string> &typenames){
    for (auto [name, ext]: extension.extensions()) {
      auto &type = ext->class_type();
      if (type) { typenames[*type] = name; }
      add_typenames(*ext, typenames);
    }
  }

  void add_typescript_class_definitions(std::ostream &stream, const lars::Extension &extension, std::unordered_map<lars::TypeIndex, std::string> &typenames, const std::string &indent, const lars::TypeIndex &class_type){
    for (auto [name, f]: extension.functions()) {
      stream << indent;
      auto member_function = f.argument_count() > 0 && f.argument_type(0) == class_type;
      if (!member_function) {
        stream << "static ";
      }
      auto return_type = f.return_type();
      stream << name << '(';
      auto argc = f.argument_count();
      for (auto i: lars::range(member_function ? 1 : 0, argc)) {
        stream << "arg" << i+1 << ": ";
        add_typescript_type_name(stream, f.argument_type(i), typenames);
        if (i+1 != argc) { stream << ", "; }
      }
      stream << "):";
      add_typescript_type_name(stream, return_type, typenames);
      stream << ";\n";
    }
  }

  void add_typescript_namespace_definitions(std::ostream &stream, const lars::Extension &extension, std::unordered_map<lars::TypeIndex, std::string> &typenames, const std::string &indent){

    auto inner_indent = indent + "  ";
    for (auto [name, ext]: extension.extensions()) {
      auto &type = ext->class_type();
      auto &base_type = ext->base_class_type();
      if (type) {
        stream << indent << "class " << name;
        if (base_type) {
          stream << " extends ";
          add_typescript_type_name(stream, *base_type, typenames);
        }
        stream << "{\n";
        add_typescript_class_definitions(stream, *ext, typenames, inner_indent, *type);
        stream << indent << "}\n";
      } else {
        stream << indent << "namespace " << name << " {\n";
        add_typescript_namespace_definitions(stream, *ext, typenames, inner_indent);
        stream << indent << "}\n";
      }
    }

    for (auto [name, f]: extension.functions()) {
      stream << indent;
      auto return_type = f.return_type();
      stream << "function " << name << '(';
      auto argc = f.argument_count();
      for (auto i: lars::range(argc)) {
        stream << "arg" << i+1 << ": ";
        add_typescript_type_name(stream, f.argument_type(i), typenames);
        if (i+1 != argc) { stream << ", "; }
      }
      stream << "):";
      add_typescript_type_name(stream, return_type, typenames);
      stream << ";\n";
    }
  }

}

std::string lars::get_typescript_definitions(const lars::Extension &extension){
  std::unordered_map<lars::TypeIndex, std::string> typenames;
  add_typenames(extension, typenames);
  std::stringstream stream;
  add_typescript_namespace_definitions(stream, extension, typenames, "");
  return stream.str();
}
