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
    This will generate `foo.ll` LLVM IR. We'll run our LLVM profiling pass on this IR and generate `foo2.ll` using the line below:
    $ opt -load build/skeleton/libSkeletonPass.* -skeleton -S foo.ll > foo2.ll
    Now that we have the transformed IR we can go ahead and compile and run it using clang:
    $ clang foo2.ll
    $ ./a.out 
    
The `-Xclang -disable-O0-optnone` flag ensures that Clang will allow later optimizations even when initially compiling without any. 
