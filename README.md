# What

``obelix`` is a system programming language. It's firmly in the ``C`` family, and I'd like to keep it there, while addressing some bug bears I have with ``C``. It currently compiles to native ARM64 code which is only tested on MacOS. If you want to help me port it over to Linux, maybe Windows, and the Raspberry Pi: pull requests welcome!

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

There are test files in the ``test`` subdirectory. Obelix source files have the ``.obl`` extension. Feel free to have a look around and a play.

