import subprocess
import os

def run_pipeline():
    print("[+] HACOE: Starting Phase 1 Compilation...")
    
    # 1. Compile the target code
    compile_cmd = ["clang", "-O0", "tests/vector_add.c", "-o", "bin/vector_add"]
    subprocess.run(compile_cmd)
    
    # 2. Run the Hardware Profiler
    print("[+] HACOE: Discovering Hardware Bounds...")
    profiler_cmd = ["./bin/profiler"]
    subprocess.run(profiler_cmd)
    
    print("[+] Pipeline complete.")

if __name__ == "__main__":
    if not os.path.exists("bin"):
        os.makedirs("bin")
    run_pipeline()

