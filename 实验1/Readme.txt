# 代码描述：
所有源代码均包含在MPIMultMatrix.c中，共2个辅助函数和一个主函数

## 运行代码：终端中调用MPI命令
    编译c代码：
    mpicc MPIMultMatrix.c -o MPIMultMatrix
    运行程序：
    mpirun -np 4 ./MPIMultMatrix m n k
    其中m为矩阵A的行数，n为矩阵A的列数和矩阵B的行数，k为矩阵B的列数

## 运行结果格式：
    Matrix A:
        ...
    Matrix B:
        ...
    Matrix C:
        ...
    矩阵乘法计算时间：xxx 秒