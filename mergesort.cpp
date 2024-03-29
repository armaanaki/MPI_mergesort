#include <cstdio>
#include <mpi.h>
#include "read_file.h"

// declare methods used later
bool get_input(int argc, char** argv, double*& init_array, unsigned long long& array_size, int comm_sz);
void merge(double*& array, int left_index, int middle_index, int right_index);
void mergesort(double*& array, int left_index, int right_index);

int main (int argc, char** argv) {
    // init variables
    int my_rank, comm_sz;
    unsigned long long array_size;
    double* init_array;
    double* my_array;
    
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

    // init arrays for all processes then scatter -- 
    // note: my method creates full length arrays on each processor, this is memory inefficent but allows us to not keep creating arrays as creating arrays takes a lot of work
    int nums_per = array_size/comm_sz;
    my_array = new double[array_size];
    MPI_Scatter(init_array, nums_per, MPI_DOUBLE, my_array, nums_per, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // sort each array on each proc
    mergesort(my_array, 0, nums_per - 1);

    // merge to rank 0
    for (int power = 1; power < comm_sz; power *= 2) {
        for (int i = 0; i < comm_sz; i += (power*2)) {
            if (my_rank == i) {
                MPI_Recv(&my_array[nums_per], nums_per, MPI_DOUBLE, i+power, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                merge(my_array, 0, nums_per - 1, (nums_per*2) - 1);
            } else if (my_rank == i+power) {
                MPI_Send(my_array, nums_per, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        nums_per *= 2;
    }

    // print the sorted array
    if (my_rank == 0) {
	 	printf("{ ");
			for (int i = 0; i < array_size; i++)
				printf("%.2f ", my_array[i]);
		printf("}\n");
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

// method to mergesort
void mergesort(double*& array, int left_index, int right_index) {
    if (left_index < right_index) {
        int middle_index = (right_index+left_index)/2;

        mergesort(array, left_index, middle_index);
        mergesort(array, middle_index + 1, right_index);

        merge(array, left_index, middle_index, right_index);
    } 
}

// method to merge arrays assuming middle_index is the breakpoint
void merge(double*& array, int left_index, int middle_index, int right_index) {
    double* temp_array = new double[right_index - left_index + 1];
    int temp_array_index = 0;
    int left_array_index = left_index;
    int right_array_index = middle_index + 1;

    // sort the left and right arrays into the temp array when possible
    while (left_array_index <= middle_index && right_array_index <= right_index) {
        if (array[left_array_index] < array[right_array_index]) {
            temp_array[temp_array_index] = array[left_array_index];
            left_array_index++;
        } else {
            temp_array[temp_array_index] = array[right_array_index];
            right_array_index++;
        }
        temp_array_index++;
    }
    
    // fill in remaining left and right indexes if there are any
    while (left_array_index <= middle_index) {
        temp_array[temp_array_index] = array[left_array_index];
        left_array_index++;
        temp_array_index++;
    }
    while (right_array_index <= right_index) {
        temp_array[temp_array_index] = array[right_array_index];
        right_array_index++;
        temp_array_index++;
    }

    // fill the temp array into the main array
    for (int i = left_index; i <= right_index; i++) 
        array[i] = temp_array[i - left_index];
}
