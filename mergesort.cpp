#include <cstdio>
#include <mpi.h>
#include <math.h>
#include "read_file.h"

// declare methods used later
bool get_input(int argc, char** argv, double*& init_array, unsigned long long& array_size, int comm_sz);

int main (int argc, char** argv) {
    // init variables
    int my_rank, comm_sz;
    unsigned long long array_size;
    double* init_array;
    
    // init MPI variables
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    // get file input and distribute input, if there is an error then all will exit
    bool error = false;
    if (my_rank == 0)
        error = get_input(argc, argv, init_array, array_size, comm_sz); 
    MPI_Bcast(&error, 1, MPI_CXX_BOOL, 0, MPI_COMM_WORLD);
    if (error) {
        MPI_Finalize();
        exit(-1);
    }

    // broadcast array size
    MPI_Bcast(&array_size, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

    // print the sorted array
    if (my_rank == 0) {
    }

    // tell MPI it's finished
    MPI_Finalize();
}

// method to read input, only called from root process
bool get_input(int argc, char** argv, double*& init_array, unsigned long long& array_size, int comm_sz) {
    if (argc != 2) {
        fprintf(stderr, "expected <array file name>\n");
        return true;
    } else {
        array_size = get_file_size(argv[1])/sizeof(double);
        init_array = read_from_file (argv[1], array_size);
        if (array_size % comm_sz != 0) {
            fprintf(stderr, "array size must be divisible by total processes");
            return true;
        }
    }

    return false;
}
