# The Ultimate Compiler

Small C-like compiler that emits x86-64 assembly.

## Run

```sh
make
./build/compilador input.c -o output.s
gcc -no-pie output.s -o output
./output
```

If `-o` is omitted, the assembly file is written next to the input using the same name with a `.s` extension.

## Test

```sh
make test
```

To keep generated test artifacts:

```sh
make test-keep
```

