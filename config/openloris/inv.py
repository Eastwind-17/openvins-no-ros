import numpy as np

# 设置输出精度，让结果更易读
np.set_printoptions(precision=8, suppress=True)

# 定义你提供的4x4矩阵
matrix = np.array([    
    [-0.9999783 , -0.00454895,  0.00476415, -0.05337982],
    [ 0.00457962, -0.99996873,  0.00644655, -0.00012782],
    [ 0.00473468,  0.00646823,  0.99996787,  0.00027013],
    [ 0.0, 0.0, 0.0, 1.0]
])

try:
    # 计算逆矩阵
    inv_matrix = np.linalg.inv(matrix)
    
    # 输出结果
    print("=== 原始矩阵 ===")
    print(matrix)
    print("\n=== 逆矩阵 ===")
    print(inv_matrix)
    
    # 验证：原矩阵 × 逆矩阵 = 单位矩阵
    print("\n=== 验证（原矩阵 × 逆矩阵）===")
    verify = np.dot(matrix, inv_matrix)
    print(verify)
    
except np.linalg.LinAlgError:
    print("该矩阵不可逆（行列式为0）")
