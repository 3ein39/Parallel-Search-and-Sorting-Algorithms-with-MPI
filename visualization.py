#!/usr/bin/env python3
"""
Visualization Script for Parallel Algorithm Performance Analysis

This script generates visualizations for:
1. Execution time vs number of cores (scaling analysis)
2. Execution time vs input size (performance analysis)

Dependencies:
- matplotlib
- pandas

To install dependencies:
python3 -m pip install matplotlib pandas

Or if you have pip directly available:
pip install matplotlib pandas
"""

import os
import re
import matplotlib.pyplot as plt
import pandas as pd
import glob

# Set up constants
RESULTS_DIR = "./Performance_Results"
SCALING_DIR = os.path.join(RESULTS_DIR, "core_scaling")
OUTPUT_DIR = os.path.join(RESULTS_DIR, "visualizations")

# Create output directory if it doesn't exist
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Algorithm names mapping
ALGO_NAMES = {
    1: "Quick Search",
    2: "Prime Number Search", 
    3: "Bitonic Sort",
    4: "Radix Sort",
    5: "Sample Sort"
}

# Colors for different algorithms
COLORS = ['b', 'g', 'r', 'c', 'm']

def parse_scaling_file(filepath):
    """Extract core count, user time, and speedup from a scaling analysis markdown file."""
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Extract table data using regex
    pattern = r'\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*([\d\.]+)\s*\|'
    matches = re.findall(pattern, content)
    
    # Convert to DataFrame
    data = []
    for match in matches:
        cores = int(match[0])
        user_time_ms = int(match[1])
        sys_time_ms = int(match[2])
        speedup = float(match[3])
        data.append([cores, user_time_ms, sys_time_ms, speedup])
    
    return pd.DataFrame(data, columns=['cores', 'user_time_ms', 'sys_time_ms', 'speedup'])

def parse_performance_file(filepath):
    """Extract input size and execution time from a performance analysis markdown file."""
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Extract table data using regex
    pattern = r'\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|'
    matches = re.findall(pattern, content)
    
    # Convert to DataFrame
    data = []
    for match in matches:
        input_size = int(match[0])
        user_time_ms = int(match[1])
        sys_time_ms = int(match[2])
        data.append([input_size, user_time_ms, sys_time_ms])
    
    return pd.DataFrame(data, columns=['input_size', 'user_time_ms', 'sys_time_ms'])

def plot_scaling_analysis():
    """Create plots showing execution time vs. number of cores for each algorithm."""
    plt.figure(figsize=(12, 8))
    
    # Process each algorithm
    for algo_num, algo_name in ALGO_NAMES.items():
        scaling_file = os.path.join(SCALING_DIR, f"{algo_name.lower().replace(' ', '_')}_scaling_analysis.md")
        scaling_file = scaling_file.replace("'", "")
        
        if not os.path.exists(scaling_file):
            print(f"Warning: Scaling file not found for {algo_name}")
            continue
        
        # Parse data
        try:
            df = parse_scaling_file(scaling_file)
            if df.empty:
                print(f"Warning: No data found in {scaling_file}")
                continue
                
            plt.subplot(2, 1, 1)
            plt.plot(df['cores'], df['user_time_ms'], marker='o', label=algo_name, color=COLORS[algo_num-1])
            
            plt.subplot(2, 1, 2)
            plt.plot(df['cores'], df['speedup'], marker='o', label=algo_name, color=COLORS[algo_num-1])
        except Exception as e:
            print(f"Error processing {scaling_file}: {e}")
    
    # Format first subplot - Execution Time vs Cores
    plt.subplot(2, 1, 1)
    plt.title('Execution Time vs. Number of Cores')
    plt.xlabel('Number of Cores')
    plt.ylabel('Execution Time (ms)')
    plt.grid(True)
    plt.legend()
    plt.xticks([1, 2, 4, 8])
    
    # Format second subplot - Speedup vs Cores
    plt.subplot(2, 1, 2)
    plt.title('Speedup vs. Number of Cores')
    plt.xlabel('Number of Cores')
    plt.ylabel('Speedup')
    plt.grid(True)
    plt.legend()
    plt.xticks([1, 2, 4, 8])
    plt.axline([1, 1], [8, 8], color='k', linestyle='--', label='Perfect Scaling')
    
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, 'scaling_analysis.png'), dpi=300)
    plt.close()
    
    print(f"Scaling analysis plot saved to {os.path.join(OUTPUT_DIR, 'scaling_analysis.png')}")

def plot_performance_analysis():
    """Create plots showing execution time vs. input size for each algorithm."""
    plt.figure(figsize=(12, 6))
    
    # Process each algorithm
    for algo_num, algo_name in ALGO_NAMES.items():
        perf_file = os.path.join(RESULTS_DIR, f"{algo_name.lower().replace(' ', '_')}_analysis.md")
        perf_file = perf_file.replace("'", "")
        
        if not os.path.exists(perf_file):
            print(f"Warning: Performance file not found for {algo_name}")
            continue
        
        # Parse data
        try:
            df = parse_performance_file(perf_file)
            if df.empty:
                print(f"Warning: No data found in {perf_file}")
                continue
                
            plt.plot(df['input_size'], df['user_time_ms'], marker='o', label=algo_name, color=COLORS[algo_num-1])
        except Exception as e:
            print(f"Error processing {perf_file}: {e}")
    
    # Format plot
    plt.title('Execution Time vs. Input Size')
    plt.xlabel('Input Size')
    plt.ylabel('Execution Time (ms)')
    plt.grid(True)
    plt.legend()
    plt.xscale('log')  # Use log scale for input size
    plt.yscale('log')  # Use log scale for execution time
    
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, 'performance_analysis.png'), dpi=300)
    plt.close()
    
    print(f"Performance analysis plot saved to {os.path.join(OUTPUT_DIR, 'performance_analysis.png')}")

def create_individual_algorithm_plots():
    """Create individual plots for each algorithm showing scaling and performance."""
    for algo_num, algo_name in ALGO_NAMES.items():
        plt.figure(figsize=(12, 10))
        
        # Get file paths
        scaling_file = os.path.join(SCALING_DIR, f"{algo_name.lower().replace(' ', '_')}_scaling_analysis.md")
        scaling_file = scaling_file.replace("'", "")
        perf_file = os.path.join(RESULTS_DIR, f"{algo_name.lower().replace(' ', '_')}_analysis.md")
        perf_file = perf_file.replace("'", "")
        
        # Plot scaling data
        if os.path.exists(scaling_file):
            try:
                df = parse_scaling_file(scaling_file)
                if not df.empty:
                    plt.subplot(3, 1, 1)
                    plt.title(f'{algo_name} - Execution Time vs. Number of Cores')
                    plt.plot(df['cores'], df['user_time_ms'], marker='o', color=COLORS[algo_num-1])
                    plt.xlabel('Number of Cores')
                    plt.ylabel('Execution Time (ms)')
                    plt.grid(True)
                    plt.xticks([1, 2, 4, 8])
                    
                    plt.subplot(3, 1, 2)
                    plt.title(f'{algo_name} - Speedup vs. Number of Cores')
                    plt.plot(df['cores'], df['speedup'], marker='o', color=COLORS[algo_num-1])
                    plt.axline([1, 1], [8, 8], color='k', linestyle='--', label='Perfect Scaling')
                    plt.xlabel('Number of Cores')
                    plt.ylabel('Speedup')
                    plt.grid(True)
                    plt.legend()
                    plt.xticks([1, 2, 4, 8])
            except Exception as e:
                print(f"Error processing scaling data for {algo_name}: {e}")
        
        # Plot performance data
        if os.path.exists(perf_file):
            try:
                df = parse_performance_file(perf_file)
                if not df.empty:
                    plt.subplot(3, 1, 3)
                    plt.title(f'{algo_name} - Execution Time vs. Input Size')
                    plt.plot(df['input_size'], df['user_time_ms'], marker='o', color=COLORS[algo_num-1])
                    plt.xlabel('Input Size')
                    plt.ylabel('Execution Time (ms)')
                    plt.grid(True)
                    plt.xscale('log')  # Use log scale for input size
            except Exception as e:
                print(f"Error processing performance data for {algo_name}: {e}")
        
        plt.tight_layout()
        plt.savefig(os.path.join(OUTPUT_DIR, f'{algo_name.lower().replace(" ", "_")}_analysis.png'), dpi=300)
        plt.close()
        
        print(f"Analysis plots for {algo_name} saved to {os.path.join(OUTPUT_DIR, algo_name.lower().replace(' ', '_') + '_analysis.png')}")

if __name__ == "__main__":
    print("Generating performance visualization graphs...")
    
    try:
        # Create combined plots
        plot_scaling_analysis()
        plot_performance_analysis()
        
        # Create individual algorithm plots
        create_individual_algorithm_plots()
        
        print("Visualization generation complete!")
        print(f"All visualizations saved to {OUTPUT_DIR}")
        
    except ImportError:
        print("Error: Required libraries not found.")
        print("Please install the required libraries using:")
        print("python3 -m pip install matplotlib pandas")
    except Exception as e:
        print(f"Error generating visualizations: {e}")