import sys
import numpy as np

def sorted_txt_to_csr_txt(input_path: str, output_path: str) -> None:
    # 1. 读取已排序的稀疏矩阵
    with open(input_path, 'r') as f:
        header = f.readline().strip()
        if not header:
            raise ValueError("输入文件为空或格式错误")
        n_rows, n_cols, nnz = map(int, header.split())
        cols = np.empty(nnz, dtype=int)
        vals = np.empty(nnz, dtype=float)
        row_counts = np.zeros(n_rows, dtype=int)
        
        for idx in range(nnz):
            line = f.readline().strip()
            if not line:
                raise ValueError(f"数据行不足，期待 {nnz} 行，读取到第 {idx} 行时结束")
            i, j, v = line.split()
            i, j, v = int(i), int(j), float(v)
            cols[idx] = j
            vals[idx] = v
            row_counts[i] += 1

    # 2. 构造 row_ptr
    row_ptr = np.empty(n_rows + 1, dtype=int)
    row_ptr[0] = 0
    for r in range(n_rows):
        row_ptr[r+1] = row_ptr[r] + row_counts[r]

    # 3. 写入纯文本
    with open(output_path, 'w') as f:
        # header
        f.write(f"{n_rows} {n_cols} {nnz}\n")
        # row_ptr
        f.write(" ".join(map(str, row_ptr.tolist())) + "\n")
        # col_ind
        f.write(" ".join(map(str, cols.tolist())) + "\n")
        # values
        f.write(" ".join(map(str, vals.tolist())) + "\n")

    print(f"已将 CSR 文本格式保存到 {output_path}")
    print(f"  row_ptr(len={len(row_ptr)}): {row_ptr}")
    print(f"  col_ind(len={len(cols)}) 前 10: {cols[:10]}")
    print(f"  values(len={len(vals)})  前 10: {vals[:10]}")

def main():
    if len(sys.argv) != 3:
        print("Usage: python to_csr_txt.py sorted_input.txt output_csr.txt")
        sys.exit(1)
    sorted_txt = sys.argv[1]
    output_txt = sys.argv[2]
    sorted_txt_to_csr_txt(sorted_txt, output_txt)

if __name__ == '__main__':
    main()
