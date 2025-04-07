代码描述：
    所有源代码均包含在MPIMultMatrixV2.c中，共2个辅助函数和一个主函数

    - 运行代码：终端中调用MPI命令

        - 编译c代码：
            mpicc MPIMultMatrixV2.c -o MPIMultMatrixV2

        - 运行程序：
            mpirun -np num_process ./MPIMultMatrixV2 m n k method block_size
            其中num_process为进程数，m为矩阵A的行数，n为矩阵A的列数和矩阵B的行数，k为矩阵B的列数，method为所选用的划分方式，block_size为块循环划分中每个块的行数
            划分方式：0为块划分，1为循环划分，2为块循环划分

    - 运行结果示例（以4进程为例）：
        各进程局部计算时间（秒）：
        进程 0: 11.654558
        进程 1: 11.953227
        进程 2: 11.543617
        进程 3: 11.874972
