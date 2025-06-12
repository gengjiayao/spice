def div_cal(num: int):
    cnt = 0
    for i in range(num - 1, 1, -1):
        cnt += i * i * 2
    cnt += num * (num - 1) >> 1
    return cnt

l1 = [8, 16, 32, 64]
l2 = [2029, 1597, 441, 5]

for i, j in zip(l1, l2):
    print(f"ops={div_cal(i) * j}\t{j}个{i}阶矩阵构成tensor")