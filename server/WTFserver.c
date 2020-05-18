#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include<pthread.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

void error(char *msg){
    perror(msg);
    exit(1);
}


int writeToFile(int fd, char* str){ // given fd and str, will write str to fd. returns number of bytes it wrote
    //printf("got string: '%s'", str);
    int bytesToWrite = strlen(str);
    int bytesWritten = 0;
    while(bytesWritten < bytesToWrite){
        bytesWritten += write(fd, str+bytesWritten, bytesToWrite-bytesWritten);
        //printf("wrote %d bytes\n", bytesWritten);
    }
    return bytesWritten;
}

void SendToClient(int socketFD,char* message){
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

int countDigits(int val){
    if(val == 0){
        return 1;
    }
    int result = 0;
    while(val != 0){
        val /= 10;
        result++;
    }
    return result;
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


int readInIntegerBeforeBytes(int sockfd){ // returns an integer which will be the number of bytes to read next
    //printf("got to readInInteger function\n");
    char* intString = (char*)malloc(20 * sizeof(char));
    char* reallocPtr = NULL;
    int stringSize = 20;
    int numOfBytesRead = -1;
    char buffer = '?';

    while(buffer != ':'){
        if(buffer == '!'){
            return -1;
        }
        //printf("buffer: %c\n", buffer);
        numOfBytesRead += read(sockfd, &buffer, 1);
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
    }
    intString[numOfBytesRead] = '\0';
    //printf("intString before conversion to int: '%s'\n", intString);
    int ret = atoi(intString);
    free(intString);
    return ret;
}

char* manifestStructToString(Manifest* manifest){ // turns given manifest struct pointer into string -> returns pointer to that string
    char* ret = (char*)malloc(200 * sizeof(char)); // start with room for 200 bytes
    char* reallocPtr = NULL;
    int bytesAllocated = 200;
    int lineLength = 0;
    int bytesWritten = sprintf(ret, "%s\n", manifest->version);
    int i;
    //printf("ret: '%s'\n", ret);

    for(i = 0; i < manifest->numOfEntries; i++){
        //printf("version: '%s', path: '%s', hash: '%s'\n", manifest->entries[i].version, manifest->entries[i].path, manifest->entries[i].hash);

        if(manifest->entries[i].path == NULL || manifest->entries[i].path[0] != '.'){
            //return ret;
            continue;
        }
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
        printf("ret: '%s'\n", ret);
    }
    return ret;
}

char* readInBytes(int sockfd, int numOfBytesToRead){ // returns a pointer to a string that contains the bytes that the server read in

    char* stringOfBytes = (char*)malloc((numOfBytesToRead + 1) * sizeof(char));
    char* reallocPtr = NULL;
    //int stringSize = 100;
    int numOfBytesRead = 0;
    int status = 0;
    do{
        numOfBytesRead += read(sockfd, &(stringOfBytes[numOfBytesRead]), numOfBytesToRead - numOfBytesRead);
        //(*numOfBytes) += status; // keep track of how many bytes you read in
    }while(numOfBytesToRead > numOfBytesRead);

    stringOfBytes[numOfBytesToRead] = '\0';
    return stringOfBytes;
}

DIR* openProjectDirectory(char* projectName){ // opens and returns directory for given project name. returns NULL if directory isnt found, or DIR* to directory if it is found
    struct dirent *pDirent;
    DIR *pDir;
    pDir = opendir("./");
    while((pDirent = readdir(pDir))!= NULL){
        if( strcmp(pDirent->d_name, projectName) == 0 ){ // project name is a match to an entry in the directory -> project found
            char temp[3 + strlen(pDirent->d_name)];
            sprintf(temp, "./%s", pDirent->d_name);
            return opendir(temp);
        }
    }

    return NULL; // no matches in directory -> project not found
        
} // THIS FUNCTION PROBABLY IS NOT NEEDED^

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

char* filesToSocketMessage(int* fds, int fds_size){ // input an array of file descriptors and the number of file descriptors in it, returns client-friendly string ready to write to socket
    int i;
    char* strForClient = (char*)malloc(101 * sizeof(char));
    strForClient[0] = '\0';
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
            reallocPtr = realloc(strForClient, (allocatedBytesForString + 1) * sizeof(char)); // double size of array
            if(reallocPtr == NULL){
                printf("Malloc failed when reading in files...how big are your files?...bad user\n");
                return NULL;
            }else{
                strForClient = reallocPtr;
            }
        }else{
            //printf("\ntotal num of bytes read is %d and length of fileLengthStr is %d and allocated bytes is %d so no need to realloc\n", totalNumOfBytesRead, strlen(fileLengthStr), allocatedBytesForString);
        }

        
        for(k = 0; k < strlen(fileLengthStr) + 1; k++){
            
            if(k == strlen(fileLengthStr)){ // put ':' because the number is finished
                strForClient[totalNumOfBytesRead] = ':';
                totalNumOfBytesRead++;
            }else{
                strForClient[totalNumOfBytesRead] = fileLengthStr[k];
                totalNumOfBytesRead++;
            }
        }
        
        //sprintf(strForClient, "%s%s:", strForClient, fileLengthStr);
        // done writing file length to string

        lseek(fds[i], 0, SEEK_SET); // go back to beginnning of file

        // now write actual file contents to string
        do{
            while(totalNumOfBytesRead + fileLength >= allocatedBytesForString){ // need to realloc!
                //printf("\nhave to realloc because total num of bytes read is %d and length of fileLengthStr is %d and allocated bytes is %d...currently writing in file contents\n", totalNumOfBytesRead, strlen(fileLengthStr), allocatedBytesForString);
                allocatedBytesForString *= 2;
                reallocPtr = realloc(strForClient, (allocatedBytesForString + 1) * sizeof(char)); // double size of array
                if(reallocPtr == NULL){
                    printf("Malloc failed when reading in files...how big are your files?...bad user\n");
                    return NULL;
                }else{
                    strForClient = reallocPtr;
                }
            }
            status = read(fds[i], &(strForClient[totalNumOfBytesRead]), fileLength - bytesReadForThisFile);
            //printf("read in %d bytes\n", allocatedBytesForString - (fileLength + bytesReadForThisFile));
            bytesReadForThisFile += status;
            totalNumOfBytesRead += status;
            //printf("%d\n", totalNumOfBytesRead);
        }while(status > 0);
        // done writing file contents to string

        if(i != fds_size - 1){
            strForClient[totalNumOfBytesRead] = ':';
            totalNumOfBytesRead++;
        }


    }

    strForClient[totalNumOfBytesRead] = '\0';
    //printf("\n'%s'\n", strForClient);
    return strForClient;
}

int checkout(int sockfd, char* projectName){ // given socket and desired project, will write appropriate files to socket for client to grab

    printf("reached checkout func\n");
    char pathToManifest[13 + strlen(projectName)]; // allocate space for path to Manifest file
    sprintf(pathToManifest, "./%s/.Manifest", projectName); // write path name into string allocated above
    //printf("path to manifest: %s\n", pathToManifest);
    int manfd = open(pathToManifest, O_RDONLY); // open .Manifest file
    Manifest* manifest = createManifestStructFromManifestFile(manfd); // create a Manifest struct from .Manifest file
    printf("man struct made\n");
    int fds[manifest->numOfEntries + 1]; // create and fill an array of file descriptors of .Manifest file and the files listed in it. This array will be passed to filesToSocketMessage function
    fds[0] = manfd; // first fd is the .Manifest fd
    int i;
    for(i = 0; i < manifest->numOfEntries; i++){ // fill the rest of the array with the files listed in the .Manifest
        //printf("opening file: '%s'\n", manifest->entries[i].path);
        fds[i + 1] = open(manifest->entries[i].path, O_RDONLY);
    }
    
    //printf("'%s'\n", filesToSocketMessage(fds, manifest->numOfEntries + 1));
    char* messageForClient = filesToSocketMessage(fds, manifest->numOfEntries + 1);

    //printf("\n****\nmessage for client, being written to socket fd: '%s'\n****\n", messageForClient);
    write(sockfd, messageForClient, strlen(messageForClient));

    free(messageForClient); //*********
    return 1;
    

}

int create(int sockfd, char* projectName){//must create the project on server side rn and then add a manifest file to it
    //we have to open up a directory called project1 then write a manifest file to it
    mkdir(projectName,0777);
    printf("Directory for project created on server side\n");
    //now we must create the manifest file and put it in the emre directory.
    char path[sizeof(projectName)+200] = "./";
    strcat(path,projectName);
    strcat(path,"/.Manifest");
    //printf("This is the path to add .Manifest %s\n",path);
    int manifest = open(path, O_RDWR | O_CREAT | O_TRUNC | O_APPEND,00644);
    write(manifest,"0",1);
    
   // open(//path which is ./projectname/.Manifes)
   //printf("CREATE DONE ON SERVER SIDE,NOW PASS MANIFEST OVER TO CLIENT SIDE\n");
   int fd = open(path,O_RDONLY);
   char* manString = fileToString(fd);
   //printf("THIS IS THE STRING BEING PASSED THROUGH:%s\n",manString);
      //now send to client the .Manifest.
    char send[sizeof(manString)+200];
    sprintf(send,"%d:",strlen(manString));
    SendToClient(sockfd,send);
    SendToClient(sockfd,manString);
}

void removeDIR(char* projectName){
    char str[400] = "rm -rf ./";
    strcat(str,projectName);
    system(str);
}

int currentversion(int sockfd, char* projectName){ // given socket and desired project, will write appropriate .Manifest info to socket for client to grab

    char pathToManifest[13 + strlen(projectName)]; // allocate space for path to Manifest file
    sprintf(pathToManifest, "./%s/.Manifest", projectName); // write path name into string allocated above
    //printf("path to manifest: %s\n", pathToManifest);
    int manfd = open(pathToManifest, O_RDONLY); // open .Manifest file
    if(manfd < 0){ // project doesn't exist on server
        printf("this project does not exist on the server\n");
        write(sockfd, "!", 1);
        return -1;
    }
    Manifest* manifest = createManifestStructFromManifestFile(manfd); // create a Manifest struct from .Manifest file
    int i;
    // find proper size for string containing message for client here
    int strSize = strlen(manifest->version) + 2;
    for(i = 0; i < manifest->numOfEntries; i++){
        strSize += strlen(manifest->entries[i].version) + 1 + strlen(manifest->entries[i].path) + 1 + countDigits(strlen(manifest->entries[i].path)) + 1;
    }

    // proper size found, so now malloc that string
    char* strForClient = (char*)malloc(strSize + sizeof(char));
    
    sprintf(strForClient, "%s:", manifest->version);
    
    for(i = 0; i < manifest->numOfEntries; i++){
        
        char temp[21];
        sprintf(temp, "%d:", strlen(manifest->entries[i].path));
        strcat(strForClient, manifest->entries[i].version);
        strcat(strForClient, ":");
        strcat(strForClient, temp);
        //strcat(strForClient, ":");
        strcat(strForClient, manifest->entries[i].path);
        if(i < manifest->numOfEntries - 1){
            strcat(strForClient, ":");
        }
        
    }
    //printf("str for client: '%s'\n", strForClient);
    int bytesWritten = 0;
    int bytesToWrite = strlen(strForClient);
    //strForClient[bytesToWrite] = '\0';
    do{
        bytesWritten += write(sockfd, strForClient, bytesToWrite - bytesWritten);
    }while(bytesWritten < bytesToWrite);
    // all versions and paths written in at this point
    
    freeManifestStruct(manifest);
    free(strForClient);
    printf("currentversion successful!\n");
    return 1;
}

int update(int sockfd, char* projectName){ // given socket and desired project, will write appropriate .Manifest info to socket for client to grab

    char pathToManifest[13 + strlen(projectName)]; // allocate space for path to Manifest file
    sprintf(pathToManifest, "./%s/.Manifest", projectName); // write path name into string allocated above
    //printf("path to manifest: %s\n", pathToManifest);
    int manfd = open(pathToManifest, O_RDONLY); // open .Manifest file
    if(manfd < 0){ // project doesn't exist on server
        printf("this project does not exist on the server\n");
        write(sockfd, "!", 1);
        return -1;
    }
    /*
    int bytesSent = sendfile(sockfd, manfd, 20);
    printf("bytes sent: %d\n", bytesSent);
    perror(errno);
    return 1;
    */

    // turn file into string and send to client

    Manifest* manifest = createManifestStructFromManifestFile(manfd); // create a Manifest struct from .Manifest file
    int i;
    //printf("manifest struct succesfully created\n");
    //printf("manifest num entries: %d\n", manifest->numOfEntries);
    //printf("manifest struct is good\n");
    // find proper size for string containing message for client here
    int strSize = strlen(manifest->version);
    for(i = 0; i < manifest->numOfEntries; i++){
        strSize += strlen(manifest->entries[i].version) + 1 + strlen(manifest->entries[i].path) + 1 + strlen(manifest->entries[i].hash) + 1;
    }

    //printf("string size: %d\n", strSize);

    // proper size found, so now malloc that string
    char* strForClient = (char*)malloc(strSize * sizeof(char));
    //printf("malloc successful\n");

    sprintf(strForClient, "%d:", strSize); // write total bytes to read in at beginning
    
    
    strcat(strForClient, manifest->version);
    strcat(strForClient, "\n");
    
    for(i = 0; i < manifest->numOfEntries; i++){
        //printf("entered loop\n");
        
        //char temp[21];
        //sprintf(temp, "%d:", strlen(manifest->entries[i].path));
        strcat(strForClient, manifest->entries[i].version);
        strcat(strForClient, " ");
        //strcat(strForClient, temp);
        //strcat(strForClient, ":");
        strcat(strForClient, manifest->entries[i].path);
        strcat(strForClient, " ");
        strcat(strForClient, manifest->entries[i].hash);
        //strcat(strForClient, "\n");
        if(i < manifest->numOfEntries - 1){
            strcat(strForClient, "\n");
        }
        //printf("%s\n", strForClient);
        
    }
    //printf("str for client: '%s'\n", strForClient);
    int bytesWritten = 0;
    int bytesToWrite = strlen(strForClient);
    //strForClient[bytesToWrite] = '\0';
    do{
        bytesWritten += write(sockfd, strForClient, bytesToWrite - bytesWritten);
    }while(bytesWritten < bytesToWrite);
    // all versions and paths written in at this point
    
    freeManifestStruct(manifest);
    free(strForClient);
    printf("successfully sent .Manifest to client!\n");
    return 1;
}

int commit(int sockfd, char* projectName){

    char pathToManifest[strlen(projectName) + 14];
    sprintf(pathToManifest, "./%s/.Manifest", projectName);
    int manfd = open(pathToManifest, O_RDONLY, 00644);
    if(manfd < 0){ // project doesn't exist on server or .Manifest for that project doesn't exist for some reason
        write(sockfd, "!", 1);
        printf("project given by client does not exist on server\n");
        return -1;
    }
    // send manifest to client
    char* msgForClient = filesToSocketMessage(&manfd, 1);
    int bytesWritten = 0;
    int bytesToWrite = strlen(msgForClient);
    while(bytesWritten < bytesToWrite){
        bytesWritten += write(sockfd, msgForClient+bytesWritten, bytesToWrite-bytesWritten);
    }
    // manifest sent

    // now, receive .Commit file from client
    int bytesToRead = -2;
    int bytesRead = 0;
    while(bytesToRead < -1){
        bytesToRead = readInIntegerBeforeBytes(sockfd);
    }
    //printf("bytes to read: %d\n", bytesToRead);
    if(bytesToRead == -1){ // .Commit file not getting sent
        printf("error on client side, .Commit file not getting sent.\n");
        return -1;
    }
    sleep(1);
    char* commitStr = readInBytes(sockfd, bytesToRead);
    //printf("commit str: '%s'\n", commitStr);

    // commit contents received, now make a file out of it

    char pathToCommit[strlen(projectName) + 12];
    sprintf(pathToCommit, "./%s/.Commit", projectName);
    int commitfd = open(pathToCommit, O_RDWR | O_CREAT | O_TRUNC, 00644);
    writeToFile(commitfd, commitStr);
    // .Commit file made

    // tell client everything was successful
    write(sockfd, "1", 1);
    
    return 1;


}

int push(int sockfd, char* projectName){

    // check for existence of project by trying to open its manifest and report back to client
    char pathToManifest[strlen(projectName) + 14];
    sprintf(pathToManifest, "./%s/.Manifest", projectName);
    int manfd = open(pathToManifest, O_RDONLY, 00644);
    if(manfd < 0){ // project doesn't exist on server or .Manifest for that project doesn't exist for some reason
        write(sockfd, "!", 1);
        printf("project given by client does not exist on server\n");
        return -1;
    }else{
        write(sockfd, "1", 1);
        printf("told client that this project exists on server. awaiting .Commit file from client\n");
    }
    sleep(1);
    // read in .Commit now
    int bytesToRead = -1;
    do{
        bytesToRead += readInIntegerBeforeBytes(sockfd);
    }while(bytesToRead < 0);
    char garbage = '?';
    char* clientCommitStr = readInBytes(sockfd, bytesToRead+1);
    int y;
    
    read(sockfd, &garbage, 1);
    //clientCommitStr = realloc(clientCommitStr, (bytesToRead + 3) * sizeof(char));
    //strcat(clientCommitStr, "\n");
    // compare this .Commit given by client to .Commit stored on server
    char pathToCommit[strlen(projectName) + 15];
    sprintf(pathToCommit, "./%s/.Commit", projectName);
    int commitfd = open(pathToCommit, O_RDONLY, 00644);
    char* serverCommitStr = fileToString(commitfd);
    //printf("server commit: '%s', client commit: '%s'\n", serverCommitStr, clientCommitStr);
    if(strcmp(clientCommitStr, serverCommitStr) != 0){ // .Commit files are different or there is no .Commit on server
        printf("push failed - .Commit files either dissimilar, or no .Commit file on server\n");
        return -1;
    }

    // copy current project directory into past_versions, and then update current project stuff
    char copyProjectCommand[strlen(projectName)*2+30];
    // find version number of manifest
    char manversion[10];
    char buffer = '?';
    int count = 0;
    while(buffer != '\n'){
        read(manfd, &buffer, 1);
        manversion[count] = buffer;
        count++;
    }
    manversion[count] = '\0';
    sprintf(copyProjectCommand, "cp -r ./%s ./past_versions_%s/%s", projectName, projectName, manversion);
    system(copyProjectCommand);

    // ok, old version stored, now do operations stated in .Commit and alter .Manifest and append to .History

    lseek(manfd, 0, SEEK_SET);
    Manifest* manifest = createManifestStructFromManifestFile(manfd);
    //printf("created manifest struct\n");
    char tag = '?';
    //char commitVersion[10];
    char path[4097];
    char hash[33];
    char version[10];
    count = 0;
    int i = 0;
    //char garbage = '?';
    int k;
    int loopCounter = 0;
    remove("./.ManAdds");
    int manaddsfd = open("./.ManAdds", O_RDWR | O_CREAT | O_APPEND, 00644);
    while(i < strlen(serverCommitStr)){ // loop for all .Commit entries
        // each loop should bring you to beginning of each new line
        tag = serverCommitStr[i];
        //printf("tag: %c\n", tag);
        i = i + 2;
        // read in version num
        while(serverCommitStr[i] != ' '){
            version[count] = serverCommitStr[i];
            count++;
            i++;
        }
        version[count] = '\0';
        count = 0;
        i++;
        // read in path
        while(serverCommitStr[i] != ' '){
            path[count] = serverCommitStr[i];
            count++;
            i++;
        }
        path[count] = '\0';
        count = 0;
        i++;
        // read in hash
        while(serverCommitStr[i] != '\n'){
            hash[count] = serverCommitStr[i];
            count++;
            i++;
        }
        hash[count] = '\0';
        count = 0;
        i++;
        //printf("to search for: version: %s, path: %s, hash: %s\n", version, path, hash);
        //sleep(1);
        //printf("manifest struct stuff: num of entries: %d, first path: %s, second hash: %s\n", manifest->numOfEntries, manifest->entries[0].path, manifest->entries[1].hash);

        // ok, have tag, version, path, hash
        // so now do manipulate/add file, and change manifest struct
        if(tag == 'M'){
            //printf("ok, tag equals M\n");
            // find entry in manifest struct and replace components
            for(k = 0; k < manifest->numOfEntries; k++){
                //printf("path in commit: '%s', path in man: '%s'\n", path, manifest->entries[k].path);
                if(strcmp(path, manifest->entries[k].path) == 0){
                    //printf("version before: %s\n", manifest->entries[k].version);
                    manifest->entries[k].version = version;
                    //printf("version after: %s\n", manifest->entries[k].version);
                    //printf("hash before: %s\n", manifest->entries[k].hash);
                    manifest->entries[k].hash = hash;
                    //printf("hash after: %s\n", manifest->entries[k].hash);
                    break;
                }
            }
            // replaced in manifest, now alter actual files
            //read(sockfd, &garbage, 1);
            int bytesToRead = readInIntegerBeforeBytes(sockfd);
            //printf("bytes to read: %d\n", bytesToRead);
            char* newFileContents = readInBytes(sockfd, bytesToRead);
            read(sockfd, &garbage, 1);
            //printf("garbage: %c\n", garbage);
            //printf("what file will be replaced with: '%s'", newFileContents);
            int fd = open(path, O_RDWR | O_TRUNC | O_CREAT, 00644);
            //printf("opened fd: %d\n", fd);
            //printf("current contents: '%s', being written: '%s'\n", fileToString(fd), newFileContents);
            writeToFile(fd, newFileContents);
            close(fd);
            //free(newFileContents);
            //read(sockfd, &garbage, 1);
            // done, ready for next
        }else if(tag == 'A'){
            // write all adds to temp file then append that file to .Manifest at end
            char addStr[strlen(version) + strlen(path) + strlen(hash) + 10];
            sprintf(addStr, "\n%s %s %s", version, path, hash);
            //printf("addStr: %s\n", addStr);
            writeToFile(manaddsfd, addStr);
            //continue;
            // add entry in manifest struct
            /*
            ManifestEntry* reallocPtr = NULL;
            printf("MANIFEST NUM OF ENTRIES: %d\n", manifest->numOfEntries);
            reallocPtr = realloc(manifest->entries, (manifest->numOfEntries + 1) * sizeof(ManifestEntry));
            if(reallocPtr == NULL){
                printf("Malloc failed when reading in files from client on push\n");
                return 0;
            }else{
                manifest->entries = reallocPtr;
            }
            // increment number of entries
            manifest->numOfEntries = manifest->numOfEntries + 1;
            printf("manifest num of entries now: %d\n", manifest->numOfEntries);
            // insert data into new entry
            //ManifestEntry* newEntry;
            // insert version
            manifest->entries[manifest->numOfEntries].version = (char*)malloc((strlen(version)+1)* sizeof(char));
            int t;
            printf("version im putting in: '%s'\n",version);
            for(t = 0; t < strlen(version); t++){
                manifest->entries[manifest->numOfEntries].version[t] = version[t];
            }
            manifest->entries[manifest->numOfEntries].version[t] = '\0';
            // insert path
            manifest->entries[manifest->numOfEntries].path = (char*)malloc((strlen(path)+1) * sizeof(char));
            printf("path im putting in :'%s'\n", path);
            for(t = 0; t < strlen(path); t++){
                manifest->entries[manifest->numOfEntries].path[t] = path[t];
                printf("%c", manifest->entries[8].path[t]);
            }
            printf("\n");
            manifest->entries[manifest->numOfEntries].path[t] = '\0';
            // insert hash
            manifest->entries[manifest->numOfEntries].hash = (char*)malloc(33 * sizeof(char));
            for(t = 0; t < strlen(hash); t++){
                manifest->entries[manifest->numOfEntries].hash[t] = hash[t];
            }
            manifest->entries[manifest->numOfEntries].hash[t] = '\0';
            */
           //printf("made it here\n");


            //manifest->entries[manifest->numOfEntries].version = newEntry;
            // added to manifest, now create actual file
            printf("about to read in num of bytes\n");
            int bytesToRead = readInIntegerBeforeBytes(sockfd);
            printf("read in num of bytes\n");
            printf("bytes to read: %d\n", bytesToRead);
            char* newFileContents = readInBytes(sockfd, bytesToRead);
            printf("bytes: '%s'\n", newFileContents);
            //printf("read value: %d\n", read(sockfd, &garbage, 1));
            //read(sockfd, &garbage, 1);
            //printf("garbage: %c\n", garbage);
            printf("path: '%s'\n", path);
            int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 00644);
            printf("path: '%s', fd: %d\n", path, fd);
            writeToFile(fd, newFileContents);
            printf("wrote to file\n");
            close(fd);
            //free(newFileContents);
            //read(sockfd, &garbage, 1);
            // done, ready for next
        }else if(tag == 'D'){ // delete file from .Manifest
            for(k = 0; k < manifest->numOfEntries; k++){ // find entry
                if(strcmp(path, manifest->entries[k].path) == 0){
                    // move every entry after it, forward one
                    int j;
                    for(j = k; j < manifest->numOfEntries - 1; j++){
                        manifest->entries[j].version = manifest->entries[j + 1].version;
                        manifest->entries[j].path = manifest->entries[j + 1].path;
                        manifest->entries[j].hash = manifest->entries[j + 1].hash;
                    }
                    manifest->numOfEntries = manifest->numOfEntries - 1;
                    break;
                }
            }
            //remove(path);
        }
        //printf("\n-----\n");
        //printf("### MANIFEST STRUCT: ###\n");
        /*
        int g;
        for(g = 0; g < manifest->numOfEntries; g++){
            printf("-\nentry #%d:\n version: '%s'\npath: '%s'\nhash: '%s'\n-\n", g, manifest->entries[g].version, manifest->entries[g].path, manifest->entries[g].hash);
            printf("##########################\n");
        }
        */
        

    }
    int manVersionInt = atoi(manifest->version);
    manVersionInt++;
    sprintf(manifest->version, "%d", manVersionInt);

    // manifest struct successfully altered, so now write it to new .Manifest file
    //printf("num entires : %d\n", manifest->numOfEntries);
    char* manifestStr = manifestStructToString(manifest);
    //printf("manifest: '%s'\n", manifestStr);
    close(manfd);
    int manfd2 = open(pathToManifest, O_TRUNC | O_RDWR | O_CREAT, 00644);
    writeToFile(manfd2, manifestStr);
    close(manfd2);
    int manfd3 = open(pathToManifest, O_RDWR | O_APPEND, 00644);
    lseek(manaddsfd, 0, SEEK_SET);
    char* addStrs = fileToString(manaddsfd);
    writeToFile(manfd3, addStrs);
    close(manaddsfd);
    close(manfd3);
    remove("./.ManAdds");

    // append to .History
    char pathToHistory[strlen(projectName) + 14];
    sprintf(pathToHistory, "./%s/.History", projectName);
    int historyfd = open(pathToHistory, O_RDWR | O_APPEND | O_CREAT, 00644);
    char historyStr[strlen(manversion) + strlen(serverCommitStr) + 40];
    sprintf(historyStr, "\n-------------------\nVersion: %s\n%s\n", manversion, serverCommitStr);
    writeToFile(historyfd, historyStr);
    close(historyfd);
    // appended to .History
    
    return 1;
    

}


char garbageCollector[500];
char client_message[256];
char buffer[1024];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;



void * socketThread(void *arg)
{
    //printf("socketThread function called\n");
    int newSocket = *((int *)arg);
    //recv(newSocket , client_message , 2000 , 0);
    read(newSocket,client_message,255);
    printf("command received from client: '%s'\n", client_message);
    int status = 0;
    int numOfBytes = 0;
    // Send message to the client socket 
    pthread_mutex_lock(&lock);
        if(strcmp(client_message,"checkout")==0){

            int numOfBytesToRead = 0;
            do{// loop until the client writes to server -- first thing written will be the number of bytes to read in
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead < 1); // will break out of loop once number of bytes to read is obtained
            char* projectName = readInBytes(newSocket, numOfBytesToRead);  // client will send name of project that it wants, and it will be read by server here
            printf("project name given by client: '%s'\n", projectName);
            checkout(newSocket, projectName);
            free(projectName);
            printf("checkout successful!\n");

        }
        else if(strcmp(client_message,"update")==0){

            int numOfBytesToRead = 0;
            do{// loop until the client writes to server -- first thing written will be the number of bytes to read in
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead < 1); // will break out of loop once number of bytes to read is obtained
            char* projectName = readInBytes(newSocket, numOfBytesToRead);  // client will send name of project that it wants, and it will be read by server here
            printf("project name given by client: '%s'\n", projectName);
            update(newSocket, projectName);
            free(projectName);

        }else if(strcmp(client_message,"upgrade")==0){
            
            //printf("Here is the message from client:%s\n",client_message);
            int numOfBytesToRead = 0;
            do{
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead<1);
            char* updateFile = readInBytes(newSocket,numOfBytesToRead);
            printf("THIS IS THE UPDATE FILE CLIENT SENT US:\n%s",updateFile);
            int update = open("./.Update",O_CREAT | O_RDWR | O_TRUNC, 00644);
            write(update,updateFile,strlen(updateFile));
            close(update);
            update = open("./.Update",O_RDONLY | O_NONBLOCK);
            Update* updateSystem = createUpdateStructureFromFile(update);
            close(update);
            //Created structure from the update file
            
            int i = 0;
            for(i = 0;i<updateSystem->numOfEntries;i++){
                printf("THIS IS IN THE STRUCTURE:%s\n",updateSystem->entries[i].path);
            }
            //now that we have everything in the structure we have to  use fileToString
            i = 0;
            for(i = 0; i<updateSystem->numOfEntries;i++){
                printf("This is the command:%s\n",updateSystem->entries[i].version);
                if(strcmp(updateSystem->entries[i].version,"A")==0 || strcmp(updateSystem->entries[i].version,"M")==0){
                //need a file descriptor
                int fd = open(updateSystem->entries[i].path, O_RDONLY | O_NONBLOCK);
                char* fileToClient = fileToString(fd);
                printf("THIS IS WHAT WE WILL SEND TO CLIENT: %s\n",fileToClient);
                char length[strlen(fileToClient)+200];
                sprintf(length,"%d:",strlen(fileToClient));
                SendToClient(newSocket,length);
                SendToClient(newSocket,fileToClient);
                }
            }
            remove("./.Update");

        }else if(strcmp(client_message,"commit")==0){

            int numOfBytesToRead = 0;
            do{// loop until the client writes to server -- first thing written will be the number of bytes to read in
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead < 1); // will break out of loop once number of bytes to read is obtained
            char* projectName = readInBytes(newSocket, numOfBytesToRead);  // client will send name of project that it wants, and it will be read by server here
            printf("project name given by client: '%s'\n", projectName);
            commit(newSocket, projectName);
            free(projectName);

        }else if(strcmp(client_message,"push")==0){


            int numOfBytesToRead = 0;
            do{// loop until the client writes to server -- first thing written will be the number of bytes to read in
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead < 1); // will break out of loop once number of bytes to read is obtained
            char* projectName = readInBytes(newSocket, numOfBytesToRead);  // client will send name of project that it wants, and it will be read by server here
            printf("project name given by client: '%s'\n", projectName);
            int success = push(newSocket, projectName);
            if(success == 1){
                printf("push successful\n");
                writeToFile(newSocket, "1");
            }else{
                printf("push failed\n");
                writeToFile(newSocket, "!");
            }

            free(projectName);


        }else if(strcmp(client_message,"create")==0){
            
            int numOfBytesToRead = 0;
            do{
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead<1);
            char* projectName = readInBytes(newSocket,numOfBytesToRead);
            int false = 0;
            printf("project name given by client:'%s'\n",projectName);//NOW WE HAVE THE PROJECT NAME
            if(isProjectInDirectory("./",projectName)==1){
                printf("Project already exists on server - create failed, let client know\n");
                false = 1;
                char length[22];
                sprintf(length,"%d:",1);
                SendToClient(newSocket,length);
                SendToClient(newSocket,"!");
            }
            if(false ==0){
            create(newSocket,projectName);
            }
            char dirName[strlen(projectName) + 20];
            sprintf(dirName, "./past_versions_%s", projectName);
            mkdir(dirName, 0777); // create dir to store past versions in
            //printf("Here is the message from client:%s\n",client_message);
        }else if(strcmp(client_message,"destroy")==0){

            int numOfBytesToRead = 0;
            do{
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead<1);
            char* projectName = readInBytes(newSocket,numOfBytesToRead);
            printf("Project name given by client: %s\n",projectName);
            struct dirent *pDirent;
            DIR *pDir;
            pDir = opendir("./");
            if(pDir==NULL){
                printf("Directory doesnt exist \n");
            }
            int truth = 0;
            while((pDirent = readdir(pDir))!=NULL){
                printf("%s\n",pDirent->d_name);
                if(strcmp(pDirent->d_name,projectName)==0){
                    truth = 1;
                    break;
                }
                else{
                    truth = 0;
                }
            }
            char pastVersionDir[strlen(projectName)+35];
            sprintf(pastVersionDir, "./past_versions_%s", projectName);
            
            //Now we know whether or not the project is in the file
            if (truth == 1){
                //run remove command here and send back a success message
                removeDIR(projectName);
                removeDIR(pastVersionDir);
                printf("Destroy succesful, sending message to client\n");
                char message[200] = "Destroy is a success";
                char length[22];
                sprintf(length,"%d:",strlen(message));
                SendToClient(newSocket,length);
                SendToClient(newSocket,message);
            }
            else if (truth ==0){
                //send client message that it has failed
                printf("Project not found on server, sending message to client\n");
                char message[200] = "Destroy has failed";
                char length[22];
                sprintf(length,"%d:",strlen(message));
                SendToClient(newSocket,length);
                SendToClient(newSocket,message);
            }

        }else if(strcmp(client_message,"currentversion")==0){
            
            int numOfBytesToRead = 0;
            do{// loop until the client writes to server -- first thing written will be the number of bytes to read in
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead < 1); // will break out of loop once number of bytes to read is obtained
            char* projectName = readInBytes(newSocket, numOfBytesToRead);  // client will send name of project that it wants, and it will be read by server here
            printf("project name given by client: '%s'\n", projectName);
            currentversion(newSocket, projectName);
            free(projectName);

        }else if(strcmp(client_message,"history")==0){ //######################################## H I S T O R Y ######################

            printf("History starting\n");
            int numOfBytesToRead = 0;
            do{
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead<1);
            char* projectName = readInBytes(newSocket,numOfBytesToRead);
            printf("Project name given by client: %s\n",projectName);
            //must now create a path.
            char path[4096] = "./";
            strcat(path,projectName);
            strcat(path,"/.History");
            //printf("This is path to .History:%s\n",path);
            int history = open(path,O_RDONLY | O_NONBLOCK);
        
            char* historyContents = fileToString(history);
            char length[strlen(historyContents)+200];
            sprintf(length,"%d:",strlen(historyContents));
            SendToClient(newSocket,length);
            SendToClient(newSocket,historyContents);
            printf("history successful\n");

        }else if(strcmp(client_message,"rollback")==0){ // ##################################### R O L L B A C K #################333
            printf("executing rollback\n");
            
            char buffer = '?';
            int status = 1;
            while(status > 0){
                status = read(newSocket, &buffer, 1);
                printf("%c", buffer);
            }
            
            int numOfBytesToRead = 0;
            do{// loop until the client writes to server -- first thing written will be the number of bytes to read in
                numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            }while(numOfBytesToRead < 1); // will break out of loop once number of bytes to read is obtained
            char* projectName = readInBytes(newSocket, numOfBytesToRead);  // client will send name of project that it wants, and it will be read by server here
            printf("project name given by client: '%s'\n", projectName);
            
            // now read in desired version number
            numOfBytesToRead = readInIntegerBeforeBytes(newSocket);
            char* version = readInBytes(newSocket, numOfBytesToRead);
            printf("version number given by client: %s\n", version);

            char pastVersionsProjectPath[strlen(projectName) + 20];
            sprintf(pastVersionsProjectPath, "./past_versions_%s", projectName);

            if(isProjectInDirectory("./", projectName) < 1){
                printf("project given by client does not exist on server\n");
                write(newSocket, "!", 1);
            }else if(isProjectInDirectory(pastVersionsProjectPath, version) < 1){
                printf("version number for project given by client does not exist on server\n");
                write(newSocket, "!", 1);
            }else{ // remove current project dir and replace with past one
                char removeDirectoryCommand[strlen(projectName) + 20];
                sprintf(removeDirectoryCommand, "rm ./%s", projectName);
                char copyDirectoryCommand[(2*strlen(projectName)) + strlen(version) + 40];
                sprintf(copyDirectoryCommand, "cp -r ./past_versions_%s/%s ./%s", projectName, version, projectName);
                system(removeDirectoryCommand);
                system(copyDirectoryCommand);
                printf("rollback successful\n");
                write(newSocket, "1", 1);
            }
            
        }
        //write(newSocket,client_message,255);
        pthread_mutex_unlock(&lock);
        /*
        status = 0;
        do{
            status = read(newSocket, garbageCollector, 500);
        }while(status > 0);
        */
        int a;
        for(a = 0; a < 256; a++){
            client_message[a] = '\0';
        }
  //send(newSocket,buffer,13,0);
        printf("disconnecting from client\n");
        close(newSocket);
        pthread_exit(NULL);
}
int main(int argc, char* argv[]){

    if(argc < 2){
        printf("you must include a port number to listen on...exiting\n");
        return 0;
    }

    if(argc > 2){
        printf("too many arguments...exiting\n");
        return 0;
    }

    if( atoi(argv[1]) < 5000 || atoi(argv[1]) > 65536 ){
        printf("you should probably use a port number from 5000 to 65536 since the system typically doesn't use these ports for important things but program will continue anyway\n");
    }

    /*
    int manfd = open("./client/project1/.Manifest", O_RDONLY);
    Manifest* manifest = createManifestStructFromManifestFile(manfd);
    freeManifestStruct(manifest);
    return;
    */

   /*
   int fd = open("./test.txt", O_RDONLY);
   printf("%s\n", fileToString(fd));
   return;
   */
    


    int serverSocket;
    int newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    //Create the socket. 
    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    // Configure settings of the server address struct
    // Address family = Internet 
    serverAddr.sin_family = AF_INET;
    //Set port number, using htons function to use proper byte order 
    serverAddr.sin_port = htons(atoi(argv[1]));
    //Set IP address to localhost
    serverAddr.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");INADDR_ANY;
    //Set all bits of the padding field to 0 
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    //Bind the address struct to the socket 
    if(bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))<0){
        error("ERROR on binding");
    }
    //Listen on the socket, with 40 max connection requests queued
    if(listen(serverSocket,50)==0)
        printf("Listening\n");
    else
        printf("Error\n");
        pthread_t tid[60];
        int i = 0;
        while(1)
        {
            //Accept call creates a new socket for the incoming connection
            addr_size = sizeof serverStorage;
            newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
            printf("new client connected!\n");
       

            //for each client request creates a thread and assign the client request to it to process
            //so the main thread can entertain next request
            if( pthread_create(&tid[i], NULL, socketThread, &newSocket) != 0 )
                printf("Failed to create thread\n");
            if( i >= 50)
            {
                i = 0;
                while(i < 50)
                {
                    pthread_join(tid[i++],NULL);
                }
                i = 0;
            }
                    
        }
    return 0;
}