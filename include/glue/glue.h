#pragma once

#include <glue/extension.h>

namespace glue{
  
  class Glue{
  private:
    std::vector<lars::Observer> observers;

  public:

    virtual ~Glue(){}
  };
  
}

