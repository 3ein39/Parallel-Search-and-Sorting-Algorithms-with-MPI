#include "mpi.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int rc;
    // Initialize the MPI environment with error checking
    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS) {
        printf("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    // Get the rank and size in the world communicator
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Get the hostname
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    
    printf("Hello from process %d of %d on host %s\n", rank, size, hostname);
    
    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}
