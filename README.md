# Parallel Search and Sorting Algorithms with MPI

## Project Overview

This project focuses on the implementation and analysis of key parallel algorithms using the Message Passing Interface (MPI). The work demonstrates efficient parallel computing techniques for solving computational problems, including Quick Search, Prime Number Finding, Bitonic Sort, Radix Sort, and Sample Sort. Through rigorous development and experimentation, the project showcases MPI programming skills and advanced parallel algorithm design, communication strategies, and performance evaluation on distributed systems.

## Repository Structure

- **Source Code**: Core implementation files for each algorithm
- **Documentation**: Detailed markdown files in the `docs/` directory explaining each algorithm
- **Input Data**: Sample datasets in the `inputs/` directory
- **Analysis**: Performance measurements and scaling analysis in `performance_summary.md`
- **Visualization**: Python script for generating performance graphs

## Algorithms Implemented

The following parallel algorithms have been implemented with comprehensive documentation:

1. **Quick Search** - A parallel search algorithm using divide-and-conquer
   - [Documentation](docs/Quick_Search.md)

2. **Prime Number Finding** - Parallel techniques to find prime numbers within a specified range
   - [Documentation](docs/Prime_Number_Search.md)

3. **Bitonic Sort** - Parallel implementation of the Bitonic sorting algorithm
   - [Documentation](docs/Bitonic_Sort.md)

4. **Radix Sort** - Distributed version of Radix Sort with parallelized counting and distribution phases
   - [Documentation](docs/Radix_Sort.md)

5. **Sample Sort** - Implementation using MPI collective communication to gather and redistribute data efficiently
   - [Documentation](docs/Sample_Sort.md)

## Documentation

Each algorithm is thoroughly documented in its respective markdown file under the `docs/` directory. The documentation includes:

- **Core Functions Explanation**: Detailed breakdowns of each function's purpose and implementation
- **Main Wrapper Functions**: How the algorithms interface with the testing framework
- **Key MPI Functions**: Detailed explanations of the MPI primitives used
- **Step-by-Step Examples**: Tracing through algorithm execution with sample data
- **Performance Considerations**: Analysis of efficiency and optimization opportunities

## Performance Analysis

The project includes comprehensive performance analysis:

- **Execution Time by Input Size**: See `performance_summary.md` for detailed measurements
- **Core Scaling Analysis**: How each algorithm scales with increasing processor cores
- **Visualization**: Charts and graphs generated using `visualization.py`

The performance analysis demonstrates the speedup and efficiency of each algorithm using different numbers of MPI processes and data sizes, with particular attention to communication overheads and scalability.

## Running the Code

For a single algorithm test:

```bash
./run.sh
```

Follow the prompts to select which algorithm to run.

## Team Contributors

- **Moaz**: Prime Number Finding
- **Mariam**: Sample Sort
- **Hussein**: Bitonic Sort
- **Farah**: Quick Search
- **Basma**: Radix Sort

## Project Information

This project was developed as part of the Parallel Computing course. The implementation demonstrates:

- Efficient problem decomposition and parallelization
- Optimized data distribution and communication patterns
- Performance analysis and optimization techniques
- Distributed computing using MPI across multiple nodes

## References and Resources

- MPI documentation and best practices
- Parallel algorithm design patterns
- Performance optimization techniques for distributed systems

---

**Note**: For detailed information about a specific algorithm, please refer to its documentation file in the `docs/` directory.