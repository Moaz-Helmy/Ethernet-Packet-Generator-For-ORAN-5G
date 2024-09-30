# Compiler name
CC = g++

# Output executable file name
out_name = Compilation/programOutput

# List of object files in the project (replace each source.cpp with source.o)
object_list = Compilation/main.o Compilation/parser.o Compilation/ethernet.o Compilation/oran.o Compilation/ecpri.o

# Output target
output: directories $(object_list)
	$(CC) $(object_list) -o $(out_name)

# Directories for object files and debug files
directories:
	mkdir -p "Parser Debug Files"
	mkdir -p "Ethernet Debug Files"
	mkdir -p "ORAN Debug Files"
	mkdir -p "ECPRI Debug Files"
	mkdir -p "Compilation"
	mkdir -p "Detected Parameters"

# Source target 1
Compilation/main.o: main.cpp
	$(CC) -c main.cpp -o Compilation/main.o

# Source target 2
Compilation/parser.o: Sources/Parser/parser.cpp Sources/Parser/parser.h
	$(CC) -c Sources/Parser/parser.cpp -o Compilation/parser.o

# Source target 3
Compilation/ethernet.o: Sources/Ethernet/ethernet.cpp Sources/Ethernet/ethernet.h
	$(CC) -c Sources/Ethernet/ethernet.cpp -o Compilation/ethernet.o

Compilation/oran.o: Sources/ORAN/oran.cpp Sources/ORAN/oran.h
	$(CC) -c Sources/ORAN/oran.cpp -o Compilation/oran.o

Compilation/ecpri.o: Sources/ECPRI/ecpri.cpp Sources/ECPRI/ecpri.h
	$(CC) -c Sources/ECPRI/ecpri.cpp -o Compilation/ecpri.o
# Clean target to delete all generated object files and the executable
clean:
	rm -f Compilation/*.o $(out_name)
