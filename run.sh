mpic++ -o program source.cpp Parallel\ Code/Prime_Number_Search.cpp Parallel\ Code/Bitonic_Sort.cpp Parallel\ Code/Sample_Sort.cpp Parallel\ Code/Quick_Search.cpp;
mpiexec -n 4 ./program