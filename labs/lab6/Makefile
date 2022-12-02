all:
	g++ server.cpp -o server
	g++ client.cpp -o client

clean:
	rm -f server
	rm -f client

zip:
	rm -rf 311511034_eos_lab6
	rm -rf 311511034_eos_lab6.zip
	mkdir 311511034_eos_lab6
	cp Makefile 311511034_eos_lab6
	cp server.cpp 311511034_eos_lab6
	cp client.cpp 311511034_eos_lab6
	cp demo.sh 311511034_eos_lab6
	zip -r 311511034_eos_lab6.zip 311511034_eos_lab6
	rm -rf 311511034_eos_lab6