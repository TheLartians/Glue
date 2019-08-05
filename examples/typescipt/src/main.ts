
const a = lib.A.create(46);
const b = lib.B.create(-5);
console.assert(a.add(b).next().data() == 42);
lib.log("Hello from TypeScript!");
