build:trivia.c
	gcc -Wall -g trivia.c -o trivia -lncurses

run:trivia
	./trivia questions0 questions1 questions2

clean:trivia
	rm ./trivia