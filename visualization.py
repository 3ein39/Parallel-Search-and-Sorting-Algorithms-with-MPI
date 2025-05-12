import matplotlib.pyplot as plt
import numpy as np
import os

# Create directory for charts if it doesn't exist
os.makedirs("imgs", exist_ok=True)

# Data from the performance summary
algorithms = ["Quick Search", "Prime Number Search", "Bitonic Sort", "Radix Sort", "Sample Sort"]

# Input sizes for each algorithm
input_sizes = {
    "Quick Search": ["1,000", "10,000", "100,000", "1,000,000"],
    "Prime Number Search": ["10,000", "100,000", "1,000,000", "10,000,000"],
    "Bitonic Sort": ["1,024", "4,096", "16,384", "65,536"],
    "Radix Sort": ["1,024", "4,096", "16,384", "65,536"],
    "Sample Sort": ["1,024", "4,096", "16,384", "65,536"]
}

# Times for each algorithm with 4 cores across different input sizes
times_4_cores = {
    "Quick Search": [0.19, 1.12, 9.12, 121.93],
    "Prime Number Search": [42.75, 158.34, 892.56, 1532.87],
    "Bitonic Sort": [12.83, 32.47, 84.92, 183.65],
    "Radix Sort": [8.76, 19.45, 56.23, 127.84],
    "Sample Sort": [10.42, 24.36, 68.75, 152.38]
}

# Core scaling data with very large inputs
cores = [1, 2, 4, 8]
core_scaling = {
    "Quick Search": [503.63, 220.63, 121.93, 78.36],
    "Prime Number Search": [6738.00, 3125.42, 1532.87, 865.23],
    "Bitonic Sort": [642.37, 325.84, 183.65, 98.42],
    "Radix Sort": [482.65, 236.48, 127.84, 72.56],
    "Sample Sort": [563.24, 287.93, 152.38, 83.17]
}

# Chart 1: Algorithm Performance by Input Size (4 Cores)
plt.figure(figsize=(14, 8))

for i, algo in enumerate(algorithms):
    if algo != "Prime Number Search":  # Exclude Prime Number Search due to scale differences
        plt.plot(range(len(input_sizes[algo])), times_4_cores[algo], marker='o', linewidth=2, label=algo)

plt.title('Algorithm Performance by Input Size (4 Cores)', fontsize=16)
plt.xlabel('Input Size', fontsize=14)
plt.ylabel('Execution Time (ms)', fontsize=14)
plt.xticks(range(len(input_sizes["Quick Search"])), ['Small', 'Medium', 'Large', 'Very Large'])
plt.legend()
plt.grid(True, linestyle='--', alpha=0.7)
plt.tight_layout()
plt.savefig("imgs/algorithm_performance_by_size.png", dpi=300)

# Chart 2: Prime Number Search Performance (separate due to scale)
plt.figure(figsize=(10, 6))
plt.plot(range(len(input_sizes["Prime Number Search"])), times_4_cores["Prime Number Search"], 
         marker='o', linewidth=2, color='crimson')
plt.title('Prime Number Search Performance by Input Size (4 Cores)', fontsize=16)
plt.xlabel('Input Size', fontsize=14)
plt.ylabel('Execution Time (ms)', fontsize=14)
plt.xticks(range(len(input_sizes["Prime Number Search"])), ['Small', 'Medium', 'Large', 'Very Large'])
plt.grid(True, linestyle='--', alpha=0.7)
plt.tight_layout()
plt.savefig("imgs/prime_search_performance.png", dpi=300)

# Chart 3: Scaling Efficiency with Cores for All Algorithms
plt.figure(figsize=(14, 8))

for algo in algorithms:
    # Calculate speedup relative to 1 core
    speedups = [core_scaling[algo][0] / time for time in core_scaling[algo]]
    plt.plot(cores, speedups, marker='o', linewidth=2, label=algo)

plt.title('Scaling Efficiency (Speedup) vs. Number of Cores', fontsize=16)
plt.xlabel('Number of Cores', fontsize=14)
plt.ylabel('Speedup (relative to 1 core)', fontsize=14)
plt.xticks(cores)
plt.plot(cores, cores, 'k--', alpha=0.5, label='Linear Speedup (Ideal)')  # Ideal scaling line
plt.legend()
plt.grid(True, linestyle='--', alpha=0.7)
plt.tight_layout()
plt.savefig("imgs/scaling_efficiency.png", dpi=300)

# Chart 4: Execution Time vs Core Count (log scale)
plt.figure(figsize=(14, 8))

for algo in algorithms:
    plt.plot(cores, core_scaling[algo], marker='o', linewidth=2, label=algo)

plt.title('Execution Time vs. Number of Cores (Very Large Input)', fontsize=16)
plt.xlabel('Number of Cores', fontsize=14)
plt.ylabel('Execution Time (ms) - Log Scale', fontsize=14)
plt.yscale('log')
plt.xticks(cores)
plt.legend()
plt.grid(True, linestyle='--', alpha=0.7)
plt.tight_layout()
plt.savefig("imgs/execution_time_vs_cores_log.png", dpi=300)

# Chart 5: Comparing Sorting Algorithms
plt.figure(figsize=(14, 8))

sorting_algos = ["Bitonic Sort", "Radix Sort", "Sample Sort"]
x = np.arange(len(input_sizes["Bitonic Sort"]))
width = 0.25

for i, algo in enumerate(sorting_algos):
    plt.bar(x + i*width, times_4_cores[algo], width, label=algo)

plt.title('Comparison of Sorting Algorithms (4 Cores)', fontsize=16)
plt.xlabel('Input Size', fontsize=14)
plt.ylabel('Execution Time (ms)', fontsize=14)
plt.xticks(x + width, ['Small', 'Medium', 'Large', 'Very Large'])
plt.legend()
plt.grid(True, linestyle='--', alpha=0.7, axis='y')
plt.tight_layout()
plt.savefig("imgs/sorting_algorithms_comparison.png", dpi=300)

# Chart 6: Super-linear Speedup Focus for Sample Sort
plt.figure(figsize=(10, 6))

sample_sort_speedups = [core_scaling["Sample Sort"][0] / time for time in core_scaling["Sample Sort"]]
ideal_speedups = [c for c in cores]  # Ideal linear speedup

plt.plot(cores, sample_sort_speedups, marker='o', linewidth=2, color='purple', label='Sample Sort')
plt.plot(cores, ideal_speedups, 'k--', alpha=0.7, label='Ideal Linear Speedup')

plt.title('Sample Sort: Super-linear Speedup Analysis', fontsize=16)
plt.xlabel('Number of Cores', fontsize=14)
plt.ylabel('Speedup Factor', fontsize=14)
plt.xticks(cores)
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()
plt.tight_layout()
plt.savefig("imgs/sample_sort_superlinear_speedup.png", dpi=300)

# NEW ADDITIONS: Individual algorithm charts

# Chart 7: Individual Algorithm Input Size Performance with 4 cores
for algo in algorithms:
    plt.figure(figsize=(10, 6))
    plt.plot(range(len(input_sizes[algo])), times_4_cores[algo], marker='o', linewidth=2, color='blue')
    
    # Add a fitted polynomial trend line
    x = np.array(range(len(input_sizes[algo])))
    y = np.array(times_4_cores[algo])
    z = np.polyfit(x, y, 2)  # 2nd degree polynomial 
    p = np.poly1d(z)
    plt.plot(x, p(x), 'r--', linewidth=1)
    
    plt.title(f'{algo}: Performance vs Input Size (4 Cores)', fontsize=16)
    plt.xlabel('Input Size', fontsize=14)
    plt.ylabel('Execution Time (ms)', fontsize=14)
    plt.xticks(range(len(input_sizes[algo])), ['Small', 'Medium', 'Large', 'Very Large'])
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(f"imgs/{algo.replace(' ', '_').lower()}_input_scaling.png", dpi=300)

# Chart 8: Individual Algorithm Core Scaling Performance with Very Large Input
for algo in algorithms:
    plt.figure(figsize=(10, 6))
    
    # Plot execution time
    plt.plot(cores, core_scaling[algo], marker='o', linewidth=2, color='green')
    
    plt.title(f'{algo}: Performance vs Number of Cores (Very Large Input)', fontsize=16)
    plt.xlabel('Number of Cores', fontsize=14)
    plt.ylabel('Execution Time (ms)', fontsize=14)
    plt.xticks(cores)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(f"imgs/{algo.replace(' ', '_').lower()}_core_scaling.png", dpi=300)

    # Also create a speedup version
    plt.figure(figsize=(10, 6))
    
    # Calculate speedup
    speedups = [core_scaling[algo][0] / time for time in core_scaling[algo]]
    
    plt.plot(cores, speedups, marker='o', linewidth=2, color='orange')
    plt.plot(cores, cores, 'k--', alpha=0.5, label='Linear Speedup (Ideal)')
    
    plt.title(f'{algo}: Speedup vs Number of Cores (Very Large Input)', fontsize=16)
    plt.xlabel('Number of Cores', fontsize=14)
    plt.ylabel('Speedup (relative to 1 core)', fontsize=14)
    plt.xticks(cores)
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(f"imgs/{algo.replace(' ', '_').lower()}_speedup.png", dpi=300)

print("Charts have been generated and saved to the 'imgs' directory.")