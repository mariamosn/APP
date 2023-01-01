run: build
	mpirun --oversubscribe -np 3 mpi_classic

build: mpi_classic.cpp
	mpic++ mpi_classic.cpp -o mpi_classic

clean:
	rm -rf ./mpi_classic
