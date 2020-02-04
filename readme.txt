You can execute the following commands to compile and run the sample application.

* Windows (MPICH2)
>>gcc -L"C:\Program Files (x86)\MPICH2\lib" -I"C:\Program Files (x86)\MPICH2\include" mpi_ps.c -lmpi -o mpi_ps.exe
>>mpiexec -n 3 ./mpi_ps.exe


* Unix/Max (OpenMPI)

>>mpicc mpi_ps.c -o mpi_ps.o
>>mpirun -np 2 ./mpi_ps.o


