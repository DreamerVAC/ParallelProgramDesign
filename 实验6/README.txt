- 代码结构
包含4个C程序文件、1个C头文件和2个shell文件。

C头文件：
    - parallel_for.h
    用于parallel_for函数声明。

C程序文件：
    - parallel_for.c
    用于parallel_for函数实现
    - matrix_mul_test.c
    用于矩阵乘法验证parallel_for实现。
    - heated_plate_openmp.c
    原始的openmp实现并行计算heated_plate问题。
    - heated_plate_pthreads.c
    使用parallel_for并行计算heated_plate问题。

shell文件：
    - matrix_mul_test.sh
    执行矩阵乘法测试脚本。
    - heated_plate_openmp.sh
    执行并行计算heated_plate问题脚本。

- 运行方式：
矩阵乘法验证parallel_for实现：
    ./matrix_mul_test.sh
    
使用parallel_for并行计算heated_plate问题：
    ./heated_plate_openmp.sh

生成的所有动态链接库文件与执行文件均会自动生成于项目文件夹下的bin文件夹中。