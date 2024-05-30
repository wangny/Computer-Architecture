single_cycle: main.o readfile.o Decode.o
	g++ -o single_cycle main.o readfile.o Decode.o
main.o readfile.o Decode.o: main.cpp readfile.cpp Decode.cpp
	g++ -c -O2  -Wall -std=c++11 main.cpp readfile.cpp Decode.cpp
clean: 
	rm -f single_cycle main.o readfile.o Decode.o



