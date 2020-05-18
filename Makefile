all: WTFtest WTF WTFserver
WTFtest:
	gcc WTFtest.c -o WTFtest -lcrypto -lssl -lpthread
WTF:
	gcc ./client/WTF.c -o ./client/WTF -lcrypto -lssl
WTFserver:
	gcc ./server/WTFserver.c -o ./server/WTFserver -lpthread
clean:
	rm -f WTFtest
	rm -f ./server/WTFserver
	rm -f ./client/WTF
	rm -r ./client/project1
	rm -r ./server/project1
	rm -r ./server/project5
test:
	mkdir ./client/project1
	mkdir ./server/project1
	echo -n "This is test1.txt" > ./client/project1/test1.txt
	echo -n "This is test2.txt" > ./client/project1/test2.txt
	echo -n "This is test3.txt" > ./client/project1/test3.txt
	echo -n "This is test4.txt" > ./client/project1/test4.txt
	echo -n "0" > ./client/project1/.Manifest
	mkdir ./server/project5
	echo -n "THIS IS TO TEST CHECKOUT"> ./server/project5/test5.txt
	echo -n "0" > ./server/project5/.Manifest