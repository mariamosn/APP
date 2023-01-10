run: build
	mpirun --oversubscribe -np 6 pipeline_MPI

build: pipeline_MPI.cpp
	mpic++ pipeline_MPI.cpp -o pipeline_MPI

clean:
	rm -rf ./pipeline_MPI
