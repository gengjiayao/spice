import torch
import torch.nn as nn

class Add(nn.Module):
    def forward(self, x, y):
        return x + y

model = Add()

x = torch.randn(1, 1, dtype=torch.float64)
y = torch.randn(1, 1, dtype=torch.float64)

torch.onnx.export(
    model,
    (x, y),
    "add.onnx",
    input_names=['x', 'y'],
    output_names=['sum'],
    dynamic_axes={
        'x': {0: 'N', 1: 'M'},
        'y': {0: 'N', 1: 'M'},
        'sum': {0: 'N', 1: 'M'},
    },
    opset_version=13,
)