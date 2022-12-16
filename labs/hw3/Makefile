all:
	g++ hw3.cpp -o hw3

clean:
	@rm -f hw3

run: all
	@./hw3 7001

zip:
	rm -rf 311511034_eos_hw3
	rm -rf 311511034_eos_hw3.zip
	mkdir 311511034_eos_hw3
	cp Makefile 311511034_eos_hw3
	cp hw3.cpp 311511034_eos_hw3
	zip -r 311511034_eos_hw3.zip 311511034_eos_hw3
	rm -rf 311511034_eos_hw3
