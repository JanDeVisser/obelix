# What

``obelix`` is a system programming language. It's firmly in the ``C`` family, and I'd like to keep it 
there, while addressing some bug bears I have with ``C``. After some experimentation with a backend
generating native `ARM64` code we concluded that in order to accelerate forward progress transpiling to
C code is better; the `ARM64` backend, and potentially `ARM32` and `x86_64` backend, could be added later, 
or maybe even implemented using an `LLVM` backend.

# How

## Prerequisites

``cmake``, the XCode command line tools (``gcc``, ``as``, ``ld`` etc).

## Compile

```console
$ git clone git@github.com:JanDeVisser/obelix.git
$ cd obelix
$ mkdir build
$ cd build
$ cmake ..
$ cd ..
$ cmake --build build --target install
```

Works best if you put ``obelix/build/bin`` in your path but you do you.

## Testing/Playing around

There are test files in the ``test`` subdirectory. Obelix source files have the ``.obl`` extension. Feel free to have a 
look around and a play.

## Todo

- [ ] Floats
- [ ] Introduce 'method-like' fuction calls like for example
```c
    const s = "Hello There"
    putln(s.length());
```
- [ ] Improve compiler errors and warnings
- [ ] Error handling. Syntax proposal:
```c
    var fh: int/int = open("foo.bar", O_RDONLY)
    if (error(fh)) { /* or !ok(fh) */
        puts("An error occurred: ")
        putln(fh) /* Auto unwrap */
        return
    }
    read(fh, 256) /* Auto unwrap */
```
- [ ] Develop object life cycle mechanism. Investigate and compare Go `defer`, Python
`context` and C++ destructors.
- [ ] Expose `format()` to Obelix. Will probably involve rewriting into C.
- [ ] Unify signed and unsigned integers. Or at least allow some sort of coercion.
- [ ] Improve explicit cast and implicit coercions.
- [ ] Keep `ARM64` backend up-to-date.
- [ ] Investigate `ARM32`, `x86_64`, and/or `LLVM` backends.
