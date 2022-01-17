all : main.cpp
	riscv64-unknown-elf-g++ -std=c++14 main.cpp -o main
	
sim: all
	riscv-vp --intercept-syscalls   main
	
dump-elf: all
	riscv64-unknown-elf-readelf -a main
	
dump-code: all
	riscv64-unknown-elf-objdump -D main
	
clean:
	rm -f main
