import subprocess
import numpy as np
import onnxruntime as ort
import os

def compare_with_stm32(onnx_path, input_data):
    # 1. Рахуємо через стандартний ONNX Runtime
    sess = ort.InferenceSession(onnx_path)
    input_name = sess.get_inputs()[0].name
    
    onnx_out = sess.run(None, {input_name: input_data.astype(np.int32)})[0]

    # 2. Викликаємо консольну утиліту ST
    st_cli = "/Users/tempdelta/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-AI/10.2.0/Utilities/mac/stedgeai"
    
    output_dir = "./st_debug_out"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Зберігаємо вхідні дані для ST CLI також в int32
    np.save("temp_in.npy", input_data.astype(np.int32))
    
    cmd = [
        st_cli, "validate", 
        "--target", "stm32",
        "-m", onnx_path, 
        "--valinput", "temp_in.npy",
        "--output", output_dir,
        "--verbosity", "1",
        "--full"
    ]
    
    print("--- ЗАПУСК ЕМУЛЯТОРА STM32 ---")
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.returncode != 0:
        print("Помилка ST CLI:")
        print(result.stderr)
        return None, None

    # 3. Завантажуємо результат обчислень "заліза"
    res_file = os.path.join(output_dir, "kobzar_clean_val_io.npz")
    if not os.path.exists(res_file):
        res_file = [f for f in os.listdir(output_dir) if f.endswith('.npz')][0]
        res_file = os.path.join(output_dir, res_file)

    st_results = np.load(res_file)
    keys = list(st_results.keys())
    print(f"Знайдені масиви від STM32: {keys}")
    
    out_key = [k for k in keys if k.startswith('c_outputs')][0]
    stm32_out = st_results[out_key]
    
    # 4. Порівняння
    onnx_out_flat = onnx_out.flatten()
    stm32_out_flat = stm32_out.flatten()
    
    cos_sim = np.dot(onnx_out_flat, stm32_out_flat) / (np.linalg.norm(onnx_out_flat) * np.linalg.norm(stm32_out_flat))
    diff = np.abs(onnx_out_flat - stm32_out_flat)
    
    print("\n=== РЕЗУЛЬТАТ ПОРІВНЯННЯ ===")
    print(f"Cosine Similarity: {cos_sim:.6f} (має бути > 0.99)")
    print(f"Max Difference:    {np.max(diff):.6f}")
    print(f"Mean Difference:   {np.mean(diff):.6f}")
    
    return onnx_out, stm32_out

prompt_indices = [47, 93, 86, 82, 1, 86, 88, 107, 8, 1, 47, 93, 86, 82, 1, 86, 88, 107, 8, 1]

my_context_window = [1] * 64
for i, idx in enumerate(prompt_indices):
    my_context_window[64 - len(prompt_indices) + i] = idx

input_32 = np.array([my_context_window], dtype=np.int32)

# ЗАПУСК
py_logits, st_logits = compare_with_stm32("kobzar_clean.onnx", input_32)