import sys

def sort_sparse(input_path:str, output_path:str) -> None:
    # 1. 读入原文件
    with open(input_path, 'r') as fin:
        header = fin.readline().strip()
        if not header:
            raise ValueError("输入文件为空或格式不正确")
        n_rows, n_cols, nnz = map(int, header.split())
        
        entries = []
        for _ in range(nnz):
            line = fin.readline().strip()
            if not line:
                raise ValueError("非零元行数不足 nnz")
            i, j, v = line.split()
            entries.append((int(i) - 1, int(j) - 1, float(v)))
    
    # 2. 排序：先按行，再按列
    entries.sort(key=lambda x: (x[0], x[1]))
    
    # 3. 写入输出文件
    with open(output_path, 'w') as fout:
        fout.write(f"{n_rows} {n_cols} {nnz}\n")
        for i, j, v in entries:
            fout.write(f"{i} {j} {v}\n")

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 sort_sparse.py input.txt output.txt")
        sys.exit(1)
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    sort_sparse(input_path, output_path)
    print(f"已将排序结果写入 {output_path}")

if __name__ == '__main__':
    main()
