#include <glue/extension.h>

using namespace glue;

struct Extension::Data {
  std::unordered_map<std::string, Member> members;
  lars::Event<const std::string &, const Member &> onMemberChanged;
};

Extension::Extension():data(std::make_shared<Data>()),onMemberChanged(data->onMemberChanged){
  
}

Extension::Member * Extension::getMember(const std::string &key){
  auto it = data->members.find(key);
  if (it != data->members.end()) {
    return &it->second;
  } else {
    return nullptr;
  }
}

const Extension::Member * Extension::getMember(const std::string &key)const{
  auto it = data->members.find(key);
  if (it != data->members.end()) {
    return &it->second;
  } else {
    auto extends = data->members.find("__extends");
    if (extends != data->members.end()) {
      return extends->second.asExtension().getMember(key);
    }
    return nullptr;
  }
}


Extension::Member &Extension::getOrCreateMember(const std::string &key){
  return data->members[key];
}

Extension::MemberDelegate Extension::operator[](const std::string &key){
  return MemberDelegate(this, key);
}

const Extension::Member &Extension::operator[](const std::string &key)const{
  if (auto member = getMember(key)) {
    return *member;
  } else {
    throw Extension::MemberNotFoundException(key);
  }
}

const char * Extension::MemberNotFoundException::what()const noexcept{
  if (buffer.size() == 0) {
    buffer = "extension contains no function '" + name + "'.";
  }
  return buffer.c_str();
}

const lars::Any & Extension::Member::asAny() const {
  if (auto res = std::get_if<lars::Any>(&data)) {
    return *res;
  }
  throw Extension::Member::InvalidCastException();
}

lars::Any & Extension::Member::asAny() {
  if (auto res = std::get_if<lars::Any>(&data)) {
    return *res;
  }
  throw Extension::Member::InvalidCastException();
}

const lars::AnyFunction & Extension::Member::asFunction() const {
  if (auto res = std::get_if<lars::AnyFunction>(&data)) {
    return *res;
  } else if(auto res = asAny().tryGet<const lars::AnyFunction>()) {
    return *res;
  }
  throw Extension::Member::InvalidCastException();
}

const Extension & Extension::Member::asExtension() const {
  if (auto res = std::get_if<Extension>(&data)) {
    return *res;
  } else if(auto res = asAny().tryGet<const Extension>()) {
    return *res;
  }
  throw Extension::Member::InvalidCastException();
}

void Extension::Member::setValue(lars::Any &&v){
  data = std::move(v);
}

void Extension::Member::setFunction(const lars::AnyFunction &f){
  data = f;
}

void Extension::Member::setExtension(const Extension &e){
  data = e;
}

Extension::Member &Extension::Member::operator=(const lars::AnyFunction &f){
  setFunction(f);
  return *this;
}

Extension::Member &Extension::Member::operator=(const Extension &e){
  setExtension(e);
  return *this;
}

