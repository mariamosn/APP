run: build
	mpirun --oversubscribe -np 6 mpi_classic

build: mpi_classic.cpp
	mpic++ mpi_classic.cpp -o mpi_classic

clean:
	rm -rf ./mpi_classic
