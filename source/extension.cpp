#include <glue/extension.h>

using namespace glue;

const NewExtension::Member &NewExtension::operator[](const std::string &key)const{
  auto it = data->members.find(key);
  if (it == data->members.end()) {
    throw NewExtension::MemberNotFoundException(key);
  }
  return it->second;
}

const char * NewExtension::MemberNotFoundException::what()const noexcept{
  if (buffer.size() == 0) {
    buffer = "extension contains no function '" + name + "'.";
  }
  return buffer.c_str();
}

const lars::Any & NewExtension::Member::asAny() const {
  if (auto res = std::get_if<lars::Any>(&data)) {
    return *res;
  }
  throw NewExtension::Member::InvalidCastException();
}

lars::Any & NewExtension::Member::asAny() {
  if (auto res = std::get_if<lars::Any>(&data)) {
    return *res;
  }
  throw NewExtension::Member::InvalidCastException();
}

const lars::AnyFunction & NewExtension::Member::asFunction() const {
  if (auto res = std::get_if<lars::AnyFunction>(&data)) {
    return *res;
  } else if(auto res = asAny().tryGet<lars::AnyFunction>()) {
    return *res;
  }
  throw NewExtension::Member::InvalidCastException();
}

const NewExtension & NewExtension::Member::asExtension() const {
  if (auto res = std::get_if<NewExtension>(&data)) {
    return *res;
  }
  throw NewExtension::Member::InvalidCastException();
}

void NewExtension::Member::setValue(lars::Any &&v){
  data = std::move(v);
};

void NewExtension::Member::setFunction(const lars::AnyFunction &f){
  data = f;
}

void NewExtension::Member::setExtension(const NewExtension &e){
  data = e;
}

NewExtension::Member &NewExtension::Member::operator=(const lars::AnyFunction &f){
  setFunction(f);
  return *this;
};

NewExtension::Member &NewExtension::Member::operator=(const NewExtension &e){
  setExtension(e);
  return *this;
};
