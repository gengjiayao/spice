import onnx
import torch
import torch.nn as nn
import torch.nn.functional as F
from onnx import shape_inference

torch.set_default_dtype(torch.float16)

padding_size = 5

tensors = [
    # CSQ0
    torch.tensor([
        [0.409216, 0.825835],
        [0.600770, 0.000000]
    ]),
    # CSQ1
    torch.tensor([
        [-1.014830, 0.770670],
        [-1.618030, 0.000000]
    ]),
    # CSQ2
    torch.tensor([
        [-0.143240, 0.549566, 0.000000, 0.000000],
        [0.539863, -0.925955, 0.937918, 0.245517],
        [0.000000, 0.755415, 0.000000, 0.000000],
        [0.000000, -0.261248, 0.000000, 0.000000]
    ]),
    # CSQ3
    torch.tensor([
        [1.966520, 0.240542],
        [0.823878, 0.000000]
    ]),
    # CSQ4
    torch.tensor([]),
    # CSQ5
    torch.tensor([
        [-0.850102, 1.020800],
        [-1.304080, 0.000000]
    ]),
    # CSQ6
    torch.tensor([
        [-0.196284, -0.282654, 0.100373, 0.830403, -0.218874],
        [0.370436, -0.312433, -0.325333, -0.637391, 0.000000],
        [-0.614328, 0.440441, 1.557710, -0.115204, 0.000000],
        [-0.026423, 0.666769, -1.013040, 0.000000, 0.000000],
        [0.909776, 0.000000, 0.000000, 0.000000, 0.000000]
    ]),
    # CSQ7
    torch.tensor([
        [-0.985910, 0.931419],
        [0.130312, 0.000000]
    ]),
    # CSQ8
    torch.tensor([
        [-0.643998]
    ])
]

def cal_idx_layer_to_1(n: int):
    src_b = torch.tensor([1, 2, 2, 2, 2, 5, 6, 6, 6, 6])
    src_r = torch.tensor([1, 2, 2, 3, 3, 1, 3, 3, 4, 4])
    src_c = torch.tensor([1, 2, 3, 2, 3, 1, 3, 4, 3, 4])

    dst_b = torch.tensor([3, 3, 3, 3, 3, 7, 7, 7, 7, 7])
    dst_r = torch.tensor([0, 0, 0, 1, 1, 0, 0, 0, 1, 1])
    dst_c = torch.tensor([0, 0, 1, 0, 1, 0, 0, 1, 0, 1])

    src_idx = src_b * n * n + src_r * n + src_c
    dst_idx = dst_b * n * n + dst_r * n + dst_c

    return src_idx, dst_idx

def cal_idx_layer_to_2(n :int):
    src_b = torch.tensor([0, 3, 7])
    src_r = torch.tensor([1, 1, 1])
    src_c = torch.tensor([1, 1, 1])

    dst_b = torch.tensor([8, 8, 8])
    dst_r = torch.tensor([0, 0, 0])
    dst_c = torch.tensor([0, 0, 0])

    src_idx = src_b * n * n + src_r * n + src_c
    dst_idx = dst_b * n * n + dst_r * n + dst_c

    return src_idx, dst_idx

'''
    padding矩阵操作 
'''
padded = [] # 暂时存放padding后的tensor
for t in tensors:
    if t.ndim != 2:
        t = t.new_zeros((0, 0))
    r, c = t.shape

    # pad 接受的是 (left, right, top, bottom)
    pad = (0, padding_size - c, 0, padding_size - r)
    padded_t = F.pad(t, pad, mode='constant', value=0.0)
    padded.append(padded_t)

batched = torch.stack(padded, dim=0)
print(batched.shape)

'''
class LU: tensor整体的LU运算
'''
class LU(nn.Module):
    def __init__(self, padding_size):
        super().__init__()
        self.n = padding_size

        # layer0 需要 LU 的 batch idx
        idx0 = torch.tensor([0,1,2,5,6])
        self.register_buffer('idx0', idx0)

        # layer1 需要 LU 的 batch idx
        idx1 = torch.tensor([3,7])
        self.register_buffer('idx1', idx1)

        # layer2 需要 LU 的 batch idx
        idx2 = torch.tensor([8])
        self.register_buffer('idx2', idx2)

        # layer0 -> layer1 的 src_idx / dst_idx
        s0, d0 = cal_idx_layer_to_1(self.n)
        self.register_buffer('src01', s0)
        self.register_buffer('dst01', d0)

        # layer1 -> layer2 的 src_idx / dst_idx
        s1, d1 = cal_idx_layer_to_2(self.n)
        self.register_buffer('src12', s1)
        self.register_buffer('dst12', d1)

    # 为了适应onnx
    @staticmethod
    def _scatter_add_via_loop(flat, src_idx, dst_idx):
        for i in range(src_idx.shape[0]):
            s = src_idx[i]           # 单个标量
            d = dst_idx[i]
            v = flat[s]              # 被减的值
            new_d = flat[d] - v      # 相当于 -= v
            flat = flat.index_copy(0, d.unsqueeze(0), new_d.unsqueeze(0))

        return flat

    def lu_inplace_batch(self, A):
        for k in range(self.n):
            A[:, k+1:, k] /= A[:, k, k].unsqueeze(-1)  # (B,) -> (B, 1)
            V = A[:, k+1:, k].unsqueeze(-1)            # (B, n-k-1) -> (B, n-k-1, 1)
            U = A[:, k, k+1:].unsqueeze(1)             # (B, n-k-1) -> (B, 1, n-k-1)
            A[:, k+1:, k+1:] -= V * U                  # 广播乘法
    
    def forward(self, input):
        A = input.clone()

        # layer0 LU
        sel0 = A.index_select(0, self.idx0)
        self.lu_inplace_batch(sel0)
        A = A.index_copy(0, self.idx0, sel0)

        # layer0 -> layer1 update
        flat = A.view(-1)
        # flat = flat.scatter_add(0, self.dst01, -flat.index_select(0, self.src01))
        flat = self._scatter_add_via_loop(flat, self.src01, self.dst01)
        # flat = self._scatter_add(flat, self.src01, self.dst01)
        # flat = flat.scatter_reduce(0, self.dst01, -flat.index_select(0, self.src01), reduce="sum")
        A = flat.view_as(A)

        # layer1 LU
        sel1 = A.index_select(0, self.idx1)
        self.lu_inplace_batch(sel1)
        A = A.index_copy(0, self.idx1, sel1)

        # layer1 -> layer2 update
        flat = A.view(-1)
        # flat = flat.scatter_add(0, self.dst12, -flat.index_select(0, self.src12))
        flat = self._scatter_add_via_loop(flat, self.src12, self.dst12)
        # flat = self._scatter_add(flat, self.src12, self.dst12)
        # flat = flat.scatter_reduce(0, self.dst12, -flat.index_select(0, self.src12), reduce="sum")
        A = flat.view_as(A)

        # layer2 LU
        sel2 = A.index_select(0, self.idx2)
        self.lu_inplace_batch(sel2)
        A = A.index_copy(0, self.idx2, sel2)
        return A

model = LU(padding_size)
A = model(batched)
print(A)

dummy = batched

# 导出onnx
torch.onnx.export(
    model,
    (dummy,),
    "lu_demo.onnx",
    input_names=["tensor"],
    output_names=["lu"],
    opset_version=11,
    do_constant_folding=False,
    dynamic_axes={
        "tensor": {0: "B", 1: "M", 2: "N"},
        "lu":     {0: "B", 1: "M", 2: "N"},
    },
)