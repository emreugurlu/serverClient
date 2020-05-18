#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include<pthread.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
int main(){
    printf("We will first test the 'add' function\n");
    chdir("client");
    system("./WTF add project1 test1.txt");
    system("./WTF add project1 test2.txt");
    system("./WTF add project1 test3.txt");
    system("./WTF add project1 test4.txt");
    printf("Success!\n");
   // printf("We will now do the create function\n");
    //printf("First we must set up the server\n");
    printf("iniating the configure command\n");
    system("./WTF configure 127.0.0.1 8001");
    printf("Success!\n");
    printf("Now we will set up the server\n");
    chdir("..");
    chdir("server");
    system("./WTFserver 8001 &");
    printf("System is set up, we will now commence 'Create' a project2");
    chdir("..");
    chdir("client");
    system("./WTF create project2");
    printf("Success!\n");
    printf("We will now 'Destroy' project2\n");
    system("./WTF destroy project2");
    printf("Success!\n");
    printf("We will now test the 'remove' function\n");
    system("./WTF remove project1 test1.txt");
    system("./WTF remove project1 test2.txt");
    system("./WTF remove project1 test3.txt");
    system("./WTF remove project1 test4.txt");
    printf("Success!\n");
    system("rm -r project2");
    chdir("..");
    chdir("server");
    system("rm -r project2");
    system("rm -r past_versions_project2");
}