local a = lib.A.create(46)
local b = lib.B.create(-5)
assert(a:add(b):next():data() == 42)
lib.log("Hello from TypeScript!")
