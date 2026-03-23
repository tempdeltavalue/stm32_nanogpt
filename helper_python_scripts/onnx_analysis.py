import onnxruntime as ort
import onnx
import numpy as np

# 1. Завантажуємо твою модель
model = onnx.load("kobzar_clean.onnx")

# 2. Фокус: додаємо ВСІ проміжні тензори до списку виходів
for vi in model.graph.value_info:
    model.graph.output.append(vi)

# Зберігаємо тимчасову модель
onnx.save(model, "kobzar_layer_debug.onnx")

# 3. Запускаємо ONNX Runtime
sess = ort.InferenceSession("kobzar_layer_debug.onnx")

# Ті самі вхідні дані, що й для STM32
prompt_indices = [47, 93, 86, 82, 1, 86, 88, 107, 8, 1, 47, 93, 86, 82, 1, 86, 88, 107, 8, 1]
my_context_window = [1] * 32
for i, idx in enumerate(prompt_indices):
    my_context_window[32 - len(prompt_indices) + i] = idx

input_32 = np.array([my_context_window], dtype=np.int32)
input_name = sess.get_inputs()[0].name

# Витягуємо результати всіх шарів
outputs = sess.run(None, {input_name: input_32})
output_names = [x.name for x in sess.get_outputs()]

print("\n=== ПОШАРОВИЙ АНАЛІЗ ONNX (PYTHON) ===")
print(f"{'Назва тензора (в ONNX)':<30} | {'Мін':<10} | {'Макс':<10} | {'Середнє':<10}")
print("-" * 70)
for name, out in zip(output_names, outputs):
    if out is not None and isinstance(out, np.ndarray):
        print(f"{name[:30]:<30} | {np.min(out):<10.3f} | {np.max(out):<10.3f} | {np.mean(out):<10.3f}")