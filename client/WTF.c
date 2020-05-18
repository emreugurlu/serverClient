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

struct Node { 
    //path name
    char* pathfile; 
    //version
    char* version;
    //hashcode
    char* hashcode;
}; 

typedef struct ManifestEntry{
    char* path;
    char* version;
    char* hash;
} ManifestEntry;

typedef struct Manifest{
    ManifestEntry* entries;
    int numOfEntries;
    char* version;
} Manifest;

typedef struct UpdateEntry{
    char* path;
    char* version;
    char* hash;
} UpdateEntry;

typedef struct Update{
    UpdateEntry* entries;
    int numOfEntries;
} Update;

Update* createUpdateStructureFromFile(int manifestfd){
    Update* manifest = (Update*)malloc(sizeof(Update));
    manifest->entries = (UpdateEntry*)malloc(20 * sizeof(UpdateEntry)); // size for 20 entries originally
    //manifest->version = (char*)malloc(10 * sizeof(char)); // size for a 9-digit number

    char buffer = '?';
    int size = 0;
    char* reallocPtr = NULL;

    //read(manifestfd, &buffer, 1);
    //printf("%c", buffer);
    //printf("manifest version number: '%s'\n", manifest->version);

    //int status = read(manifestfd, &buffer, 1);
    int status = 1;
    //printf("%c", buffer);
    int entryNum = 0;
    int allocatedBytesForPath = 100; // originally 100 bytes allocated for path

    while(status > 0){ // loop for whole manifest file and read components into manifest struct
        
        size = 0;
        manifest->entries[entryNum].version = (char*)malloc(10 * sizeof(char));
        //buffer = '?';

        //status = read(manifestfd, &buffer, 1);
        do{ // loop for version
            status = read(manifestfd, &buffer, 1);
            //printf("%c", buffer);
            manifest->entries[entryNum].version[size] = buffer;
            //printf("putting %c into version num for entry #%d\n", buffer, entryNum);
            size++;
            
        }while(buffer != ' ' && status > 0);

        manifest->entries[entryNum].version[size - 1] = '\0';
        //printf("version num for entry #%d is %s\n", entryNum, manifest->entries[entryNum].version);
        //buffer = '?';
        manifest->entries[entryNum].path = (char*)malloc((allocatedBytesForPath + 1) * sizeof(char));
        size = 0;
        //status = read(manifestfd, &buffer, 1);
        //printf("%c", buffer);
        do{ // loop for path

            if(size >= allocatedBytesForPath){ // realloc if necessary
                reallocPtr = realloc(manifest->entries[entryNum].path, 2 * allocatedBytesForPath * sizeof(char)); // double size of array
                allocatedBytesForPath *= 2;
                if(reallocPtr == NULL){
                    printf("Malloc failed when making .Manifest struct...how big is your .Manifest?...bad user\n");
                    return NULL;
                }else{
                    manifest->entries[entryNum].path = reallocPtr;
                }
            }

            status = read(manifestfd, &buffer, 1);
            manifest->entries[entryNum].path[size] = buffer;
            size++;
            
            //printf("%c", buffer);

        }while(buffer != ' ' && status > 0);
        manifest->entries[entryNum].path[size - 1] = '\0';
        //buffer = '?';
        size = 0;
        manifest->entries[entryNum].hash = (char*)malloc(33 * sizeof(char)); // hash has 32 bytes
        //status = read(manifestfd, &buffer, 1);
        //printf("%c", buffer);
        do{ // loop for hash
            status = read(manifestfd, &buffer, 1);
            manifest->entries[entryNum].hash[size] = buffer;
            size++;
            
            //printf("%c", buffer);
        }while(buffer != '\n' && status > 0);
        manifest->entries[entryNum].hash[size - 1] = '\0';
        //printf("the hash for entry #%d is %s\n", entryNum+1, manifest->entries[entryNum].hash);
        entryNum++;

        // check if at end of file here, because apparently C is a petty little b*tch with that
        status = read(manifestfd, &buffer, 1);
        if(status < 1){
            break;
        }else{
            lseek(manifestfd, -2, SEEK_CUR);
        }
        status = read(manifestfd, &buffer, 1);
    }
    manifest->numOfEntries = entryNum;

    lseek(manifestfd, 0, SEEK_SET);

    return manifest;
    

}

void deleteU(char* path, char* hash,char* projectName){//deletes entry in .Manifest given by .Update file
struct Node newnode;
//path is going to be ./project/file
//we also have the hash
printf("This is the path given by .Update:%s\n",path);
printf("This is the hash given by .Update:%s\n",hash);
//must create path to .Manifest
char pathToman[200]="./";
strcat(pathToman,projectName);
strcat(pathToman,"/");
strcat(pathToman,".Manifest");
printf("This is the path to .Manifest:%s\n",pathToman);
int manifest = open(pathToman,O_RDWR | O_APPEND | O_NONBLOCK, 00644);
char* manString = (char*)malloc(100*sizeof(char));
char* reallocPtr;
int filesize = 0;
int status = 0;
do{
    status = read(manifest,&(manString[filesize]),100);
    filesize += status;
    if(status == 100){
        reallocPtr = realloc(manString,2 * filesize * sizeof(char));
        if(reallocPtr ==NULL){
            printf("Malloc failed ... how big is your input? ... bad user\n");
            return;
        }else {
            manString = reallocPtr;
        }
    }
}while(status>0);
manString[filesize] = '\0';
close(manifest);
//we now have whatever is in manifest as a string.
//we know the length of hash is 33
//we know the length of the project name
char* path1 = (char*)malloc(strlen(path)*sizeof(char));
char* hash1 = (char*)malloc(strlen(hash)*sizeof(char));
int update = 0;
int line = 0;
int length = 0;
int found = 0;
int properline = 0;
int finalline = 0;
int removeline = 0;
int i = 0;
while(manString[i]!='\0'){
    if(manString[i]=='\n'){
        line++;
        printf("Line:%d\n",line);
        int j = i+1;
        while(manString[j]!=' '){
            length++;
            j++;
        }
        char* ver = (char*)malloc(length*sizeof(char));
        int m = 0;
        i++;//skipping newline char
        while(manString[i]!= ' '){
            ver[m] = manString[i];
            i++;
            m++;
        }
        ver[length]='\0';
        i++;
        int k = 0;//getting file path now
        while(k<strlen(path)){
            path1[k]=manString[i];
            k++;
            i++;
        }
        path1[strlen(path)]='\0';
        int l = i;
        if(strcmp(path1,path)==0){
            l++;
            found = 1;
            removeline = line;
            int k = 0;
            while(k<32){
                hash1[k] = manString[l];
                k++;
                l++;
            }
            hash1[32]='\0';
            printf("This is the hash of the matching PATH:%s\n",hash1);
            newnode.hashcode = hash1;
            newnode.version = ver;
        }
        printf("This is the version number:%s\n",ver);
        printf("This is the PATH:%s\n",path1);
    }
    i++;
}
finalline = line;
if(found == 1){
//update the text file
            //int newversion = atoi(newnode.version);
           // newversion = newversion + 1;
            
            int i = 0;
            int line = 0;
            int j = 0;
            //calculate size of line that has to be removed.
            int removal = 0;
            removal+=3;//two spaces,newline
            removal+=strlen(newnode.version);//version
            removal+=strlen(path);//path
            removal+=strlen(newnode.hashcode);//size of hash 
            printf("THIS IS HOW MUCH HAS TO BE REMOVED FROM OLD MANIFEST SIZE:%d\n",removal);
            int newsize = strlen(manString) - removal;
            char* newManifest = (char*)malloc(newsize * sizeof(char));
            printf("THIS IS NEW SIZE OF NEW MANIFEST:%d\n",newsize);
            printf("THIS IS OLD SIZE OF OLD MANIFEST:%d\n",strlen(manString));
            printf("THIS IS THE LINE BEING REMOVED:%d\n",removeline);
            //removal is how much we are skipping.
            int p = 0;
            //newManifest[0] = fileAsStr1[0];
            //int j = 0;
            newManifest[0] = manString[0];
            i++;
            j++;
            while(manString[i]!='\0'){
                if(manString[i]=='\n'){
                    line++;
                    newManifest[j] = manString[i];
                    j++;
                    i++;
                    if(removeline == line){
                        while(p!=removal){
                            p++;
                            i++;
                        }    

                    }
                }


                newManifest[j]=manString[i];
                i++;
                j++;
            }
            newManifest[newsize] = '\0';
            //int fd=open(directory,O_RDONLY | O_WRONLY | O_TRUNC);
            //close(fd);
            printf("THIS IS THE UPDATED MANIFEFST:\n%s\n",newManifest);
            printf("THIS IS THE OLD MANIFEFST:\n%s\n",manString);
            //Now write this to the .Manifest file
            int manifest = open(pathToman,O_RDONLY | O_WRONLY | O_TRUNC);
            write(manifest,newManifest,strlen(newManifest));
}




}

char* filesToSocketMessage(int* fds, int fds_size){ // input an array of file descriptors and the number of file descriptors in it, returns client-friendly string ready to write to socket
    int i;
    char* strForServer = (char*)malloc(101 * sizeof(char));
    strForServer[0] = '\0';
    char* reallocPtr = NULL;
    int allocatedBytesForString = 100; // originally space for 100 bytes
    int totalNumOfBytesRead = 0;
    int fileLength = 0;
    int status = 0;
    for(i = 0; i < fds_size; i++){ // loop for each fd
        int bytesReadForThisFile = 0;
        
        // first find number of bytes in file so we can write that to message so client can easily read through it
        char garbage[101];
        fileLength = 0;
        do{
            status = read(fds[i], &garbage, 100);
            //printf("%s", garbage);
            fileLength += status;
        }while(status > 0);

        if(fileLength < 0){ // file doesn't exist so just continue
            continue;
        }

        //printf("file length of file #%d: %d\n", i+1, fileLength);

        //printf("\n*******************\n");

        int k;
        char fileLengthStr[20];
        //printf("fileLength: %d\n", fileLength);
        //itoa(fileLength, fileLengthStr, 10);
        sprintf(fileLengthStr, "%d", fileLength);
        //printf("so fileLengthStr is %s\n", fileLengthStr);

        // if putting in the number of bytes brings you over the allocated space for the string then must realloc first
        if(totalNumOfBytesRead + strlen(fileLengthStr) + 1 >= allocatedBytesForString){ // realloc if necessary
            //printf("\nhave to realloc because total num of bytes read is %d and length of fileLengthStr is %d and allocated bytes is %d...currently writing in file length\n", totalNumOfBytesRead, strlen(fileLengthStr), allocatedBytesForString);
            allocatedBytesForString *= 2;
            reallocPtr = realloc(strForServer, (allocatedBytesForString + 1) * sizeof(char)); // double size of array
            if(reallocPtr == NULL){
                printf("Malloc failed when reading in files...how big are your files?...bad user\n");
                return NULL;
            }else{
                strForServer = reallocPtr;
            }
        }else{
            //printf("\ntotal num of bytes read is %d and length of fileLengthStr is %d and allocated bytes is %d so no need to realloc\n", totalNumOfBytesRead, strlen(fileLengthStr), allocatedBytesForString);
        }

        
        for(k = 0; k < strlen(fileLengthStr) + 1; k++){
            
            if(k == strlen(fileLengthStr)){ // put ':' because the number is finished
                strForServer[totalNumOfBytesRead] = ':';
                totalNumOfBytesRead++;
            }else{
                strForServer[totalNumOfBytesRead] = fileLengthStr[k];
                totalNumOfBytesRead++;
            }
        }
        
        //sprintf(strForServer, "%s%s:", strForServer, fileLengthStr);
        // done writing file length to string

        lseek(fds[i], 0, SEEK_SET); // go back to beginnning of file

        // now write actual file contents to string
        do{
            while(totalNumOfBytesRead + fileLength >= allocatedBytesForString){ // need to realloc!
                //printf("\nhave to realloc because total num of bytes read is %d and length of fileLengthStr is %d and allocated bytes is %d...currently writing in file contents\n", totalNumOfBytesRead, strlen(fileLengthStr), allocatedBytesForString);
                allocatedBytesForString *= 2;
                reallocPtr = realloc(strForServer, (allocatedBytesForString + 1) * sizeof(char)); // double size of array
                if(reallocPtr == NULL){
                    printf("Malloc failed when reading in files...how big are your files?...bad user\n");
                    return NULL;
                }else{
                    strForServer = reallocPtr;
                }
            }
            status = read(fds[i], &(strForServer[totalNumOfBytesRead]), fileLength - bytesReadForThisFile);
            //printf("read in %d bytes\n", allocatedBytesForString - (fileLength + bytesReadForThisFile));
            bytesReadForThisFile += status;
            totalNumOfBytesRead += status;
            //printf("%d\n", totalNumOfBytesRead);
        }while(status > 0);
        // done writing file contents to string

        if(i != fds_size - 1 && fds[i+1] > 0){
            strForServer[totalNumOfBytesRead] = ':';
            totalNumOfBytesRead++;
        }


    }

    strForServer[totalNumOfBytesRead] = '\0';
    //printf("\n'%s'\n", strForServer);
    return strForServer;
}

int writeToFile(int fd, char* str){ // given fd and str, will write str to fd. returns number of bytes it wrote
    int bytesToWrite = strlen(str);
    int bytesWritten = 0;
    while(bytesWritten < bytesToWrite){
        bytesWritten += write(fd, str+bytesWritten, bytesToWrite-bytesWritten);
    }
    return bytesWritten;
}

char* manifestStructToString(Manifest* manifest){ // turns given manifest struct pointer into string -> returns pointer to that string
    char* ret = (char*)malloc(200 * sizeof(char)); // start with room for 200 bytes
    char* reallocPtr = NULL;
    int bytesAllocated = 200;
    int lineLength = 0;
    int bytesWritten = sprintf(ret, "%s\n", manifest->version);
    int i;

    for(i = 0; i < manifest->numOfEntries; i++){
        //printf("version: '%s', path: '%s', hash: '%s'\n", manifest->entries[i].version, manifest->entries[i].path, manifest->entries[i].hash);

        // check if realloc is necessary
        lineLength = strlen(manifest->entries[i].version) + strlen(manifest->entries[i].path) + strlen(manifest->entries[i].version) + 3;
        while(lineLength + bytesWritten > bytesAllocated){
            bytesAllocated *= 2;
            reallocPtr = realloc(ret, bytesAllocated * sizeof(char));
            if(reallocPtr == NULL){
                printf("malloc failed for .Manifest file...how big is your input?\n");
                return NULL;
            }else{
                ret = reallocPtr;
            }
        }

        bytesWritten += sprintf(ret+bytesWritten, "%s ", manifest->entries[i].version);
        bytesWritten += sprintf(ret+bytesWritten, "%s ", manifest->entries[i].path);
        if(i < manifest->numOfEntries - 1){
            bytesWritten += sprintf(ret+bytesWritten, "%s\n", manifest->entries[i].hash);
        }else{
            bytesWritten += sprintf(ret+bytesWritten, "%s", manifest->entries[i].hash);
        }
        //ret[bytesWritten] = '\0';
        //free(manifest->entries[i]);
    }
    return ret;
}

char* fileToString(int fd){ // turns given file into string -> returns pointer to that string
    if(fd < 0){
        return NULL;
    }
    char* str = (char*)malloc(101 * sizeof(char));
    char* reallocPtr = NULL;
    int allocatedBytesForString = 100; // originally space for 100 bytes
    int numOfBytesRead = 0;
    int status = 0;
    do{
        status = read(fd, &(str[numOfBytesRead]), 100);
        numOfBytesRead += status;
        if(numOfBytesRead >= allocatedBytesForString){ // realloc if necessary
            allocatedBytesForString *= 2;
            reallocPtr = realloc(str, (allocatedBytesForString + 1) * sizeof(char)); // double size of array
            if(reallocPtr == NULL){
                printf("Malloc failed when reading in file...how big are your files?...bad user\n");
                return NULL;
            }else{
                str = reallocPtr;
            }
        }
    }while(status > 0);

    str[numOfBytesRead] = '\0';
    return str;
    
}

void freeManifestStruct(Manifest* manifest){

    //printf("version of manifest: '%s'\n", manifest->version);
    int i;
    for(i = 0; i < manifest->numOfEntries; i++){
        //printf("version: '%s', path: '%s', hash: '%s'\n", manifest->entries[i].version, manifest->entries[i].path, manifest->entries[i].hash);
        free(manifest->entries[i].version);
        free(manifest->entries[i].path);
        free(manifest->entries[i].hash);
        //free(manifest->entries[i]);
    }
    free(manifest->entries);
    free(manifest->version);
    free(manifest);
    

}

Manifest* createManifestStructFromManifestFile(int manifestfd){
    Manifest* manifest = (Manifest*)malloc(sizeof(Manifest));
    manifest->entries = (ManifestEntry*)malloc(20 * sizeof(ManifestEntry)); // size for 20 entries originally
    manifest->version = (char*)malloc(10 * sizeof(char)); // size for a 9-digit number

    char buffer = '?';
    int size = 0;
    char* reallocPtr = NULL;

    //read(manifestfd, &buffer, 1);
    //printf("%c", buffer);
    do{ // read version number of manifest file
        read(manifestfd, &buffer, 1);
        manifest->version[size] = buffer;
        size++;
        
    }while(buffer != '\n');
    manifest->version[size - 1] = '\0';
    //printf("manifest version number: '%s'\n", manifest->version);

    //int status = read(manifestfd, &buffer, 1);
    int status = 1;
    //printf("%c", buffer);
    int entryNum = 0;
    int allocatedBytesForPath = 100; // originally 100 bytes allocated for path

    while(status > 0){ // loop for whole manifest file and read components into manifest struct
        
        size = 0;
        manifest->entries[entryNum].version = (char*)malloc(10 * sizeof(char));
        //buffer = '?';

        //status = read(manifestfd, &buffer, 1);
        do{ // loop for version
            status = read(manifestfd, &buffer, 1);
            //printf("%c", buffer);
            manifest->entries[entryNum].version[size] = buffer;
            //printf("putting %c into version num for entry #%d\n", buffer, entryNum);
            size++;
            
        }while(buffer != ' ' && status > 0);

        manifest->entries[entryNum].version[size - 1] = '\0';
        //printf("version num for entry #%d is %s\n", entryNum, manifest->entries[entryNum].version);
        //buffer = '?';
        manifest->entries[entryNum].path = (char*)malloc((allocatedBytesForPath + 1) * sizeof(char));
        size = 0;
        //status = read(manifestfd, &buffer, 1);
        //printf("%c", buffer);
        do{ // loop for path

            if(size >= allocatedBytesForPath){ // realloc if necessary
                reallocPtr = realloc(manifest->entries[entryNum].path, 2 * allocatedBytesForPath * sizeof(char)); // double size of array
                allocatedBytesForPath *= 2;
                if(reallocPtr == NULL){
                    printf("Malloc failed when making .Manifest struct...how big is your .Manifest?...bad user\n");
                    return NULL;
                }else{
                    manifest->entries[entryNum].path = reallocPtr;
                }
            }

            status = read(manifestfd, &buffer, 1);
            manifest->entries[entryNum].path[size] = buffer;
            size++;
            
            //printf("%c", buffer);

        }while(buffer != ' ' && status > 0);
        manifest->entries[entryNum].path[size - 1] = '\0';
        //buffer = '?';
        size = 0;
        manifest->entries[entryNum].hash = (char*)malloc(33 * sizeof(char)); // hash has 32 bytes
        //status = read(manifestfd, &buffer, 1);
        //printf("%c", buffer);
        do{ // loop for hash
            status = read(manifestfd, &buffer, 1);
            manifest->entries[entryNum].hash[size] = buffer;
            size++;
            
            //printf("%c", buffer);
        }while(buffer != '\n' && status > 0);
        manifest->entries[entryNum].hash[size - 1] = '\0';
        //printf("the hash for entry #%d is %s\n", entryNum+1, manifest->entries[entryNum].hash);
        entryNum++;

        // check if at end of file here, because apparently C is a petty little b*tch with that
        status = read(manifestfd, &buffer, 1);
        if(status < 1){
            break;
        }else{
            lseek(manifestfd, -2, SEEK_CUR);
        }
        status = read(manifestfd, &buffer, 1);
    }
    manifest->numOfEntries = entryNum;

    lseek(manifestfd, 0, SEEK_SET);

    return manifest;
    

}


int readInIntegerBeforeBytes(int sockfd){ // returns an integer which will be the number of bytes to read next. will return -1 if it is not a number being read in
    //printf("got to readInInteger function\n");
    char* intString = (char*)malloc(20 * sizeof(char));
    char* reallocPtr = NULL;
    int stringSize = 20;
    int numOfBytesRead = -1;
    char buffer = '?';
    int count = 0;

    while(buffer != ':'){
        //printf("buffer: %c\n", buffer);
        numOfBytesRead += read(sockfd, &buffer, 1);
        if(buffer == '!' && count == 0){
            return -1;
        }
        intString[numOfBytesRead] = buffer;
        if(numOfBytesRead == stringSize - 1){
            reallocPtr = realloc(intString, 2 * stringSize * sizeof(char)); // double size of array
            stringSize *= 2;
            if(reallocPtr == NULL){
                printf("Malloc failed...how big is your input?...bad user\n");
                    return 0;
            }else{
                intString = reallocPtr;
            }
        }
        count++;
    }
    intString[numOfBytesRead] = '\0';
    //printf("intString before conversion to int: '%s'\n", intString);
    int ret = atoi(intString);
    free(intString);
    return ret;
}

char* readInBytes(int sockfd, int numOfBytesToRead){ // returns a pointer to a string that contains the bytes that the server read in

    char* stringOfBytes = (char*)malloc((numOfBytesToRead + 1) * sizeof(char));
    char* reallocPtr = NULL;
    //int stringSize = 100;
    int numOfBytesRead = 0;
    int status = 0;
    do{
        numOfBytesRead += read(sockfd, &(stringOfBytes[numOfBytesRead]), numOfBytesToRead);
        //(*numOfBytes) += status; // keep track of how many bytes you read in
    }while(numOfBytesToRead > numOfBytesRead);

    stringOfBytes[numOfBytesToRead] = '\0';
    return stringOfBytes;
}

void error(char *msg)
{
    perror(msg);
    exit(0);
}
void SendToServer(int socketFD,char* message){
    int n;
    n =write(socketFD, message, strlen(message));
    if(n<0){
        error("ERROR writing to socket\n");
        return;
    }
    //printf("Client has written '%s' to server\n", message);
    
    /*
    //sleep(5);
    char buffer[256];
    n = read(socketFD,buffer,255);
    if(n<0){
        error("ERROR reading from scoket");
    }
    */
    //printf("Server says: function '%s' received\n",buffer);
}
int connectToServer(char* host,char* port)
{
  int portno = atoi(port);
  //printf("%s",host);
  //printf("In thread\n");
  char message[1000];
  char buffer[1024];
  int clientSocket;
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
  // Create the socket. 
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  //Configure settings of the server address
 // Address family is Internet 
  serverAddr.sin_family = AF_INET;
  //Set port number, using htons function 
  serverAddr.sin_port = htons(portno);
 //Set IP address to localhost
  serverAddr.sin_addr.s_addr = inet_addr(host);
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    //Connect the socket to the server using the address
    addr_size = sizeof serverAddr;
    while(connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size)<0){
        error("error connecting to server...will try again in 3 seconds.\n");
        sleep(3);
    }
    printf("connected to %s\n", host);
    //printf("THIS IS SOCKETFD IN FUNCTION:%d/n",clientSocket);
    return clientSocket;
    /*strcpy(message,"Hello");
   if( send(clientSocket , message , strlen(message) , 0) < 0)
    {
            printf("Send failed\n");
    }
    //Read the message from the server into the buffer
    if(recv(clientSocket, buffer, 1024, 0) < 0)
    {
       printf("Receive failed\n");
    }
    //Print the received message
    printf("Data received: %s\n",buffer);
    close(clientSocket);
    pthread_exit(NULL);*/
}

char *str2md5(const char *str, int length) {//This turns a string into hash
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    return out;
}

int isProjectInDirectory(char* directoryName, char* projectName){ // searches given directory for given project name.
                                                                //returns -1 if directory isnt found, 0 if project is not found, 1 if project is found
    struct dirent *pDirent;
    DIR *pDir;
    pDir = opendir(directoryName);
    if(pDir == NULL){ // directory doesn't exist
        return -1;
    }
    while((pDirent = readdir(pDir))!= NULL){
        if( strcmp(pDirent->d_name, projectName) == 0 ){ // project name is a match to an entry in the directory -> project found
            return 1;
        }
    }

    return 0; // no matches in directory -> project not found
        
}

int checkout(int sockfd, char* projectName){ // will return 0 on fail, 1 on success

    if(isProjectInDirectory("./", projectName) == 1){ // project already exists on client side, so checkout fails
        printf("project already exists on client - checkout failed\n");
        return 0;
    }

    write(sockfd, "12:hi whats up?", 17);

    /* PSEUDO CODE - FILL THIS PART IN
    if( project does not exist on server ){
        printf("project does not exist on server");
        return 0;
    }
    */

}

int main(int argc, char* argv[]){ //************************* M A I N ********************************************************************************************************************************

    if(strcmp(argv[1],"configure") == 0 ){ // configure command called
    int configure = open("./.configure", O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 00644);
    //write(configure,"host: ",6);
    int length = strlen(argv[2]);
    write(configure,argv[2],length);
    write(configure,"\n",1);
    //write(configure,"port: ",6);
    length = strlen(argv[3]);
    write(configure,argv[3],length);
    return 1;
    //write(configure,"\n",1);
        //if unsuccessful
        //return 0;

        //if successful
        //return 1;
    //printf("hell\n");
    }else if( strcmp(argv[1], "add") == 0 ){ // add command called --- check if the command is add or remove before the others because these two don't require a connection to the server
    //we must open up project directory and add a file there based on the name
    //first we must check if the directory project or whatever exists within client directory. If it doesnt exist create the directory named that then go into it and create the text file.
    struct dirent *pDirent;
    DIR *pDir;
     pDir = opendir("./");
    if(pDir == NULL){
        printf("Directory doesnt exist \n");
        return 1;
    }
    int truth = 0;
    while((pDirent = readdir(pDir))!= NULL){
        //ls
        printf("%s",pDirent->d_name);
        //printf("\n");
        if(strcmp(pDirent->d_name,argv[2])==0){
            truth = 1;
            break;
        }
        else{
            truth = 0;
        }
    }//5d41402abc4b2a76b9719d911017c592
    char str[2000] = "./";
    char directory[2000];
    strcat(str,argv[2]);
    strcat(str,"/");
    strcpy(directory,str);//this is ./client1/projectname/
    strcat(str,argv[3]);//this is ./client1/projectname/file
    printf("This is the location for directory:%s\n",directory);
    printf("This is the location:%s\n",str);
    if(truth ==1){//go into the directory go into manifest and add the txt to it after getting the hash information.
       // int configure = open(str, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 00644);
        int configure = open(str, O_RDONLY | O_NONBLOCK);//we open up the file here
        //creating hash right now.
        /*unsigned char c[MD5_DIGEST_LENGTH];
        int i;
        MD5_CTX mdContext;
        int bytes;
        unsigned char data[1024];
         MD5_Init (&mdContext);*/
         char* fileAsStr = (char*)malloc(100 * sizeof(char));
            char* reallocPtr;
            //int* reallocIntPtr;
            //char** reallocPtr2;
            int fileSize = 0;
            int status = 0;
            do{
                status = read(configure, &(fileAsStr[fileSize]), 100);
                fileSize += status; // keep track of how many bytes you read in
                if(status == 100){
                    reallocPtr = realloc(fileAsStr, 2 * fileSize * sizeof(char)); // double size of array
                    if(reallocPtr == NULL){
                        printf("Malloc failed...how big is your input?...bad user\n");
                        return 0;
                    }else{
                        fileAsStr = reallocPtr;
                    }
                } //yes
            }while(status > 0);
            fileAsStr[fileSize] = '\0';
            printf("%s\n", fileAsStr);//this is whats in the text file and its hash is below
            char *output = str2md5(fileAsStr, strlen(fileAsStr));//**** means completed
            //We must get the hashcode of the text thats being added first ****//we now have the hash
            printf("%s\n", output);
            //free(output);
            close(configure);
            struct Node newnode;
            newnode.version = "0";
            newnode.hashcode = output;
            newnode.pathfile = str;
            strcat(directory,".Manifest");
            printf("%s\n",directory);
            int manifest = open(directory, O_RDWR | O_APPEND | O_NONBLOCK,00644);
            char* fileAsStr1 = (char*)malloc(100 * sizeof(char));
            char* reallocPtr1;
            //int* reallocIntPtr;
            //char** reallocPtr2;
            int fileSize1 = 0;
            int status1 = 0;
            //if its a newline clear out everything and keep going
            //we need a buffer and we need 3 characters that can hold enough space for everything, the pathfile length we know, 
            do{
                status1 = read(manifest, &(fileAsStr1[fileSize1]), 100);
                fileSize1 += status1; // keep track of how many bytes you read in
                if(status1 == 100){
                    reallocPtr1 = realloc(fileAsStr1, 2 * fileSize1 * sizeof(char)); // double size of array
                    if(reallocPtr1 == NULL){
                        printf("Malloc failed...how big is your input?...bad user\n");
                        return 0;
                    }else{
                        fileAsStr1 = reallocPtr1;
                    }
                } //yes
            }while(status1 > 0);
            fileAsStr1[fileSize1] = '\0';
            close(manifest);
            //printf("%c\n", fileAsStr1[1000000000]);
           // printf("\n");
            int i = 0;
            //we know length of hash is 33.
            //we know length of project 
            char* path = (char*)malloc(strlen(str) * sizeof(char));
            char* hash = (char*)malloc(strlen(output)*sizeof(char));
            int update = 0;
            int line = 0;
            int length = 0;
            int found = 0;
            int properline = 0;
            int finalline = 0;
            while(fileAsStr1[i]!='\0'){
                if(fileAsStr1[i]== '\n'){
                    line++;
                    printf("Line:%d\n",line);
                    //we are currently at version right now
                    int j = i+1;
                    while(fileAsStr1[j]!= ' '){//calculating length of version number
                        length++;
                        j++;
                    } 
                char* ver = (char*)malloc(length*sizeof(char));
                //nt k = i+1;
                int m = 0;
                i++;//skipping the NEWLINE char.
                while(fileAsStr1[i]!=' '){
                    ver[m] = fileAsStr1[i];
                    i++;
                    m++;
                }
                //we now have version number
                ver[length] = '\0';
                i++;//skipping past space
                //now get the file path
                int k = 0;
                while(k < strlen(str)){
                    path[k] = fileAsStr1[i];
                    k++;
                    i++;
                }
                path[strlen(str)] = '\0';
                int l = i;
                if(strcmp(path,str)==0){
                    l++;//to skip the space after finding the path
                    found = 1;
                    int k = 0;
                    while(k<strlen(output)){
                        hash[k] = fileAsStr1[l];
                        k++;
                        l++;
                    }
                    hash[strlen(output)]='\0';
                    printf("This is the hash of the matching PATH:%s\n",hash);
                    if(strcmp(hash,output)==0){//hashs are the same no changes have to be made to the .manifest
                        break;
                    }
                    else{//hashes are not the same, update integer to 1 so we know to change it.
                         newnode.version = ver;
                         //newnode.hashcode = hash;
                         int newversion = atoi(newnode.version);
                         newversion = newversion+1;
                         length = length+1;
                         char* ver2 = (char*)malloc(length*sizeof(char));
                         sprintf(ver2, "%d", newversion);
                         newnode.version = ver2;
                         update = 1; //this means we will update the string 
                         properline = line;
                         printf("IT IS AT LINE:%d\n",properline);
                    }
                }
               // printf("string length of str:%d\n",strlen(str));
                printf("This is version number:%s\n",ver);
                printf("This is PATH:%s\n",path);
                }
                //printf("%c",fileAsStr1[i]);
                i++;
            }
            finalline = line;
            if(update == 1){//update the text file
            //int newversion = atoi(newnode.version);
           // newversion = newversion + 1;
            int newsize = strlen(fileAsStr1) + 1;
            char* newManifest = (char*)malloc(newsize * sizeof(char));
            int i = 0;
            int line = 0;
            int j = 0;
            newManifest[0] = fileAsStr1[0];
            while(fileAsStr1[i]!='\0'){
                if(fileAsStr1[i]=='\n'){//we are at line 1 rn
                    line++;
                    newManifest[i]=fileAsStr1[i];
                    i++;//skipping newline
                    if(properline == line){
                        //copy everything we have in newnode to this line
                        int counter = 0;
                        while(newnode.version[counter]!='\0'){
                            newManifest[i] = newnode.version[counter];
                            counter++;
                            i++;
                        }
                        newManifest[i]=' ';
                        i++;//skipped over space
                        counter = 0;
                        while(newnode.pathfile[counter]!='\0'){
                            newManifest[i] = newnode.pathfile[counter];
                            counter++;
                            i++;
                        }
                        newManifest[i]=' ';
                        i++;
                        counter = 0;
                        while(newnode.hashcode[counter]!='\0'){
                            newManifest[i] = newnode.hashcode[counter];
                            counter++;
                            i++;
                        }
                    }
                }
            newManifest[i] = fileAsStr1[i];
            i++;
            }
            newManifest[newsize] = '\0';
            printf("THIS IS THE UPDATED MANIFEFST:\n%s\n",newManifest);
            //Now write this to the .Manifest file
            int manifest = open(directory, O_RDWR );
            write(manifest,newManifest,strlen(newManifest));
            }

            else if(found == 0){//create space for a new one with version number+two spaces+sizeofpath+a new line+size of hash
            //allocate space of the old file+version number+two spaces+newline+sizeofpath+hash.
           // printf("This is size of the path: %d\n",strlen(fileAsStr));
            printf("This is size of the path: %d\n",strlen(str));
            printf("This is size of the hash: %d\n",strlen(newnode.hashcode));
            printf("This is size of the version: %d\n",strlen(newnode.version));
            printf("This is size of the old manifest: %d\n",strlen(fileAsStr1));
            int fulllength = strlen(fileAsStr1);//this is path length
            fulllength = fulllength + strlen(newnode.version) + strlen(newnode.hashcode);
            fulllength += strlen(str);
            //now add the length of two spaces and a newline and the old manifest
            fulllength += 3;//ading two spaces and a newline
            printf("This will be the size of the new manifest:%d\n",fulllength);
            printf("This is the final line where we will add all the info:%d\n",finalline);
            char* newManifest = (char*)malloc(fulllength * sizeof(char));
            //old manifest is in fileAsStr1
            int i = 0;
            newManifest[0] = fileAsStr1[0];//we added the first number at the top
            int line = 0;
            while(fileAsStr1[i] != '\0'){
                newManifest[i] = fileAsStr1[i];
                i++;
                }
                newManifest[i]='\n';
                i++;
            int k = 0;
            while(newnode.version[k]!='\0'){
                newManifest[i]=newnode.version[k];
                k++;
                i++;
            }
            newManifest[i]=' ';
            i++;
            k = 0;
            while(newnode.pathfile[k]!='\0'){
                newManifest[i]=newnode.pathfile[k];
                k++;
                i++;
            }
            newManifest[i] = ' ';
            i++;
            k = 0;
             while(newnode.hashcode[k]!='\0'){
                newManifest[i]=newnode.hashcode[k];
                k++;
                i++;
            }
            newManifest[i]='\0';
            printf("THIS IS THE NEW MANIFEST:\n%s\n",newManifest);
            int manifest = open(directory, O_RDWR );
            write(manifest,newManifest,strlen(newManifest));

                //just start adding
            }
            //now we open up the .Manifest file and add all this to it.
            //After that we create a node that has the path of the file,the hashcode, the version which at the moment will be 0 ****
            //first check the .Manifest file if the pathname are the same. If the pathname are the same check if the hash is the same if its the same abort if theyre different then change the hash and increase the version by 1.
            //we would have to remove the whole line in this case and put everyhting again. 
            //If its a completely new file then just find the end, make a new line and insert everything. 
            
    }
    else if(truth == 0){//directory aint here
    
    }



    }else if( strcmp(argv[1], "remove") == 0 ){ // remove command called
    struct dirent *pDirent;
    DIR *pDir;
     pDir = opendir("./");
    if(pDir == NULL){
        printf("Directory doesnt exist \n");
        return 1;
    }
    int truth = 0;
    while((pDirent = readdir(pDir))!= NULL){
        if(strcmp(pDirent->d_name,argv[2])==0){
            truth = 1;
            break;
        }
        else{
            truth = 0;
        }
    }
    char str[2000] = "./";
    char directory[2000];
    strcat(str,argv[2]);
    strcat(str,"/");
    strcpy(directory,str);//this is ./client1/projectname/
    strcat(str,argv[3]);//this is ./client1/projectname/file
    printf("This is the location for directory:%s\n",directory);
    printf("This is the location:%s\n",str);
    if(truth ==1){//go into the directory go into manifest and add the txt to it after getting the hash information.
       // int configure = open(str, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 00644);
        int configure = open(str, O_RDONLY | O_NONBLOCK);//we open up the file here
            struct Node newnode;
            //we just need pathfile to remove.
            newnode.pathfile = str;
            strcat(directory,".Manifest");
            printf("%s\n",directory);//this is .Manifest entrance rn
            int manifest = open(directory, O_RDWR | O_APPEND | O_NONBLOCK,00644);
            char* fileAsStr1 = (char*)malloc(100 * sizeof(char));
            char* reallocPtr1;
            //int* reallocIntPtr;
            //char** reallocPtr2;
            int fileSize1 = 0;
            int status1 = 0;
            //if its a newline clear out everything and keep going
            //we need a buffer and we need 3 characters that can hold enough space for everything, the pathfile length we know, 
            do{
                status1 = read(manifest, &(fileAsStr1[fileSize1]), 100);
                fileSize1 += status1; // keep track of how many bytes you read in
                if(status1 == 100){
                    reallocPtr1 = realloc(fileAsStr1, 2 * fileSize1 * sizeof(char)); // double size of array
                    if(reallocPtr1 == NULL){
                        printf("Malloc failed...how big is your input?...bad user\n");
                        return 0;
                    }else{
                        fileAsStr1 = reallocPtr1;
                    }
                } //yes
            }while(status1 > 0);
            fileAsStr1[fileSize1] = '\0';
            close(manifest);//WE HAVE THE MANIFEST IN AN ARRAY NOW
            //printf("%c\n", fileAsStr1[1000000000]);
           // printf("\n");
            int i = 0;
            //we know length of hash is 33.
            //we know length of project 
            char* path = (char*)malloc(strlen(str) * sizeof(char));
            char* hash = (char*)malloc(32*sizeof(char));
            int update = 0;
            int line = 0;
            int length = 0;
            int found = 0;
            int properline = 0;
            int finalline = 0;
            int removeline = 0;
            while(fileAsStr1[i]!='\0'){
                if(fileAsStr1[i]== '\n'){
                    line++;
                    printf("Line:%d\n",line);
                    //we are currently at version right now
                    int j = i+1;
                    while(fileAsStr1[j]!= ' '){//calculating length of version number
                        length++;
                        j++;
                    } 
                char* ver = (char*)malloc(length*sizeof(char));
                //nt k = i+1;
                int m = 0;
                i++;//skipping the NEWLINE char.
                while(fileAsStr1[i]!=' '){
                    ver[m] = fileAsStr1[i];
                    i++;
                    m++;
                }
                //we now have version number
                ver[length] = '\0';
                i++;//skipping past space
                //now get the file path
                int k = 0;
                while(k < strlen(str)){
                    path[k] = fileAsStr1[i];
                    k++;
                    i++;
                }
                path[strlen(str)] = '\0';
                int l = i;
                if(strcmp(path,str)==0){
                    l++;//to skip the space after finding the path
                    found = 1;//THIS MEANS THE STRING EXISTS AT FOUND AND WHICH LINE IS IT?
                    removeline = line;
                    int k = 0;
                    while(k<32){
                        hash[k] = fileAsStr1[l];
                        k++;
                        l++;
                    }
                    hash[32]='\0';//size of hash is 32
                    printf("This is the hash of the matching PATH:%s\n",hash);
                    //hashes are not the same, update integer to 1 so we know to change it.
                        newnode.hashcode = hash;
                         newnode.version = ver;
                         //newnode.hashcode = hash;
                         //int newversion = atoi(newnode.version);
                        // newversion = newversion+1;
                         //length = length+1;
                         //char* ver2 = (char*)malloc(length*sizeof(char));
                         //sprintf(ver2, "%d", newversion);
                        // newnode.version = ver2;
                         //update = 1; //this means we will update the string 
                         //properline = line;
                         //printf("IT IS AT LINE:%d\n",properline);
                    
                }
               // printf("string length of str:%d\n",strlen(str));
                printf("This is version number:%s\n",ver);
                printf("This is PATH:%s\n",path);
                }
                //printf("%c",fileAsStr1[i]);
                i++;
            }
            finalline = line;
            if(found == 1){//update the text file
            //int newversion = atoi(newnode.version);
           // newversion = newversion + 1;
            
            int i = 0;
            int line = 0;
            int j = 0;
            //calculate size of line that has to be removed.
            int removal = 0;
            removal+=3;//two spaces,newline
            removal+=strlen(newnode.version);//version
            removal+=strlen(str);//path
            removal+=strlen(newnode.hashcode);//size of hash 
            printf("THIS IS HOW MUCH HAS TO BE REMOVED FROM OLD MANIFEST SIZE:%d\n",removal);
            int newsize = strlen(fileAsStr1) - removal;
            char* newManifest = (char*)malloc(newsize * sizeof(char));
            printf("THIS IS NEW SIZE OF NEW MANIFEST:%d\n",newsize);
            printf("THIS IS OLD SIZE OF OLD MANIFEST:%d\n",strlen(fileAsStr1));
            printf("THIS IS THE LINE BEING REMOVED:%d\n",removeline);
            //removal is how much we are skipping.
            int p = 0;
            //newManifest[0] = fileAsStr1[0];
            //int j = 0;
            newManifest[0] = fileAsStr1[0];
            i++;
            j++;
            while(fileAsStr1[i]!='\0'){
                if(fileAsStr1[i]=='\n'){
                    line++;
                    newManifest[j] = fileAsStr1[i];
                    j++;
                    i++;
                    if(removeline == line){
                        while(p!=removal){
                            p++;
                            i++;
                        }    

                    }
                }


                newManifest[j]=fileAsStr1[i];
                i++;
                j++;
            }
            newManifest[newsize] = '\0';
            //int fd=open(directory,O_RDONLY | O_WRONLY | O_TRUNC);
            //close(fd);
            printf("THIS IS THE UPDATED MANIFEFST:\n%s\n",newManifest);
            printf("THIS IS THE OLD MANIFEFST:\n%s\n",fileAsStr1);
            //Now write this to the .Manifest file
            int manifest = open(directory,O_RDONLY | O_WRONLY | O_TRUNC);
            write(manifest,newManifest,strlen(newManifest));
            }

   
    }else if(truth == 0){//directory aint here
        printf("Fuck off bud, put in a proper directory\n");
    }
    }// if something other than configure/add/remove is called then we must open ./.configure so we can connect to server
    
//                  SETTING UP CONNECTION HERE AND GATHERING HOST AND PORT
        int configureFD = open("./.configure", O_RDONLY);
        //printf("its open\n");
        if(configureFD < 0){ // ./.configure file not found
            printf("server information not yet configured... exiting\n");
            return 0;
        }

        char* host = (char*)malloc(256 * sizeof(char)); // max length of a hostname is 255 bytes
        char* port = (char*)malloc(10 * sizeof(char)); // port should be from 5000 to 65536 but malloc extra space in case user is silly
        char buffer = '?';
        int i = 0;

        // store hostname/IP in host
        read(configureFD, &buffer, 1);
        //printf("This is buffer currently:%c\n",buffer);
        while(buffer != '\n'){
            
            host[i] = buffer;
            i++;
            read(configureFD, &buffer, 1);
            
        }
        //printf("This is buffer currently:%c\n",buffer);
        host[i] = '\0';
        //printf("host:%s\n",host);
        // hostname/IP stored


        // store port number in port
        i = 0;
        int status = read(configureFD, &buffer, 1);
        //read(configureFD, &buffer, 1);
        
        while(status > 0){ // store hostname/IP in host
            //printf("This is buffer currently:%c\n",buffer);
            port[i] = buffer;
            i++;
            status = read(configureFD, &buffer, 1);
        }

        port[i] = '\0';
       // printf("port:%s\n",port);
        // port number stored

        close(configureFD); // close ./.configure file descriptor

        printf("host: '%s', port: '%s'\n", host, port);

    if( strcmp(argv[1], "checkout") == 0){ // checkout command called ################################################# C H E C K O U T #############################################################


        if(isProjectInDirectory("./", argv[2]) == 1){ // project already exists on client side, so checkout fails
            printf("project already exists on client - checkout failed\n");
            return 0;
        }
        int socketFD = connectToServer(host,port);
        printf("sending checkout command to server\n");
        SendToServer(socketFD,argv[1]);
        sleep(1);
        //char projectNameLengthStr[(strlen(argv[2]) / 10) + 2];
        char projectNameLengthStr[22];
        sprintf(projectNameLengthStr, "%d:", strlen(argv[2]));
        SendToServer(socketFD, projectNameLengthStr);
        SendToServer(socketFD, argv[2]);

        int numOfBytesToRead = 0;
        //printf("about to enter loop to wait for server's response\n");
        do{// loop until the client writes to server -- first thing written will be the number of bytes to read in -> will be .Manifest file first
            numOfBytesToRead = readInIntegerBeforeBytes(socketFD);
        }while(numOfBytesToRead < 1); // will break out of loop once number of bytes to read is obtained

        char* manifestStr = readInBytes(socketFD, numOfBytesToRead); // Manifest file now saved as string
        char dirName[3 + strlen(argv[2])];
        sprintf(dirName, "./%s", argv[2]);
        int dirSuccess = mkdir(dirName, 0777); // make new directory for project
        char manifestPath[strlen(dirName) + 11]; // "./<project name>/.Manifest\0"
        sprintf(manifestPath, "%s/.Manifest", dirName);
        int manfd = open(manifestPath, O_CREAT | O_RDWR | O_TRUNC, 00644); // create .Manifest file
        int bytesWritten = 0;
        do{
            bytesWritten += write(manfd, manifestStr+bytesWritten, strlen(manifestStr)); // write manifest string to new .Manifest file
        }while(bytesWritten < strlen(manifestStr));

        free(manifestStr);

        // .Manifest file created, so now create the remaining files
        // start by checking the paths in .Manifest to see if any subdirectories need to be made *****
        lseek(manfd, 0, SEEK_SET);
        Manifest* manifest = createManifestStructFromManifestFile(manfd);
        close(manfd);
        i = 0;
        for(i = 0; i < manifest->numOfEntries; i++){

            int numOfSlashes = 0;
            char pathToSubdirectories[strlen(manifest->entries[i].path)]; // make string with enough room for however many subdirectories there may be
            int k;
            for(k = 0; k < strlen(manifest->entries[i].path); k++){

                if(manifest->entries[i].path[k] == '/'){ // if this is true then that means a subdirectory might need to be made
                    numOfSlashes++;
                    if(numOfSlashes > 2){ // subdirectory needs to be made
                        pathToSubdirectories[k] = '\0';
                        //printf("creating subdirectory with path: '%s'\n", pathToSubdirectories);
                        mkdir(pathToSubdirectories, 0777);
                    }
                    //indexOfPreviousSlash = k;
                }
                pathToSubdirectories[k] = manifest->entries[i].path[k];
                //printf("put %c in path to subdirectories\n", manifest->entries[i].path[k]);
            }
        }
        // ALL SUBDIRECTORIES MADE

        // NOW, CREATE FILES
        char garbage;
        status = read(socketFD, &garbage, 1); // try to read in the colon that is found between files in our protocol...if there isn't one, that means you are at end of all files and are finished
        //printf("'test contents: %s\n", test);
        int manifestEntry = 0;
        while(status > 0 && manifestEntry < manifest->numOfEntries){
            numOfBytesToRead = readInIntegerBeforeBytes(socketFD);
            //printf("num of bytes to read: %d\n", numOfBytesToRead);
            char* fileStr = readInBytes(socketFD, numOfBytesToRead);
            //printf("file string: %s\n", fileStr);
            int fd = open(manifest->entries[manifestEntry].path, O_RDWR | O_CREAT | O_TRUNC, 00644);
            write(fd, fileStr, strlen(fileStr));
            close(fd);
            free(fileStr);
            status = read(socketFD, &garbage, 1);
            manifestEntry++;
        }

        freeManifestStruct(manifest);

        printf("checkout successful!\n");
        printf("disconnecting from server\n");
        close(socketFD);
        return 1;
        
    }else if( strcmp(argv[1], "update") == 0 ){ // update command called ******************************************************* U P D A T E *******************************************************

        int socketFD = connectToServer(host,port);
        printf("sending update command to server\n");
        SendToServer(socketFD,argv[1]);
        sleep(1);
        //char projectNameLengthStr[(strlen(argv[2]) / 10) + 2];
        char projectNameLengthStr[22];
        sprintf(projectNameLengthStr, "%d:", strlen(argv[2]));
        SendToServer(socketFD, projectNameLengthStr);
        SendToServer(socketFD, argv[2]);

        int numOfBytesToRead = -2;
        //printf("about to read in bytes\n");
        //printf("about to enter loop to wait for server's response\n");
        do{// loop until the client writes to server -- first thing written will be the number of bytes to read in -> will be .Manifest file first
            numOfBytesToRead = readInIntegerBeforeBytes(socketFD);
        }while(numOfBytesToRead < -1); // will break out of loop once number of bytes to read is obtained

        //printf("read in bytes\n");

        if(numOfBytesToRead == -1){ // this means server sent a '!' which means the project was not found

            printf("server could not find this project. update failed.\n");
            return 0;
        }

        char* manifestStr = readInBytes(socketFD, numOfBytesToRead); // Manifest file now saved as string
        //printf("server's manifest: '%s'\n", manifestStr);
        // write server's manifest file to a file on client temporarily so we can use createManifestStructFromManifestFile function on it
        int serverManfd = open("./tempServerManifest", O_CREAT | O_RDWR | O_TRUNC, 00644);
        int bytesWritten = 0;
        do{
            bytesWritten += write(serverManfd, manifestStr+bytesWritten, strlen(manifestStr)); // write manifest string to new .Manifest file
        }while(bytesWritten < strlen(manifestStr));

        lseek(serverManfd, 0, SEEK_SET);
        Manifest* serverMan = createManifestStructFromManifestFile(serverManfd); // create struct of server manifest
        free(manifestStr);
        close(serverManfd);
        remove("./tempServerManifest");
        //printf("server's path of first entry: %s\n", serverMan->entries[1].path);

        // create path to open client's manifest file
        char dirName[3 + strlen(argv[2])];
        sprintf(dirName, "./%s", argv[2]);
        int dirSuccess = mkdir(dirName, 0777); // make new directory for project
        char manifestPath[strlen(dirName) + 11]; // "./<project name>/.Manifest\0"
        sprintf(manifestPath, "%s/.Manifest", dirName);
        int clientManfd = open(manifestPath, O_RDONLY, 00644); // open client's .Manifest file
        //
        Manifest* clientMan = createManifestStructFromManifestFile(clientManfd); // create struct of client manifest
        close(clientManfd);
        //printf("client's path of first entry: %s\n", clientMan->entries[1].path);

        // ok, at this point we have a struct of both the client's manifest and the server's manifest, so we can easily compare the two - which is what we do now #################

        // create path to .Update file
        char updatePath[strlen(dirName) + 8];
        sprintf(updatePath, "%s/.Update", dirName);

        // create path to .Conflict
        char conflictPath[strlen(dirName) + 10];
        sprintf(conflictPath, "%s/.Conflict", dirName);

        //printf("server's version num: %s and clients version num: %s\n", serverMan->version, clientMan->version);

        // CASE 1: SUCCESS - the version numbers of the two manifests are the same
        if(strcmp(clientMan->version, serverMan->version) == 0){
            
            // create blank .Update file
            open(updatePath, O_CREAT | O_TRUNC | O_RDWR, 00644);
            remove(conflictPath);
            printf("Up To Date\n");
            return 1;
        }

        // PARTIAL SUCCESS CASES AND FAILURE CASE
        
        // check all of client manifest's files against server manifest's files / open .Update file in case you need to write to it
        int i;
        int k;
        // remove existing .Update file if it exists
        remove(updatePath);
        remove(conflictPath);
        int updatefd = open(updatePath, O_RDWR | O_CREAT | O_APPEND, 00644);
        int conflictfd = open(conflictPath, O_RDWR | O_CREAT | O_APPEND, 00644);
        int usedConflict = 0; // keep track of if you write to .Conflict because if not, remove the file at end
        char updateMsg[4032];
        int extraServerManFiles[serverMan->numOfEntries]; // use to keep track of any server manifest files that aren't matched with files from client manifest
        for(i = 0; i < serverMan->numOfEntries; i++){ // fill all entries with 0 at first, then 1 as you match with them
            extraServerManFiles[i] = 0;
        }
        //int firstModificationOfUpdate = 1;
        for(i = 0; i < clientMan->numOfEntries; i++){ // loop through all client manifest files
            int foundFile = 0;
            // convert current file to string then md5 hash
            char* temp = fileToString(open(clientMan->entries[i].path, O_RDONLY));
            //printf("current file contents: '%s'\n", temp);
            char* currentFileMD5 = str2md5(temp, strlen(temp));
            //printf("file: %s, hash: '%s'\n", clientMan->entries[i].path, currentFileMD5);
            for(k = 0; k < serverMan->numOfEntries; k++){ // loop through all server manifest files
                if(strcmp(clientMan->entries[i].path, serverMan->entries[k].path) == 0){ // found file match
                    foundFile = 1;
                    extraServerManFiles[k] = 1;
                    if(strcmp(clientMan->entries[i].version, serverMan->entries[k].version) != 0 && strcmp(clientMan->entries[i].hash, serverMan->entries[k].hash) != 0){ // matching files have
                                                                                                                                                    // different versions and hashes in the manifest
                        //printf("inside first if statement... i = %d, file: %s %s\ncurrent file hash: %s, server stored hash: %s\n", i, clientMan->entries[i].path, serverMan->entries[k].path, currentFileMD5, serverMan->entries[k].hash);
                        //printf("test420 hash: %s\n", str2md5(fileToString(open(clientMan->entries[4].path, O_RDONLY, 00644)), strlen(clientMan->entries[4].path)));
                        if(strcmp(currentFileMD5, clientMan->entries[i].hash) == 0){ // live hash of client file matches what is on client manifest
                            //printf("inside second if statement\n");
                            //printf("#########GOT TO HASH MATCH#########\n");
                            // **** increment file version number - not sure where
                            // append to .Update
                            sprintf(updateMsg, "M %s %s\n", clientMan->entries[i].path, serverMan->entries[k].hash);
                            int bytesToWrite = strlen(updateMsg);
                            int bytesWritten = 0;
                            while(bytesWritten < bytesToWrite){
                                bytesWritten += write(updatefd, updateMsg, strlen(updateMsg));
                            }
                            /*
                            while(bytesWritten < bytesToWrite){
                                bytesWritten += write(updatefd, &(clientMan->entries[i].path[bytesWritten]), bytesToWrite); // write in path
                            }
                            int hashBytesWritten = 0;
                            while(hashBytesWritten < 32){
                                hashBytesWritten += write(updatefd, &(serverMan->entries[i].hash[bytesWritten]), 32); // write in hash
                                bytesWritten += hashBytesWritten;
                            }
                            */
                            printf("M %s\n", clientMan->entries[i].path);

                        }else{ // live hash of file does not match what is on client manifest -- FAILURE CASE
                            sprintf(updateMsg, "C %s %s\n", clientMan->entries[i].path, currentFileMD5);
                            int bytesToWrite = strlen(updateMsg);
                            int bytesWritten = 0;
                            while(bytesWritten < bytesToWrite){
                                bytesWritten += write(conflictfd, updateMsg, strlen(updateMsg));
                            }
                            printf("C %s\n", clientMan->entries[i].path);
                            usedConflict = 1;

                        }
                    }
                }
            }
            if(!foundFile){ // file not found on server...this means client has file that server does not
                // append to .Update and print to stdout
                //printf("#########GOT TO !FOUNDFILE#########\n");
                sprintf(updateMsg, "D %s %s\n", clientMan->entries[i].path, clientMan->entries[i].hash);
                int bytesToWrite = strlen(updateMsg);
                int bytesWritten = 0;
                while(bytesWritten < bytesToWrite){
                    bytesWritten += write(updatefd, updateMsg, strlen(updateMsg));
                }
                //printf("update msg: %s. of this, wrote %d bytes\n", updateMsg, write(updatefd, updateMsg, strlen(updateMsg)));
                /*
                while(bytesWritten < bytesToWrite){
                    bytesWritten += write(updatefd, &(clientMan->entries[i].path[bytesWritten]), bytesToWrite); // write in path
                }
                int hashBytesWritten = 0;
                while(hashBytesWritten < 32){
                    hashBytesWritten += write(updatefd, &(clientMan->entries[i].hash[bytesWritten]), 32); // write in hash
                    bytesWritten += hashBytesWritten;
                }
                */
                printf("D %s\n", clientMan->entries[i].path);
            }
            
        }

        // now, check and see if there were any extra files on server manifest
        for(i = 0; i < serverMan->numOfEntries; i++){
            if(!(extraServerManFiles[i])){ // true if file was never matched with
                sprintf(updateMsg, "A %s %s\n", serverMan->entries[i].path, serverMan->entries[i].hash);
                int bytesToWrite = strlen(updateMsg);
                int bytesWritten = 0;
                while(bytesWritten < bytesToWrite){
                    bytesWritten += write(updatefd, updateMsg, strlen(updateMsg));
                }
                printf("A %s\n", serverMan->entries[i].path);
            }
        }

        if(!usedConflict){ // remove .Conflict if unused
            remove(conflictPath);
        }

        printf("Update complete!\n");
        return 1;



    }else if( strcmp(argv[1], "upgrade") == 0 ){ // upgrade command called ########################### U P G R A D E ########################

        struct dirent *pDirent;
        DIR *pDir;
        pDir = opendir("./");
        if(pDir == NULL){
            printf("Directory doesnt exist \n");
            return 1;
        }
        int truth = 0;
        while((pDirent = readdir(pDir))!= NULL){
            //ls
            printf("%s",pDirent->d_name);
            //printf("\n");
            if(strcmp(pDirent->d_name,argv[2])==0){
                truth = 1;
                break;
            }
            else{
                truth = 0;
            }
        }
        if(truth ==0){
            printf("Project directory doesnt exist\n");
            return 1;
        }       
        
        char updatePath[4120] = "./";
        char conflictPath[4120];
        strcat(updatePath,argv[2]);//we now have ./projectname
        strcpy(conflictPath,updatePath);
        strcat(conflictPath,"/.Conflict");
        strcat(updatePath,"/.Update");//we now have ./projectname/.Update (how to check if this doesnt exsit:?)

        printf("This is the path to the .Update file:\n%s\n",updatePath);
        int conflict = open(conflictPath,O_RDONLY | O_NONBLOCK);
        if(conflict != -1){
            printf("CONFLICT EXISTS, PLEASE RESOLVE ALL CONFLICTS THEN UPDATE\n");
            return 1;
        }
        int fd = open(updatePath,O_RDONLY | O_NONBLOCK);
        if(fd == -1){
            printf("Update file does not exist, you must make a Update first\n");
            return 1;
        }//must turn all thhe information in .Update into string now.
        char* updateFile = (char*)malloc(100*sizeof(char));
        char* reallocPtr;
        int filesize = 0;
        int status = 0;
        int fd2 = open(updatePath,O_RDONLY | O_NONBLOCK);
        int status1 = 0;
        int filesize1 = 0;
        if((status1=read(fd2,&(updateFile[filesize1]),100)==0)){
            printf("There is nothing in the file, you are UP TO DATE\n");
            remove(updatePath);
            close(fd2);
            return 0;
        }
        do{
            status = read(fd,&(updateFile[filesize]),100);
            filesize+=status;
            if(status ==100){
                reallocPtr = realloc(updateFile,2*filesize*sizeof(char));
                if(reallocPtr ==NULL){
                    printf("Malloc failed... how big is your input? ... bad user\n");
                    return 0;
                }else{
                    updateFile = reallocPtr;
                }
            }
        }while(status>0);
        updateFile[filesize] = '\0';
        close(fd);
        printf("This is the .Update file:\n%s",updateFile);
        int i = 0;
        while(updateFile[i]!='\0'){
            if(updateFile[i]=='D'){
                //time to get the path connected to D
                i++;//we are at space
                i++;//skipping space.
                int j = i;//get size of the path first
                int size = 0;
                while(updateFile[j] !=' '){ 
                    size++;
                    j++;
                }
                printf("\n");
                //we now have the size. Malloc a sring big enough
                char* pathCopy = (char*)malloc(size*sizeof(char));
                j = 0;
                while(updateFile[i]!=' '){
                    pathCopy[j]=updateFile[i];
                    j++;
                    i++;
                }
                pathCopy[size]='\0';
                printf("THIS IS THE PATH WE JUST RETRIEVED:%s\n",pathCopy);
                //now get hash then call the command delete
                //we are at a space rn. So gotta skip it
                i++;
                //now we are at hash. Read until \n, we already know size of hash too.
                char* hash = (char*)malloc(32*sizeof(char));
                j = 0;
                while(updateFile[i]!='\n'){
                    hash[j]=updateFile[i];
                    j++;
                    i++;
                }
                hash[32]='\0';
                printf("This IS THE HAS OF THE PATH WE JUST RETRIEVED:%s\n",hash);
                //Now call the function
                deleteU(pathCopy,hash,argv[2]);
                free(pathCopy);
                free(hash);
            }
            else if(updateFile[i]=='A'){
                //get the path
            }
            else if(updateFile[i]=='M'){

            }
            i++;
        }
        int fd1 = open(updatePath,O_RDONLY | O_NONBLOCK);
        Update* update = createUpdateStructureFromFile(fd1);
        close(fd1);
        i = 0;
        for(i=0;i<update->numOfEntries;i++){
            printf("TESTING MODIFIED CODE:%s\n",update->entries[i].version);
            if(update->entries[i].version[0] =='A'){
            int numOfSlashes = 0;
            char* pathToSubdirectories[strlen(update->entries[i].path)];
            int k;
                for(k = 0; k < strlen(update->entries[i].path); k++){

                    if(update->entries[i].path[k] == '/'){ // if this is true then that means a subdirectory might need to be made
                        numOfSlashes++;
                        if(numOfSlashes > 2){ // subdirectory needs to be made
                            pathToSubdirectories[k] = '\0';
                            //printf("creating subdirectory with path: '%s'\n", pathToSubdirectories);
                            mkdir(pathToSubdirectories, 0777);
                        }
                        //indexOfPreviousSlash = k;
                    }
                    pathToSubdirectories[k] = update->entries[i].path[k];
                    //printf("put %c in path to subdirectories\n", manifest->entries[i].path[k]);
                }
            }
        }//DIRECTORIES FOR ADD CREATED
        int socketFD = connectToServer(host,port);
        printf("Sending over the upgrade command to server\n");
        SendToServer(socketFD,argv[1]);
        sleep(1);
        printf("Sending over the .Update file now to server\n");
        char length[strlen(updateFile)+100];
        sprintf(length,"%d:",strlen(updateFile));
        SendToServer(socketFD,length);
        SendToServer(socketFD,updateFile);
        int k;
        for(k=0;k<update->numOfEntries;k++){
            if(strcmp(update->entries[k].version,"A")==0 || strcmp(update->entries[k].version,"M")==0){
                int bytesToRead = 0;
                do{
                    bytesToRead = readInIntegerBeforeBytes(socketFD);
                }while(bytesToRead<1);
                char* fileFromServer = readInBytes(socketFD,bytesToRead);
                printf("THIS IS FROM THE SERVER: %s\n",fileFromServer);
                //now that we retrieved the information from server we have to open up the path and copy paste it.
                int fd = open(update->entries[k].path,O_RDWR | O_CREAT, 00644);
                write(fd,fileFromServer,strlen(fileFromServer));
                close(fd);
            }
        }
        printf("Upgrade Completed!\n");
        remove(updatePath);

    }else if( strcmp(argv[1], "commit") == 0 ){ // commit command called ################################################ C O M M I T #############################################################################################
                                                                        //#########################################################################################################################################################
        // if client has .Update or .Conflict file, commit should fail
        // so, check for .Update first
        char pathToUpdateOrConflict[strlen(argv[2]) + 14];
        sprintf(pathToUpdateOrConflict, "./%s/.Update", argv[2]);
        int update_fd = open(pathToUpdateOrConflict, O_RDONLY, 00644);
        if(update_fd > 0){ // .Update file exists
            char garbage;
            if(read(update_fd, &garbage, 1)){ // .Update file is not empty
                printf(".Update file is not empty. commit failed\n");
                return 0;
            }
            close(update_fd);
        }
        
        // now check for .Conflict
        sprintf(pathToUpdateOrConflict, "./%s/.Conflict", argv[2]);
        int conflict_fd = open(pathToUpdateOrConflict, O_RDONLY, 00644);
        if(conflict_fd > 0){
            printf(".Conflict file exists. commit failed\n");
            return 0;
        }

        // ok, now fetch .Manifest from server
        int socketFD = connectToServer(host,port);
        printf("sending commit command to server\n");
        SendToServer(socketFD,argv[1]);
        sleep(1);
        char projectNameLengthStr[22];
        sprintf(projectNameLengthStr, "%d:", strlen(argv[2]));
        SendToServer(socketFD, projectNameLengthStr);
        SendToServer(socketFD, argv[2]);
        
        int numOfBytesToRead = -2;
        do{// loop until the client writes to server -- first thing written will be the length of the manifest
            numOfBytesToRead = readInIntegerBeforeBytes(socketFD);
        }while(numOfBytesToRead < -1); // will break out of loop once length of manifest file is read in
        //printf("manifest version num: %d, should be 8\n", manifestVersionNum);

        if(numOfBytesToRead == -1){ // this means server sent a '!' which means the project was not found

            printf("server could not find this project. commit failed.\n");
            return 0;
        }

        char* serverManStr = readInBytes(socketFD, numOfBytesToRead);
        printf("server's manifest: '%s', size: %d\n", serverManStr, strlen(serverManStr));
        
        // server's manifest successfully received
        // now, turn it into struct
        int manifestfd = open("./.temp", O_CREAT | O_RDWR | O_TRUNC, 00644);
        int bytesToWrite = strlen(serverManStr);
        int bytesWritten = 0;
        while(bytesWritten < bytesToWrite){
            bytesWritten += write(manifestfd, serverManStr+bytesWritten, strlen(serverManStr));
        }
        //write(manifestfd, "\n", 1);
        close(manifestfd);
        int manfd = open("./.temp", O_RDONLY, 00644);
        Manifest* serverMan = createManifestStructFromManifestFile(manfd);
        close(manfd);
        //sleep(15);
        remove("./.temp");
        // got server manifest all set up now

        // now, get client manifest all set up
        char pathToManifest[strlen(argv[2]) + 14];
        sprintf(pathToManifest, "./%s/.Manifest", argv[2]);
        int manifestfd2 = open(pathToManifest, O_RDONLY, 00644);
        Manifest* clientMan = createManifestStructFromManifestFile(manifestfd2);
        close(manifestfd2);
        // client manifest is all set up now

        /* BULLSHIT TESTING
        printf("both manifests are structs\n");
        int test = open("./test_project/garbagemanifest", O_RDONLY, 00644);
        printf("file opened\n");
        Manifest* test2 = createManifestStructFromManifestFile(test);
        printf("manifest struct created\n");
        char* test3 = manifestStructToString(test2);
        printf("manifest string created\n");
        printf("test manifest: '%s'", test3);
        return 1;
        */

        // first, check if manifest version numbers are different - if so, commit failed
        if(strcmp(serverMan->version, clientMan->version) != 0){
            write(socketFD, "!", 1); // tell server that commit failed and nothing is coming to it
            printf("commit failed - you must update local project before you can commit\n");
            return 0;
        }

        // now, compare all files and write to .Commit if applicable
        char commitPath[strlen(argv[2]) + 12];
        sprintf(commitPath, "./%s/.Commit", argv[2]);
        int commitfd = open(commitPath, O_RDWR | O_CREAT | O_TRUNC, 00644);
        //int usedConflict = 0; // keep track of if you write to .Conflict because if not, remove the file at end
        char updateMsg[4045];
        int extraServerManFiles[serverMan->numOfEntries]; // use to keep track of any server manifest files that aren't matched with files from client manifest
        int i;
        int k;
        for(i = 0; i < serverMan->numOfEntries; i++){ // fill all entries with 0 at first, then 1 as you match with them
            extraServerManFiles[i] = 0;
        }

        int usedCommit = 0;
        printf("\n\n\n\n\nmnifest num of entries: %d\n\n\n\n", clientMan->numOfEntries);

        /*
        for(i = 0; i < clientMan->numOfEntries; i++){
            printf("first letter of hash: %c\n", clientMan->entries[i].hash[1]);
        }
        return 0;
        */
       printf("version: '%s' path: '%s' hash: '%s'\n", clientMan->entries[6].version, clientMan->entries[6].path, clientMan->entries[6].hash);
       //return 0;
        
        for(i = 0; i < clientMan->numOfEntries; i++){ // loop through all client manifest files
            int foundFile = 0;
            
            char* temp = fileToString(open(clientMan->entries[i].path, O_RDONLY));
            
            char* currentFileMD5 = str2md5(temp, strlen(temp));
            printf("\n------\nnow looking for file: %s\n", clientMan->entries[i].path);
            
            for(k = 0; k < serverMan->numOfEntries; k++){ // loop through all server manifest files

                if(strcmp(clientMan->entries[i].path, serverMan->entries[k].path) == 0){ // found file match
                    printf("file match\n");
                    foundFile = 1;
                    extraServerManFiles[k] = 1;
                    if(strcmp(serverMan->entries[k].hash, clientMan->entries[i].hash) != 0 && atoi(clientMan->entries[i].version) > atoi(serverMan->entries[k].version)){ // file's hash is different in server and client manifests AND version is higher in client than server
                            // FAILURE CASE
                            printf("commit failed - you must sync with the repository before committing changes\n");
                            write(socketFD, "!", 1);
                            remove(commitPath);
                            return 0;
                    }

                    if(strcmp(clientMan->entries[i].hash, serverMan->entries[k].hash) == 0){ // hash for this file in server and client manifests is the same
                        printf("hash match\n");


                        if(strcmp(currentFileMD5, clientMan->entries[i].hash) != 0){ // live hash of client file does not match what is on client manifest but version on client is less than or equal to server
                            // **** increment file version 
                            /*
                            printf("live hash mismatch\nincrementing file: %s.\n", clientMan->entries[i].path);
                            int temp = atoi(clientMan->entries[i].version);
                            temp++;
                            sprintf(clientMan->entries[i].version, "%d", temp);
                            // update version number on .Manifest
                            char* tempManStr = manifestStructToString(clientMan);
                            //printf("manifest to overwrite with: '%s'\n", tempManStr);
                            printf("about to trunc manifest. new manifest: '%s'\n", tempManStr);
                            int tempManfd = open(pathToManifest, O_RDWR | O_TRUNC, 00644);
                            writeToFile(tempManfd, tempManStr);
                            close(tempManfd);
                            free(tempManStr);
                            // version number incremented
                            */

                            // append to .Commit
                            sprintf(updateMsg, "M %d %s %s\n", atoi(clientMan->entries[i].version)+1, clientMan->entries[i].path, serverMan->entries[k].hash);
                            writeToFile(commitfd, updateMsg);
                            printf("M %s\n", clientMan->entries[i].path);
                            usedCommit = 1;
                        }else{ // files are same, do nothing
                            continue;
                        }
                    }
                }
            }

            if(!foundFile){ // file not found on server...this means client has file that server does not
                printf("!foundFile");
                // **** increment file version number
                /*
                int temp = atoi(clientMan->entries[i].version);
                temp++;
                sprintf(clientMan->entries[i].version, "%d", temp);
                // update version number on .Manifest
                char* tempManStr = manifestStructToString(clientMan);
                int tempManfd = open(pathToManifest, O_RDWR | O_TRUNC, 00644);
                writeToFile(tempManfd, tempManStr);
                close(tempManfd);
                free(tempManStr);
                // version number incremented
                */
                // append to .Conflict and print to stdout
                sprintf(updateMsg, "A %d %s %s\n", atoi(clientMan->entries[i].version)+1, clientMan->entries[i].path, clientMan->entries[i].hash);
                writeToFile(commitfd, updateMsg);
                printf("A %s\n", clientMan->entries[i].path);
                usedCommit = 1;
            }
        }
            
            
        
        

        // now, check and see if there were any extra files on server manifest
        for(i = 0; i < serverMan->numOfEntries; i++){
            if(!(extraServerManFiles[i])){ // true if file was never matched with

                /*
                // **** increment file version number
                int temp = atoi(serverMan->entries[i].version);
                temp++;
                sprintf(serverMan->entries[i].version, "%d", temp);
                // update version number on .Manifest
                //char* tempManStr = manifestStructToString(clientMan);
                //int tempManfd = open(pathToManifest, O_RDWR | O_TRUNC, 00644);
                //writeToFile(tempManfd, tempManStr);
                //close(tempManfd);
                //free(tempManStr);
                // version number incremented
                */

                sprintf(updateMsg, "D %d %s %s\n", atoi(serverMan->entries[i].version)+1, serverMan->entries[i].path, serverMan->entries[i].hash);
                writeToFile(commitfd, updateMsg);
                printf("D %s\n", serverMan->entries[i].path);
                usedCommit = 1;
            }
        }

        if(!usedCommit){ // remove .Conflict if unused and tell server no .Commit is coming
            close(commitfd);
            remove(commitPath);
            write(socketFD, "!", 1);
            printf("no changes to be made.\n");
            return 0;
        }

        // done writing to .Commit, now send it to server
        lseek(commitfd, 0, SEEK_SET);
        char* commitStr = filesToSocketMessage(&commitfd, 1);
        writeToFile(socketFD, commitStr); // write commit file to socket here
        // done sending .Commit, now wait for server's response

        char response = '?';
        do{
            read(socketFD, &response, 1);
        }while(response == '?');

        if(response == '1'){
            printf("server received .Commit file - commit succesful!\n");
        }else{
            printf("there was some issue with the server receiving the .Commit file. commit failed\n");
        }

        return 1;


    }else if( strcmp(argv[1], "push") == 0 ){ // push command called ##################################################################################### P U S H #############################################################################
                                                // #############################################################################################################################################################################################
        // check for existence of .Commit on client side
        char commitPath[strlen(argv[2]) + 12];
        sprintf(commitPath, "./%s/.Commit", argv[2]);
        int commitfd = open(commitPath, O_RDWR, 00644);
        if(commitfd < 0){ // .Commit file doesn't exist
            printf("push failed - client has no .Commit file\n");
            return 0;
        }


        // connect to server and tell it we are doing push command
        int socketFD = connectToServer(host,port);
        printf("sending push command to server\n");
        SendToServer(socketFD,argv[1]);
        sleep(1);

        // now send project name to server to ensure it exists
        char projectNameLengthStr[22];
        sprintf(projectNameLengthStr, "%d:", strlen(argv[2]));
        SendToServer(socketFD, projectNameLengthStr);
        SendToServer(socketFD, argv[2]);

        // receive response on whether or not server has project
        int status = 0;
        char response = '?';
        while(status < 1){
            status = read(socketFD, &response, 1);
        }
        if(response == '!'){
            printf("server does not have this project. push failed.\n");
            return 0;
        }else{
            printf("this project exists on the server. now sending .Commit to server\n");
        }
        
        // now, send .Commit and all files listed in it to server
        // first, parse .Commit to get the paths of the files listed. store in an array
        char path[4097]; // max length of path is 4096 bytes
        char* commitStr = fileToString(commitfd);
        lseek(commitfd, 0, SEEK_SET);
        // find nummber of entries in .Commit first
        int i;
        int numOfEntries = 0;
        for(i = 0; i < strlen(commitStr); i++){
            if(commitStr[i] == '\n'){
                numOfEntries++;
            }
        }
        // create array of ints to hold fd's of these files
        int fds[numOfEntries + 1];
        // put .Commit fd in first spot
        fds[0] = commitfd;
        int fdsEntries = 1;
        int numOfSpaces = 0;
        int pathLength = 0;
        int startIndex = 0;
        int endIndex = 0;
        //char tag = '?';
        //int onTag = 1;
        // now get paths of files and open them and store in fds array
        for(i = 0; i < strlen(commitStr); i++){
            /*
            if(onTag){
                tag = commitStr[i];
                onTag = 0;
            }
            */
            if(commitStr[i] == ' '){
                numOfSpaces++;
            }
            if(numOfSpaces == 2){ // have reached path
                startIndex = i + 1;
                // find end of path
                i++;
                while(commitStr[i] != ' '){
                    pathLength++;
                    i++;
                }
                endIndex = i - 1;
                pathLength = endIndex - startIndex;
                //printf("path length: %d\n", pathLength);
                char path[pathLength + 1];
                int k;
                int pathIndex = 0;
                for(k = startIndex; k <= endIndex; k++){
                    path[pathIndex] = commitStr[k];
                    pathIndex++;
                }
                path[pathLength+1] = '\0';
                //printf("path: %s\n", path);
                fds[fdsEntries] = open(path, O_RDONLY, 00644);
                fdsEntries++;
                numOfSpaces = 0;
                i = i + 30; // skip through hash - unnecessary
            }
            
        }
        
        // ok, array of fd's is made, now make string out of it and send to server
        char* strForServer = filesToSocketMessage(fds, numOfEntries + 1);
        //printf("str for server: '%s'", strForServer);
        writeToFile(socketFD, strForServer); // write to socket fd here
        printf("wrote this to server: '%s'\n", strForServer);

        // listen for response
        response = '?';
        status = 0;
        while(status < 1){
            status = read(socketFD, &response, 1);
        }
        if(response == '!'){
            printf("push failed\n");
        }else{
            printf("push successful!\n");
        }
        close(commitfd);
        remove(commitPath);
        return 1;



    }else if( strcmp(argv[1], "create") == 0 ){ // create command called, i do this ############################################ C R E A T E ###########################3
    //have client send project name along with the instruction and have server build it on server side, then server will send back to client
     /*if(isProjectInDirectory("./", argv[2]) == 1){ // project already exists on client side, so checkout fails
            printf("project already exists on client - create failed\n");
            return 0;
        }*/

        int socketFD = connectToServer(host,port);
        printf("sending create command to server\n");
        SendToServer(socketFD,argv[1]);
        sleep(1);
        char projectNameLengthStr[22];
        sprintf(projectNameLengthStr,"%d:",strlen(argv[2]));
        SendToServer(socketFD, projectNameLengthStr);
        SendToServer(socketFD,argv[2]);
        int numOfBytesToRead = 0;
        do{//first thing written will be number of bytes to read in 
            numOfBytesToRead = readInIntegerBeforeBytes(socketFD);
        }while(numOfBytesToRead <1);
        //printf("THIS IS NUMBER OF BYTES:%d\n",numOfBytesToRead);
        char* manifestStr = readInBytes(socketFD,numOfBytesToRead);//manifest file saved as a string now
        //printf("THIS IS MANIFESTstr:%s\n",manifestStr);
        //har* check = "!";
        if(strcmp(manifestStr,"!")== 0){
            printf("Create failed - project already exists on server\n");
            return 0;
        }
    
        //now make project directory on this side.
        mkdir(argv[2],0777);//directory for project created
        char path[sizeof(argv[2])+200] = "./";
        strcat(path,argv[2]);
        strcat(path,"/.Manifest");
        //path to .Manifest created
        int manfd = open(path, O_CREAT | O_RDWR, 00644);
        int bytesWritten = 0;
        write(manfd,manifestStr,strlen(manifestStr));
        printf("Directory for project created on client side and server side - create successful\n");

    //manifest file created.Done.
    }else if( strcmp(argv[1], "destroy") == 0 ){ // destroy command called, i do this #######################3 D E S T R O Y ########################


        int socketFD = connectToServer(host,port);
        printf("Sending Destroy command to server\n");
        SendToServer(socketFD,argv[1]);
        sleep(1);
        char projectNameLengthStr[22];
        sprintf(projectNameLengthStr,"%d:",strlen(argv[2]));
        SendToServer(socketFD,projectNameLengthStr);
        SendToServer(socketFD,argv[2]);
        //SendToServer(socketFD,argv[1]);
        int numOfBytesToRead = 0;
        do{
            numOfBytesToRead = readInIntegerBeforeBytes(socketFD);
        }while(numOfBytesToRead<1);
        char* message = readInBytes(socketFD,numOfBytesToRead);
        printf("Message from Server: %s\n",message);


    }else if( strcmp(argv[1], "currentversion") == 0 ){ // currentversion command called ###################3 C U R R E N T V E R S I O N #################

        int socketFD = connectToServer(host,port);
        printf("sending currentversion command to server\n");
        SendToServer(socketFD,argv[1]);
        sleep(1);
        char projectNameLengthStr[22];
        sprintf(projectNameLengthStr, "%d:", strlen(argv[2]));
        SendToServer(socketFD, projectNameLengthStr);
        SendToServer(socketFD, argv[2]);
        
        int manifestVersionNum = -2;
        do{// loop until the client writes to server -- first thing written will be the manifest version number
            manifestVersionNum = readInIntegerBeforeBytes(socketFD);
        }while(manifestVersionNum < -1); // will break out of loop once manifest version number is found
        //printf("manifest version num: %d, should be 8\n", manifestVersionNum);

        if(manifestVersionNum == -1){ // this means server sent a '!' which means the project was not found

            printf("server could not find this project. currentversion failed.\n");
            return 0;
        }

        //sleep(1);

        printf("\n*************************\nproject name: %s\n", argv[2]);
        printf("manifest version: %d\n", manifestVersionNum);

        // now read in version numbers and paths written to the socket
        int bytesToRead = 0;
        int versionNum = 0;
        int status = 0;
        char garbo;
        do{
            versionNum = readInIntegerBeforeBytes(socketFD);
            //read(socketFD, &garbo, 1);
            bytesToRead = readInIntegerBeforeBytes(socketFD);
            //read(socketFD, &garbo, 1);
            char* temp = readInBytes(socketFD, bytesToRead);
            printf("file: %s, version: %d\n", temp, versionNum);
            free(temp);
            status = read(socketFD, &garbo, 1);

        }while(status > 0);

        printf("************************\n\ncurrentversion successful!\n");
        return 1;

    }else if( strcmp(argv[1], "history") == 0 ){ // history command called ######################### H I S T O R Y ##################
        

        int socketFD = connectToServer(host,port);
        //first must check if it exists on server side
        //int truth = 0;

        
        printf("Sending over 'history' command to server\n");
        //SendToServer(socketFD,argv[1]);
        SendToServer(socketFD,argv[1]);
        sleep(1);
        char LengthOfProject[strlen(argv[2])];
        sprintf(LengthOfProject,"%d:",strlen(argv[2]));
        SendToServer(socketFD,LengthOfProject);
        SendToServer(socketFD,argv[2]);
        //sleep(1);
        //printf("THIS IS TRUTH %d\n",truth);
        
        int numOfBytesToRead = 0;
        do{
            numOfBytesToRead = readInIntegerBeforeBytes(socketFD);
        }while(numOfBytesToRead<1);
        char* historyContents = readInBytes(socketFD,numOfBytesToRead);
        printf("THIS IS THE HISTORY OF %s: \n%s\n",argv[2],historyContents);

    }else if( strcmp(argv[1], "rollback") == 0 ){ // rollback command called ##################### R O L L B A C K ##############3

        int socketFD = connectToServer(host,port);
        printf("sending rollback command to server\n");
        SendToServer(socketFD,argv[1]);
        sleep(1);

        // now send project name and version number to server
        char projectNameLengthStr[22];
        sprintf(projectNameLengthStr, "%d:", strlen(argv[2]));
        write(socketFD, projectNameLengthStr, strlen(projectNameLengthStr));
        return 1;
        SendToServer(socketFD, projectNameLengthStr);
        SendToServer(socketFD, argv[2]);
        sprintf(projectNameLengthStr, "%d:", strlen(argv[3]));
        SendToServer(socketFD, projectNameLengthStr);
        SendToServer(socketFD, argv[3]);

        // wait for success/failure response
        int status = 0;
        char response = '?';
        while(status < 1){
            status = read(socketFD, &response, 1);
        }
        if(response == '!'){
            printf("rollback failed...either invalid project name or version name given to server\n");
        }else{
            printf("rollback successful!\n");
        }
        return 0;

    }else{ // some incompatible command called

        printf("invalid command called...exiting\n");
        return 0;

    }

}


