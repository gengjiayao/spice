import numpy
import torch
import torch.nn.functional as F
import onnxruntime as ort

# --------------------------------------------------------------------------
# 1. 重建输入 batch，完全与导出脚本一致
# --------------------------------------------------------------------------
torch.set_default_dtype(torch.float16)
padding_size = 5

# 原始 tensors 列表
tensors = [
    # CSQ0
    torch.tensor([[0.409216, 0.825835], [0.600770, 0.000000]]),
    # CSQ1
    torch.tensor([[-1.014830, 0.770670], [-1.618030, 0.000000]]),
    # CSQ2
    torch.tensor([
        [-0.143240, 0.549566, 0.000000, 0.000000],
        [ 0.539863, -0.925955, 0.937918, 0.245517],
        [ 0.000000,  0.755415, 0.000000, 0.000000],
        [ 0.000000, -0.261248, 0.000000, 0.000000],
    ]),
    # CSQ3
    torch.tensor([[1.966520, 0.240542], [0.823878, 0.000000]]),
    # CSQ4
    torch.tensor([]),
    # CSQ5
    torch.tensor([[-0.850102, 1.020800], [-1.304080, 0.000000]]),
    # CSQ6
    torch.tensor([
        [-0.196284, -0.282654, 0.100373, 0.830403, -0.218874],
        [ 0.370436, -0.312433, -0.325333, -0.637391,  0.000000],
        [-0.614328,  0.440441, 1.557710, -0.115204,  0.000000],
        [-0.026423,  0.666769, -1.013040, 0.000000,  0.000000],
        [ 0.909776,  0.000000, 0.000000,  0.000000,  0.000000],
    ]),
    # CSQ7
    torch.tensor([[-0.985910, 0.931419], [0.130312, 0.000000]]),
    # CSQ8
    torch.tensor([[-0.643998]]),
]

# pad each tensor to (padding_size, padding_size)
padded = []
for t in tensors:
    if t.ndim != 2:
        # 保证空 tensor 也成为 (0,0)
        t = t.new_zeros((0, 0))
    r, c = t.shape
    pad = (0, padding_size - c, 0, padding_size - r)  # (left, right, top, bottom)
    padded.append(F.pad(t, pad, mode='constant', value=0.0))

# stack 成 batch: (B, M, N)
batched = torch.stack(padded, dim=0)
print("batched.shape =", batched.shape, " dtype=", batched.dtype)

# --------------------------------------------------------------------------
# 2. （可选）用原 PyTorch 模型做一次前向以获得参考输出
#    如果你希望对比 ONNX 推理结果
# --------------------------------------------------------------------------
class LU(torch.nn.Module):
    def __init__(self, padding_size):
        super().__init__()
        self.n = padding_size
        idx0 = torch.tensor([0,1,2,5,6])
        idx1 = torch.tensor([3,7])
        idx2 = torch.tensor([8])
        self.register_buffer('idx0', idx0)
        self.register_buffer('idx1', idx1)
        self.register_buffer('idx2', idx2)

        def cal_idx_layer_to_1(n):
            src_b = torch.tensor([1,2,2,2,2,5,6,6,6,6])
            src_r = torch.tensor([1,2,2,3,3,1,3,3,4,4])
            src_c = torch.tensor([1,2,3,2,3,1,3,4,3,4])
            dst_b = torch.tensor([3,3,3,3,3,7,7,7,7,7])
            dst_r = torch.tensor([0,0,0,1,1,0,0,0,1,1])
            dst_c = torch.tensor([0,0,1,0,1,0,0,1,0,1])
            src_idx = src_b * n * n + src_r * n + src_c
            dst_idx = dst_b * n * n + dst_r * n + dst_c
            return src_idx, dst_idx

        def cal_idx_layer_to_2(n):
            src_b = torch.tensor([0,3,7])
            src_r = torch.tensor([1,1,1])
            src_c = torch.tensor([1,1,1])
            dst_b = torch.tensor([8,8,8])
            dst_r = torch.tensor([0,0,0])
            dst_c = torch.tensor([0,0,0])
            src_idx = src_b * n * n + src_r * n + src_c
            dst_idx = dst_b * n * n + dst_r * n + dst_c
            return src_idx, dst_idx

        s0, d0 = cal_idx_layer_to_1(self.n)
        s1, d1 = cal_idx_layer_to_2(self.n)
        self.register_buffer('src01', s0)
        self.register_buffer('dst01', d0)
        self.register_buffer('src12', s1)
        self.register_buffer('dst12', d1)

    def lu_inplace_batch(self, A):
        for k in range(self.n):
            A[:, k+1:, k] /= A[:, k, k].unsqueeze(-1)
            V = A[:, k+1:, k].unsqueeze(-1)
            U = A[:, k, k+1:].unsqueeze(1)
            A[:, k+1:, k+1:] -= V * U

    def forward(self, input):
        A = input.clone()
        # layer0
        sel0 = A.index_select(0, self.idx0);  self.lu_inplace_batch(sel0)
        A = A.index_copy(0, self.idx0, sel0)
        # layer0 -> layer1
        flat = A.view(-1)
        flat = flat.scatter_add(0, self.dst01, -flat.index_select(0, self.src01))
        A = flat.view_as(A)
        # layer1
        sel1 = A.index_select(0, self.idx1);  self.lu_inplace_batch(sel1)
        A = A.index_copy(0, self.idx1, sel1)
        # layer1 -> layer2
        flat = A.view(-1)
        flat = flat.scatter_add(0, self.dst12, -flat.index_select(0, self.src12))
        A = flat.view_as(A)
        # layer2
        sel2 = A.index_select(0, self.idx2);  self.lu_inplace_batch(sel2)
        A = A.index_copy(0, self.idx2, sel2)
        return A

# 计算 PyTorch 参考输出（float16）
model = LU(padding_size).eval()
with torch.no_grad():
    torch_out = model(batched)

# print(torch_out)

# --------------------------------------------------------------------------
# 3. 加载 ONNX 模型并运行推理
# --------------------------------------------------------------------------
onnx_path = "./model/lu_demo.onnx"

# 创建 InferenceSession
sess = ort.InferenceSession(onnx_path, providers=["CPUExecutionProvider"])

# 准备 numpy 输入
input_name = sess.get_inputs()[0].name
np_input = batched.cpu().numpy()

# 推理
outs = sess.run(None, {input_name: np_input})
onnx_out = outs[0]
print(onnx_out)
print("ONNX 推理输出 shape=", onnx_out.shape, " dtype=", onnx_out.dtype)
