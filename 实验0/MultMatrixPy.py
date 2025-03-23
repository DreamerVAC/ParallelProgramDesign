# -*- coding: utf-8 -*-
import numpy as np
import time

N = 256
A = np.random.rand(N, N)
B = np.random.rand(N, N)
C = np.zeros((N, N))

start = time.time()
for i in range(N):
    for j in range(N):
        for k in range(N):
            C[i][j] += A[i][k] * B[k][j]
end = time.time()

print("Python Time: ", end - start, "s")
