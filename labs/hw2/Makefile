all:
	@g++ hw2.cpp -o hw2

clean:
	@rm -f hw2

run: all
	@./hw2 7001

zip:
	rm -rf 311511034_eos_hw2
	rm -rf 311511034_eos_hw2.zip
	mkdir 311511034_eos_hw2
	cp Makefile 311511034_eos_hw2
	cp hw2.cpp 311511034_eos_hw2
	zip -r 311511034_eos_hw2.zip 311511034_eos_hw2
	rm -rf 311511034_eos_hw2
