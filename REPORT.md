# 1.3 Shadow stack
## Why tail calls break the shadow stack
The shadow stack only works if every function pushes one return address
when it starts (GUARD_ENTER) and pops one when it returns (GUARD_RETURN).
The pushes and the pops have to stay balanced.

A tail call breaks that balance. When a function ends by returning the
result of another function, the compiler may reuse the current frame and
jump straight into that other function instead of doing a real call and
return. The first function already pushed its return address, but it
jumps away before its GUARD_RETURN runs, so that push is never popped.

We prevent this by building with -fno-optimize-sibling-calls, so every
call keeps its own frame and its own matching return.