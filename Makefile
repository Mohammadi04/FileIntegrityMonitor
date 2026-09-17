fim: src/main.cpp 
	clang++ -std=c++17 -Wall -Wextra src/main.cpp -o fim

.PHONY: clean
clean:
	rm -f fim
