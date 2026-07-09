import subprocess
import sys

def run_perf(binary_path):
    print(f"--- HACOE Bare-Metal PMU Profiling: {binary_path} ---")
    
    # We track cycles, total instructions, and L2 cache misses
    # Note: specific L2 event names vary by CPU. 'l2_rqsts.miss' is a common x86 counter.
    perf_cmd = [
        "perf", "stat",
        "-e", "cycles,instructions,cache-misses",
        binary_path
    ]
    
    try:
        # Run perf and capture stderr (where perf prints its output)
        subprocess.run(perf_cmd, check=True)
    except subprocess.CalledProcessError:
        print("[!] Perf execution failed. Do you have root/sudo privileges for PMU counters?")
        print("    Try running: sudo sysctl -w kernel.perf_event_paranoid=-1")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python perf_runner.py <path_to_binary>")
    else:
        run_perf(sys.argv[1])
