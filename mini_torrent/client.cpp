#include"extra.h"
using namespace std;
#define BUFFER_SIZE 512*1024




/*.............................................Ports...................................................................*/

vector<int> collect_ports(char *answer){
    vector<string> tokens;
    string resp(answer);
    int s;
    boost::split(tokens, resp, boost::is_any_of(" "));
    vector <int> connection_ports;

    for(int i=0;i<tokens.size()-2;i++){
        connection_ports.push_back(atoi(tokens[i].c_str()));
        s=s*i;
    }

    return connection_ports;
}

//2nd function...

vector<string> collect_sha_From(char *answer){
    vector<string> tokens;
    int s;
    string resp(answer);
    vector<string> s_h_a;
    boost::split(tokens, resp, boost::is_any_of(" "));
    int token_size = tokens.size();
    s=0;
    for(int i=token_size-2;i<token_size;i++){
        s=s*i;
        s_h_a.push_back(tokens[i]);
    }
    
    return s_h_a;
} 


/*.............................................SHA...................................................................*/
vector<string> collect_sha(string location){

    FILE *fp = fopen(location.c_str(),"r+");
    if(fp==NULL){
        exit(1);
    }
    vector<string> final_return_result;

    char hash_little[40];

    fseek(fp,0, SEEK_END);
    int total_size = ftell(fp);
    rewind(fp);

    unsigned char buffer[BUFFER_SIZE];
    
    unsigned char hash1[20];
    
    char partial[40];
    
    string block_string="";
    
    int n;
    
    while( (n=fread(buffer,1,sizeof(buffer),fp))>0 && total_size>0 ){
       
        total_size = total_size-n;
       
        SHA1(buffer, n, hash1);
       
        bzero(buffer, BUFFER_SIZE); 
       
        for(int i=0;i<5;i++){
            sprintf(partial+2*i,"%02x",hash1[i]);
        }

        block_string =block_string + partial;
    }
    
    final_return_result.push_back(block_string);

    //whole file sha1
    unsigned char final_hash[20];
    SHA1((unsigned char *)block_string.c_str(), block_string.size(), final_hash);
    for(int i=0;i<20;i++){
        sprintf(hash_little+i*2,"%02x",final_hash[i]);
    }
    final_return_result.push_back(string(hash_little));
    return final_return_result;
}


/*****************************************CLIENT Work******************************************************************/
void *RequestThread(void *newsockfd1){
    
    // this char c is for receving file name from client
    char buff[BUFFER_SIZE], data[BUFFER_SIZE];
    int numbers;
    int newsockfd = *((int*)newsockfd1);
    
    int val = recv(newsockfd, data, sizeof(data),0);
    //cout<<val;
    cout<<"File Name is :"<<data<<endl;
    // by opening the file name received from client we are checking if file is present
    // in server or not

    string fi=string(data);
    FILE *fp = fopen(fi.c_str(), "r+");

    if(fp == NULL){
        perror("file does not exist");
        pthread_exit(NULL);
    }

    cout << "\033[1;33;40m" << "File opened, now sending file" << "\033[0m";
    cout<<endl;
    
    // moving pointer of file to calculate file size
    fseek(fp, 0, SEEK_END);
    long long size = ftell(fp);
    rewind(fp);

    // sending file size to client
    //send(newsockfd, &size, sizeof(size), 0);

    // sending file to client by reading in chunks
    int count=0;
    while( (numbers = fread(buff, sizeof(char), BUFFER_SIZE, fp))> 0){
        cout<<"Date read in one iteration = "<<numbers<<endl;
        send(newsockfd, buff, numbers, 0);
        bzero(buff, sizeof(buff));
        count++;
    }
    
    cout << "\033[1;33;40m" << "File Downloaded" << "\033[0m";
    cout<<endl;

    fclose(fp);
    close(newsockfd);
}

void *ThreadServerProgram(void *arg){

    // parsing of argument containing the ip & port
    char *arr = (char*)arg;
    string argument_input(arr);
    vector<string> arguments;
    boost::split(arguments, argument_input, boost::is_any_of(" "));

    // variables used in program
    int sockfd, new_sockfd, portno, n;
    char buff[BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    portno = atoi(arguments[1].c_str());
    bzero((char *) &serv_addr, sizeof(serv_addr));

    // socket creation at server side
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){
        printf("socket opening failure");
        exit(1);
    }
    
    // assigning ip & port in structure
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(arguments[0].c_str());

    // socket binding
    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){
        perror("Binding failed, incorrect ip or port");
    }

    // socket listening
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    pthread_t thread1;

    while(1){

        // socket accepts, once accepts create new socket for read & write
        printf("waiting for connections\n");
        int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if(newsockfd<0){
            perror("error on accept\n");
        }
        printf("connected....\n");

        // argument parsing of newsockfd
        int *arg = (int*)malloc(sizeof(*arg));
        *arg = newsockfd;

        // create new thread whenever a request for new connection comes in.
        pthread_create(&thread1, NULL, RequestThread, (void*)arg);

    }
    pthread_exit(NULL);
}


void receiveFile(int port_no, string dest, vector<string> shas, string fname){
    
    int n,fsize;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buff[BUFFER_SIZE];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd<0){
        perror("error opening socket\n");
    }

    server = gethostbyname("127.0.0.1");
    if(server == NULL){
        fprintf(stderr, "Error, no such host\n");
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));

    // assign serverip & port into the structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(port_no);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){
        perror("connection failed to server\n");
    }

    // sending file_name to another peer to fetch filename
    send(sockfd, fname.c_str(), strlen(fname.c_str()), 0);
    //recv(sockfd, &fsize, sizeof(fsize),0);

    cout<<"file_size="<<fsize<<endl;

    FILE *fp = fopen(dest.c_str(), "w+");
    if(fp==NULL){
        perror("Not able to create file");   
    }

    int count = 0;
    // recieving file by reading in chunks
    while((n = recv(sockfd, buff, BUFFER_SIZE, 0))>0){
        cout<<"n=="<<n<<endl;
        fwrite(buff, sizeof(char), n, fp);
        bzero(buff, sizeof(buff));
        count++;
    }
    cout<<count<<endl;
    fclose(fp);

}


int command_size_check(vector<string> &v, unsigned int min_size, unsigned int max_size, string error_msg)
{
    if(v.size() < min_size || v.size() > max_size)
    {

            cout << "\033[1;31m" << error_msg << "\033[0m";

        cout<<endl;
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) 
{ 

    if(argc < 4){

cout<< "\033[1;31;40m" << "Arguments not proper..Please input right number of arguments.." <<endl<< "\033[0m";
cout<< "\033[1;31;40m" << "Program Will EXIT now..." << "\033[0m";
cout<<endl;

        exit(1);
    }

    //argv[1] ip address in which its own server will run
    // argv[2] port address in which its own server will run
    // argv[3] it will tracker_info.txt which will contains tracker's details
    vector<string> ports, IP;
    ports  = getTrackerPort(argv[3]);
    IP = getTrackerIP(argv[3]);

    pthread_t thread3;

    string s = "";
    s+=argv[1];
    s+=" ";
    s+=argv[2];

    // create thread for running server in this port
    pthread_create(&thread3, NULL, ThreadServerProgram, (void*)s.c_str());


    // Client Program for Peer starts here
    // it will connect to tracker
    //variables used in program
    string fname;
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buff[BUFFER_SIZE];
    FILE *fp;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd<0){
        perror("error opening socket\n");
    }

    server = gethostbyname(IP[0].c_str());

    if(server == NULL){
        fprintf(stderr, "Error, no such host\n");
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = atoi(ports[0].c_str());

    // assign serverip & port into the structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP[0].c_str());
    serv_addr.sin_port = htons(portno);


    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){
        perror("connection failed to server\n");
    }
        int q=0;

    // client request_type or command is asked from user 
    while(true){
        // this is for connecting to tracker
        char request[BUFFER_SIZE]; 
        char response[BUFFER_SIZE];
        if(q==0){
           cout << "\033[1;33;40m" << "Enter Appropriate Commands.." << "\033[0m" << " "<<endl;
        }
        else 
           cout << "\033[1;33;40m" << "Continue with your Commands.." << "\033[0m" << " "<<endl;


        q=1;


        getline(cin,fname);
        
        int len = fname.length();
        
        for(int i=0;i<len;i++){
            request[i] = fname.at(i);
        }

        vector<string> str = request_type_command(request);
        if(str[0]=="upload_file"){
            
            if(0 == command_size_check(str, 3, 3, "Upload: (usage):- \"upload_file   <directory/file_path>   <group_name>\""))
                continue;

            string filePath = getFilePath(request);
            vector<string> SHA_one = collect_sha(filePath);
            string final_request(request);
            final_request.append(" ");
            final_request.append(SHA_one[0]);
            final_request.append(" ");
            final_request.append(SHA_one[1]);
            final_request.append(" ");
            final_request.append(argv[2]);
            final_request.append(" ");
            final_request.append(argv[1]);
            bzero(request, BUFFER_SIZE);
            for(int i=0;i<final_request.length();i++){
                request[i] = final_request.at(i);
            }
        }
        write(sockfd, request, strlen(request));
        memset(request, '\0', sizeof(request));
        memset(response, '\0', sizeof(response));
        read(sockfd, response, sizeof(response));
        printf("%s\n", response);

        if(str[0]=="download_file"){

            
            if(0 == command_size_check(str, 4, 4,"Download: (usage):- \"download_file    <group_name>    <file_name>     <destination_path>\""))
                continue;

            if(strcmp(response,"you cannot download, first join the group")!=0){
            vector<int> ports = collect_ports(response);

            vector<string> shas = collect_sha_From(response);
            pthread_t thread1;
            int no_of_ports = ports.size();
            int i=0;
            while(no_of_ports>0){
                // here implement multiple thread for downloading files
                // cout<<"FileName--"<<str[2]<<endl;
        
                string dest = str[3];
                dest.append("/");
                dest.append(str[2]);
                receiveFile(ports[i], dest, shas, str[2]);
                i++;
                no_of_ports--;
            }
            }
        }
    }

    close(sockfd); 
    return 0; 
} 