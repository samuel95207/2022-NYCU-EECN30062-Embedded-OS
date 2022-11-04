all:
	g++ lab5.cpp -o lab5

clean:
	rm -f lab5

zip:
	rm -rf 311511034_eos_lab5
	rm -rf 311511034_eos_lab5.zip
	mkdir 311511034_eos_lab5
	cp Makefile 311511034_eos_lab5
	cp lab5.cpp 311511034_eos_lab5
	cp demo.sh 311511034_eos_lab5
	zip -r 311511034_eos_lab5.zip 311511034_eos_lab5
	rm -rf 311511034_eos_lab5