
function test() {
  const a = lib.A.create(46);
  const b = lib.B.create(-5);
  console.assert(a.add(b).next().data() == 42);
}

test();

class Greeter {
  private name: string;

  constructor(name: string){
    this.name = name;
  }

  public greet() {
    lib.log(`Hello ${this.name}!`);
  }
}

const greeter = new Greeter("C++");
greeter.greet();
