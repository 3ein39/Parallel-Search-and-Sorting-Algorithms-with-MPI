# Parallel Search and Sorting Algorithms with MPI

## Project Title

Parallel Search and Sorting Algorithms with MPI

## Description

This project focuses on the implementation and analysis of key parallel algorithms using the Message Passing Interface (MPI). Students will apply parallel computing techniques to solve computational problems efficiently, including Quick Search, Prime Number Finding, Bitonic Sort, Radix Sort, and Sample Sort. Through hands-on development and experimentation, the project aims to reinforce MPI programming skills and promote understanding of parallel algorithm design, communication strategies, and performance evaluation on distributed systems.

## Team

This is a team-based project. Each student must join a team of 3 at minimum and 6 members at maximum.

**Course Team:**

  * Dr. Masoud Ismail
  * Eng. Rehab Mahmoud   
  * Eng. Mahmoud Badry 

## Project Submission Information

  * Each team should send the names of the members over email before 27-04-2025.
  * The project will be discussed onsite in the faculty.
  * Submission Date: Final Exam Day (TBD). Each team will have an announced delivery time that will be public on our Google's classroom.

## Marking and Assessment

This assignment will be marked out of 120%. 20% of the grade is an extra credit grade (Bonus).

## Learning Outcomes

  * Understanding of Parallel Programming Concepts
  * MPI Programming Skills
  * Problem Decomposition and Parallelization
  * Data Distribution and Communication
  * Performance Optimization
  * Teamwork and Collaboration

## Details

### Problem Distribution and Input Handling

The master process is responsible for reading the input data (e.g., array, dataset, or number range) and distributing the necessary portions to worker processes. Students should design their programs to support dynamic input sizes and flexible process partitioning.

### Parallel Algorithms to Implement (Choose 4 - The 5th one is a bonus)

  * Quick Search: Implement a parallel search algorithm using divide-and-conquer, where each process searches a subset of the data.
  * Prime Number Finding: Use parallel techniques to find all prime numbers within a specified range.
  * Bitonic Sort: Implement the Bitonic sorting algorithm and parallelize its compare-and-swap steps using MPI.
  * Radix Sort: Develop a distributed version of Radix Sort where counting and distribution phases are parallelized.
  * Sample Sort: Implement Sample Sort using MPI collective communication to gather and redistribute data efficiently.
  * Algorithm Visualization is required to be presented.

#### Team tasks 

* Moaz: Prime Number Finding
* Eng Mariam: Sample Sort
* Hussein: Bitonic Sort
* Farah: Quick Search
* Basma: Radix Sort

### Performance Analysis

  * Students must measure the speedup and efficiency of each algorithm using different numbers of MPI processes and data sizes.
  * Comparison with sequential versions (optional but recommended for deeper insight).
  * Analyze communication overheads and scalability.
  * Running in a cluster of $N \> 2$ (Two machines or more).

### Submission Files

  * Source code
  * Documentation detailing the design, implementation, and performance analysis of Parallel Search and Sorting Algorithms with MPI
  * Presentation or demonstration showcasing the project functionality, performance results, and any additional features implemented

**Important Notes:**

  * IT IS YOUR RESPONSIBILITY TO KEEP RECORDS OF ALL THE WORK SUBMITTED.
  * COPYING FROM EACH OTHER ISN'T ALLOWED.