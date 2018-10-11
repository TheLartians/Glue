#include <lars/log.h>
#include <lars/glue.h>

#include <lars/component_visitor.h>


namespace visitable{
  struct A:lars::Visitable<A>{ int value = 0; };
  struct B:lars::DVisitable<B, A>{ B(){ value = 1; } };
  struct C:lars::DVisitable<C, A>{ C(){ value = 2; } };

  void test(){
    lars::Extension extension;
    extension.add_function("add", [](double x,double y){ return x+y; });
    assert( extension.get_function("add")(2,3).smart_get<int>() == 5 );
    extension.add_function("add_A", [](A & x,A & y){ A a; a.value = x.value+y.value; return a; });
    assert( extension.get_function("add_A")(B(),C()).smart_get<A>().value == 3 );
  }
}

namespace normal{
  struct A{ int value = 0; };
  struct B:A{ B(int v){ value = v; } };
  
  void test(){
    using MB = lars::MVisitable<B, A>;
    MB a(5);
    
    lars::Extension extension;
    extension.add_function("add", [](A & x,A & y){ A a; a.value = x.value+y.value; return a; });
    assert( extension.get_function("add")(MB(2),MB(3)).smart_get<A>().value == 5 );
  }
}

int main(int argc, char *argv[]) {
  visitable::test();
  normal::test();
  return 0;
}
