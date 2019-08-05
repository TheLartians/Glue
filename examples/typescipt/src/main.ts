
class Greeter {
  name: string;

  constructor(name: string){
    this.name = name;
  }

  greet() {
    lib.log(`Hello ${this.name}!`);
  }
};

const greeter = new Greeter("C++");
greeter.greet();


const a = lib.A.create(46);
const b = lib.B.create(-5);
console.assert(a.add(b).next().data() == 42);
