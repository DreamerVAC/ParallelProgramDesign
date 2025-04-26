- 代码结构
    包含1个源代码文件
    - OpenMPMultMatrix.c

- 运行方式
    首先下载omp库并确保库文件夹位于项目头文件目录中

    - 本实验采用macOS进行，编译指令如下：

    ```
    /opt/homebrew/opt/llvm/bin/clang -O3 -Xpreprocessor -fopenmp \
        -I/opt/homebrew/opt/libomp/include \
        OpenMPMultMatrix.c \
        -L/opt/homebrew/opt/libomp/lib -lomp \
        -o OpenMPMultMatrix
    ```

    编译后可得到OpenMPMultMatrix bytecode可执行文件

    - 之后运行

    ```
    ./OpenMPMultMatrix
    ```
    
    即可运行                                            
