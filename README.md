# LLVM profiling tool

This tool performs 3 things:
1. Basic execution information including the number of LLVM instructions, basic blocks etc.
2. Expensive instruction information like the number of memory operations, multiplications and branches since they are most likely to affect execution time.
3. Function call information: the number of times each function is called.

Build:

    $ cd llvm_profiling
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

Run:

    $ clang -S -emit-llvm -Xclang -disable-O0-optnone foo.c
    $ opt -load build/skeleton/libSkeletonPass.* -skeleton -S foo.ll
    
The `-Xclang -disable-O0-optnone` flag ensures that Clang will allow later optimizations even when initially compiling without any. 
