declare module lib {
  class A {
    __tostring(): string;
    add(arg1: lib.A): lib.A;
    static create(this: void, arg0: number): lib.A;
    data(): number;
    next(): lib.A;
    setData(arg1: number): void;
  }
  class B extends lib.A {
    static create(this: void, arg0: number): lib.B;
    name(): string;
    setName(arg1: string): void;
  }
  function log(this: void, arg0: string): void;
}