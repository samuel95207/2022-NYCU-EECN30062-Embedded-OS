all: game_cpp guess_cpp

game_cpp:
	g++ game.cpp -o game

guess_cpp:
	g++ guess.cpp -o guess


clean:
	@rm -f game
	@rm -f guess


zip:
	rm -rf 311511034_eos_lab7
	rm -rf 311511034_eos_lab7.zip
	mkdir 311511034_eos_lab7
	cp Makefile 311511034_eos_lab7
	cp game.cpp 311511034_eos_lab7
	cp guess.cpp 311511034_eos_lab7
	cp demo.sh 311511034_eos_lab7
	zip -r 311511034_eos_lab7.zip 311511034_eos_lab7
	rm -rf 311511034_eos_lab7
