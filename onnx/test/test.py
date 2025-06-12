import onnxruntime as ort
import numpy as np

sess = ort.InferenceSession("add.onnx")
a_np = np.random.randn(10, 5).astype(np.float64)
b_np = np.random.randn(10, 5).astype(np.float64)

out = sess.run(
    ['c' if 'c' in [o.name for o in sess.get_outputs()] else 'sum'],
    {'x': a_np, 'y': b_np}
)
print("onnxruntime:\n", out)