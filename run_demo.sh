#!/bin/bash

# Parallel Algorithm Demo Script
# This script runs all algorithms with multiple input sizes and logs the results

# Directory for storing results
RESULTS_DIR="./Performance_Results"
mkdir -p $RESULTS_DIR

# Compile the project
echo "Compiling the project..."
mpic++ -o program source.cpp Prime_Number_Search.cpp Bitonic_Sort.cpp Sample_Sort.cpp Quick_Search.cpp Radix_Sort.cpp

# Function to generate sorted array of given size
generate_sorted_array() {
    local size=$1
    local output_file=$2
    echo "Generating sorted array of size $size..."
    for ((i=1; i<=$size; i++)); do
        echo -n "$i " >> $output_file
    done
    echo "" >> $output_file
}

# Function to generate random array of given size
generate_random_array() {
    local size=$1
    local output_file=$2
    echo "Generating random array of size $size..."
    > $output_file  # Clear file
    for ((i=0; i<$size; i++)); do
        echo -n "$((RANDOM % 10000)) " >> $output_file
    done
    echo "" >> $output_file
}

# Function to generate prime search range
generate_prime_range() {
    local end=$1
    local output_file=$2
    echo "Generating prime search range 2 to $end..."
    echo "2 $end" > $output_file
}

# Function to run algorithm and capture results
run_algorithm() {
    local algo_num=$1
    local input_file=$2
    local output_file=$3
    local result_file=$4
    local target=$5  # Only used for Quick Search
    
    # Prepare input
    cp $input_file in.txt
    
    echo "Running algorithm $algo_num with input file $input_file..."
    
    # Run with 8 processes and time it
    if [ $algo_num -eq 1 ] && [ ! -z "$target" ]; then
        # For Quick Search, we need to automate the target input
        echo "Running Quick Search for target $target..."
        (time mpiexec -n 8 ./program <<< $'1\n'"$target"$'\ny\n0') 2> $RESULTS_DIR/time_output.txt
    else
        # For other algorithms
        (time mpiexec -n 8 ./program <<< $algo_num$'\ny\n0') 2> $RESULTS_DIR/time_output.txt
    fi
    
    # Extract timing info
    real_time=$(grep "real" $RESULTS_DIR/time_output.txt | awk '{print $2}')
    user_time=$(grep "user" $RESULTS_DIR/time_output.txt | awk '{print $2}')
    sys_time=$(grep "sys" $RESULTS_DIR/time_output.txt | awk '{print $2}')
    
    # Copy output to result file
    echo "Algorithm: $(case $algo_num in 1) echo "Quick Search";; 2) echo "Prime Number Search";; 3) echo "Bitonic Sort";; 4) echo "Radix Sort";; 5) echo "Sample Sort";; esac)" > $result_file
    echo "Input Size: $(wc -w < $input_file)" >> $result_file
    echo "Real Time: $real_time" >> $result_file
    echo "User Time: $user_time" >> $result_file
    echo "System Time: $sys_time" >> $result_file
    echo "=======================================" >> $result_file
    
    # Append the actual output
    cat out.txt >> $result_file
    echo "" >> $result_file
    echo "=======================================" >> $result_file
}

# Function to run algorithm with specific number of cores
run_algorithm_with_cores() {
    local algo_num=$1
    local input_file=$2
    local output_file=$3
    local result_file=$4
    local cores=$5
    local target=$6  # Only used for Quick Search
    
    # Prepare input
    cp $input_file in.txt
    
    echo "Running algorithm $algo_num with $cores cores..."
    
    # Run with specified number of processes and time it
    if [ $algo_num -eq 1 ] && [ ! -z "$target" ]; then
        # For Quick Search, we need to automate the target input
        echo "Running Quick Search for target $target with $cores cores..."
        (time mpiexec -n $cores ./program <<< $'1\n'"$target"$'\ny\n0') 2> $RESULTS_DIR/time_output.txt
    else
        # For other algorithms
        (time mpiexec -n $cores ./program <<< $algo_num$'\ny\n0') 2> $RESULTS_DIR/time_output.txt
    fi
    
    # Extract timing info
    real_time=$(grep "real" $RESULTS_DIR/time_output.txt | awk '{print $2}')
    user_time=$(grep "user" $RESULTS_DIR/time_output.txt | awk '{print $2}')
    sys_time=$(grep "sys" $RESULTS_DIR/time_output.txt | awk '{print $2}')
    
    # Copy output to result file
    echo "Algorithm: $(case $algo_num in 1) echo "Quick Search";; 2) echo "Prime Number Search";; 3) echo "Bitonic Sort";; 4) echo "Radix Sort";; 5) echo "Sample Sort";; esac)" > $result_file
    echo "Cores: $cores" >> $result_file
    echo "Input Size: $(wc -w < $input_file)" >> $result_file
    echo "Real Time: $real_time" >> $result_file
    echo "User Time: $user_time" >> $result_file
    echo "System Time: $sys_time" >> $result_file
    echo "=======================================" >> $result_file
    
    # Append the actual output
    cat out.txt >> $result_file
    echo "" >> $result_file
    echo "=======================================" >> $result_file
}

echo "Starting parallel algorithm demonstrations with 8 cores..."

# Array sizes to test
SMALL_SIZE=1024
MEDIUM_SIZE=4096
LARGE_SIZE=16384
PRIME_SMALL=10000
PRIME_MEDIUM=100000
PRIME_LARGE=1000000

# Create input data directory
INPUT_DIR="$RESULTS_DIR/inputs"
mkdir -p $INPUT_DIR

# Generate inputs for sorting algorithms
generate_random_array $SMALL_SIZE "$INPUT_DIR/small_random.txt"
generate_random_array $MEDIUM_SIZE "$INPUT_DIR/medium_random.txt"
generate_random_array $LARGE_SIZE "$INPUT_DIR/large_random.txt"

# Generate inputs for prime search
generate_prime_range $PRIME_SMALL "$INPUT_DIR/prime_small.txt"
generate_prime_range $PRIME_MEDIUM "$INPUT_DIR/prime_medium.txt"
generate_prime_range $PRIME_LARGE "$INPUT_DIR/prime_large.txt"

# Run sorting algorithms on different inputs
for algo in 3 4 5; do  # 3=Bitonic, 4=Radix, 5=Sample
    echo "Testing algorithm $algo..."
    
    mkdir -p "$RESULTS_DIR/algo_$algo"
    
    # Test small input
    run_algorithm $algo "$INPUT_DIR/small_random.txt" "out.txt" "$RESULTS_DIR/algo_$algo/small_result.txt"
    
    # Test medium input
    run_algorithm $algo "$INPUT_DIR/medium_random.txt" "out.txt" "$RESULTS_DIR/algo_$algo/medium_result.txt"
    
    # Test large input
    run_algorithm $algo "$INPUT_DIR/large_random.txt" "out.txt" "$RESULTS_DIR/algo_$algo/large_result.txt"
done

# Run Prime Number Search on different ranges
echo "Testing Prime Number Search..."
mkdir -p "$RESULTS_DIR/algo_2"  # 2=Prime Number Search
run_algorithm 2 "$INPUT_DIR/prime_small.txt" "out.txt" "$RESULTS_DIR/algo_2/small_result.txt"
run_algorithm 2 "$INPUT_DIR/prime_medium.txt" "out.txt" "$RESULTS_DIR/algo_2/medium_result.txt"
run_algorithm 2 "$INPUT_DIR/prime_large.txt" "out.txt" "$RESULTS_DIR/algo_2/large_result.txt"

# Run Quick Search with different targets
echo "Testing Quick Search..."
mkdir -p "$RESULTS_DIR/algo_1"  # 1=Quick Search
generate_random_array 1000 "$INPUT_DIR/search_small.txt"
SEARCH_TARGET=$((RANDOM % 1000))  # Random target
run_algorithm 1 "$INPUT_DIR/search_small.txt" "out.txt" "$RESULTS_DIR/algo_1/small_result.txt" $SEARCH_TARGET

generate_random_array 10000 "$INPUT_DIR/search_medium.txt"
SEARCH_TARGET=$((RANDOM % 10000))  # Random target
run_algorithm 1 "$INPUT_DIR/search_medium.txt" "out.txt" "$RESULTS_DIR/algo_1/medium_result.txt" $SEARCH_TARGET

generate_random_array 100000 "$INPUT_DIR/search_large.txt"
SEARCH_TARGET=$((RANDOM % 100000))  # Random target
run_algorithm 1 "$INPUT_DIR/search_large.txt" "out.txt" "$RESULTS_DIR/algo_1/large_result.txt" $SEARCH_TARGET

# Generate analysis reports
echo "Generating analysis reports..."

# Function to generate analysis report
generate_analysis() {
    local algo_num=$1
    local algo_name=$2
    local output_file=$3
    
    echo "# $algo_name Performance Analysis" > $output_file
    echo "" >> $output_file
    echo "## Test Configuration" >> $output_file
    echo "- Date: $(date)" >> $output_file
    echo "- Number of processes: 8" >> $output_file
    echo "" >> $output_file
    echo "## Results" >> $output_file
    echo "" >> $output_file
    
    # Create proper markdown table
    echo "| Input Size | Real Time (ms) | User Time (ms) | System Time (ms) |" >> $output_file
    echo "|------------|----------------|----------------|------------------|" >> $output_file
    
    # Function to convert time to milliseconds
    convert_to_ms() {
        local time_str=$1
        local minutes=$(echo $time_str | grep -o '[0-9]\+m' | grep -o '[0-9]\+' || echo "0")
        local seconds=$(echo $time_str | grep -o '[0-9]\+\.[0-9]\+s' | grep -o '[0-9]\+\.[0-9]\+' || echo "0")
        # Convert to milliseconds: (minutes * 60 + seconds) * 1000
        echo "$minutes * 60000 + $seconds * 1000" | bc | cut -d'.' -f1
    }
    
    # Extract small size results
    if [ -f "$RESULTS_DIR/algo_$algo_num/small_result.txt" ]; then
        input_size=$(grep "Input Size:" "$RESULTS_DIR/algo_$algo_num/small_result.txt" | awk '{print $3}')
        real_time=$(grep "Real Time:" "$RESULTS_DIR/algo_$algo_num/small_result.txt" | awk '{print $3}')
        user_time=$(grep "User Time:" "$RESULTS_DIR/algo_$algo_num/small_result.txt" | awk '{print $3}')
        sys_time=$(grep "System Time:" "$RESULTS_DIR/algo_$algo_num/small_result.txt" | awk '{print $3}')
        
        # Convert times to milliseconds
        real_time_ms=$(convert_to_ms "$real_time")
        user_time_ms=$(convert_to_ms "$user_time")
        sys_time_ms=$(convert_to_ms "$sys_time")
        
        echo "| $input_size | $real_time_ms | $user_time_ms | $sys_time_ms |" >> $output_file
    fi
    
    # Extract medium size results
    if [ -f "$RESULTS_DIR/algo_$algo_num/medium_result.txt" ]; then
        input_size=$(grep "Input Size:" "$RESULTS_DIR/algo_$algo_num/medium_result.txt" | awk '{print $3}')
        real_time=$(grep "Real Time:" "$RESULTS_DIR/algo_$algo_num/medium_result.txt" | awk '{print $3}')
        user_time=$(grep "User Time:" "$RESULTS_DIR/algo_$algo_num/medium_result.txt" | awk '{print $3}')
        sys_time=$(grep "System Time:" "$RESULTS_DIR/algo_$algo_num/medium_result.txt" | awk '{print $3}')
        
        # Convert times to milliseconds
        real_time_ms=$(convert_to_ms "$real_time")
        user_time_ms=$(convert_to_ms "$user_time")
        sys_time_ms=$(convert_to_ms "$sys_time")
        
        echo "| $input_size | $real_time_ms | $user_time_ms | $sys_time_ms |" >> $output_file
    fi
    
    # Extract large size results
    if [ -f "$RESULTS_DIR/algo_$algo_num/large_result.txt" ]; then
        input_size=$(grep "Input Size:" "$RESULTS_DIR/algo_$algo_num/large_result.txt" | awk '{print $3}')
        real_time=$(grep "Real Time:" "$RESULTS_DIR/algo_$algo_num/large_result.txt" | awk '{print $3}')
        user_time=$(grep "User Time:" "$RESULTS_DIR/algo_$algo_num/large_result.txt" | awk '{print $3}')
        sys_time=$(grep "System Time:" "$RESULTS_DIR/algo_$algo_num/large_result.txt" | awk '{print $3}')
        
        # Convert times to milliseconds
        real_time_ms=$(convert_to_ms "$real_time")
        user_time_ms=$(convert_to_ms "$user_time")
        sys_time_ms=$(convert_to_ms "$sys_time")
        
        echo "| $input_size | $real_time_ms | $user_time_ms | $sys_time_ms |" >> $output_file
    fi
    
    echo "" >> $output_file
    echo "## Performance Analysis" >> $output_file
    echo "" >> $output_file
    echo "The $algo_name algorithm was tested with 8 processes on different input sizes. The results show how the algorithm's performance scales with increasing input size." >> $output_file
    echo "" >> $output_file
    echo "### Observations" >> $output_file
    echo "" >> $output_file
    echo "- Write your observations based on the results" >> $output_file
    echo "- Compare with sequential performance if available" >> $output_file
    echo "- Analyze scalability and efficiency" >> $output_file
    echo "" >> $output_file
}

# Generate reports for each algorithm
generate_analysis 1 "Quick Search" "$RESULTS_DIR/quick_search_analysis.md"
generate_analysis 2 "Prime Number Search" "$RESULTS_DIR/prime_search_analysis.md"
generate_analysis 3 "Bitonic Sort" "$RESULTS_DIR/bitonic_sort_analysis.md"
generate_analysis 4 "Radix Sort" "$RESULTS_DIR/radix_sort_analysis.md"
generate_analysis 5 "Sample Sort" "$RESULTS_DIR/sample_sort_analysis.md"

# Add core scaling analysis section
echo "Starting core scaling analysis with medium-sized inputs..."

# Directory for core scaling results
SCALING_DIR="$RESULTS_DIR/core_scaling"
mkdir -p $SCALING_DIR

# Function to generate core scaling analysis report
generate_scaling_analysis() {
    local algo_num=$1
    local algo_name=$2
    local output_file=$3
    
    echo "# $algo_name Core Scaling Analysis" > $output_file
    echo "" >> $output_file
    echo "## Test Configuration" >> $output_file
    echo "- Date: $(date)" >> $output_file
    echo "- Fixed input size (medium)" >> $output_file
    echo "" >> $output_file
    echo "## Results" >> $output_file
    echo "" >> $output_file
    
    # Create proper markdown table
    echo "| Number of Cores | Real Time (ms) | User Time (ms) | System Time (ms) | Speedup |" >> $output_file
    echo "|-----------------|----------------|----------------|------------------|---------|" >> $output_file
    
    # Function to convert time to milliseconds
    convert_to_ms() {
        local time_str=$1
        local minutes=$(echo $time_str | grep -o '[0-9]\+m' | grep -o '[0-9]\+' || echo "0")
        local seconds=$(echo $time_str | grep -o '[0-9]\+\.[0-9]\+s' | grep -o '[0-9]\+\.[0-9]\+' || echo "0")
        # Convert to milliseconds: (minutes * 60 + seconds) * 1000
        echo "$minutes * 60000 + $seconds * 1000" | bc | cut -d'.' -f1
    }
    
    local base_time_ms=""
    
    # Process results for each core count
    for cores in 1 2 4 8; do
        if [ -f "$SCALING_DIR/algo_$algo_num/cores_${cores}_result.txt" ]; then
            real_time=$(grep "Real Time:" "$SCALING_DIR/algo_$algo_num/cores_${cores}_result.txt" | awk '{print $3}')
            user_time=$(grep "User Time:" "$SCALING_DIR/algo_$algo_num/cores_${cores}_result.txt" | awk '{print $3}')
            sys_time=$(grep "System Time:" "$SCALING_DIR/algo_$algo_num/cores_${cores}_result.txt" | awk '{print $3}')
            
            # Convert times to milliseconds
            real_time_ms=$(convert_to_ms "$real_time")
            user_time_ms=$(convert_to_ms "$user_time")
            sys_time_ms=$(convert_to_ms "$sys_time")
            
            # Calculate speedup
            if [ $cores -eq 1 ]; then
                base_time_ms=$real_time_ms
                speedup="1.00"
            else
                # Avoid division by zero
                if (( $base_time_ms > 0 )); then
                    speedup=$(echo "scale=2; $base_time_ms / $real_time_ms" | bc -l)
                else
                    speedup="N/A"
                fi
            fi
            
            echo "| $cores | $real_time_ms | $user_time_ms | $sys_time_ms | $speedup |" >> $output_file
        fi
    done
    
    echo "" >> $output_file
    echo "## Scaling Analysis" >> $output_file
    echo "" >> $output_file
    echo "This analysis shows how $algo_name scales with increasing number of processor cores while keeping the input size constant." >> $output_file
    echo "" >> $output_file
    echo "### Observations" >> $output_file
    echo "" >> $output_file
    echo "- Write your observations about scaling efficiency" >> $output_file
    echo "- Analyze if the algorithm achieves linear speedup" >> $output_file
    echo "- Identify potential bottlenecks in parallelization" >> $output_file
    echo "- Ideal speedup would be equal to the number of cores" >> $output_file
    echo "" >> $output_file
}

# Run core scaling tests for each algorithm
echo "Testing core scaling for sorting algorithms..."

# Create directories for core scaling results
for algo in 1 2 3 4 5; do
    mkdir -p "$SCALING_DIR/algo_$algo"
done

# Run Quick Search with different core counts
echo "Testing Quick Search scaling..."
SEARCH_TARGET=$((RANDOM % 10000))  # Random target for medium-sized array
for cores in 1 2 4 8; do
    run_algorithm_with_cores 1 "$INPUT_DIR/search_medium.txt" "out.txt" "$SCALING_DIR/algo_1/cores_${cores}_result.txt" $cores $SEARCH_TARGET
done

# Run Prime Number Search with different core counts
echo "Testing Prime Number Search scaling..."
for cores in 1 2 4 8; do
    run_algorithm_with_cores 2 "$INPUT_DIR/prime_medium.txt" "out.txt" "$SCALING_DIR/algo_2/cores_${cores}_result.txt" $cores
done

# Run Bitonic Sort with different core counts
echo "Testing Bitonic Sort scaling..."
for cores in 1 2 4 8; do
    run_algorithm_with_cores 3 "$INPUT_DIR/medium_random.txt" "out.txt" "$SCALING_DIR/algo_3/cores_${cores}_result.txt" $cores
done

# Run Radix Sort with different core counts
echo "Testing Radix Sort scaling..."
for cores in 1 2 4 8; do
    run_algorithm_with_cores 4 "$INPUT_DIR/medium_random.txt" "out.txt" "$SCALING_DIR/algo_4/cores_${cores}_result.txt" $cores
done

# Run Sample Sort with different core counts
echo "Testing Sample Sort scaling..."
for cores in 1 2 4 8; do
    run_algorithm_with_cores 5 "$INPUT_DIR/medium_random.txt" "out.txt" "$SCALING_DIR/algo_5/cores_${cores}_result.txt" $cores
done

# Generate scaling analysis reports
generate_scaling_analysis 1 "Quick Search" "$SCALING_DIR/quick_search_scaling_analysis.md"
generate_scaling_analysis 2 "Prime Number Search" "$SCALING_DIR/prime_search_scaling_analysis.md"
generate_scaling_analysis 3 "Bitonic Sort" "$SCALING_DIR/bitonic_sort_scaling_analysis.md"
generate_scaling_analysis 4 "Radix Sort" "$SCALING_DIR/radix_sort_scaling_analysis.md"
generate_scaling_analysis 5 "Sample Sort" "$SCALING_DIR/sample_sort_scaling_analysis.md"

echo "Core scaling analysis completed! Results are stored in the $SCALING_DIR directory"

echo "Demo completed! Results are stored in the $RESULTS_DIR directory"