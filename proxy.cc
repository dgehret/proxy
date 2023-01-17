#include <stdio.h>
#include <iomanip>
#include <csignal>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctime>
#include <chrono>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <openssl/bio.h> 
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <memory>
#include <cstdlib>
#include <string.h>
#include <utility>


#define MAXLEN 320000
#define TIMEOUT 5

using namespace std::chrono;
using namespace std;


void sighandler(int);

// Used to grab the forbidden sites from a file
void parseConfig(string file_path, vector<string> &vec) {

	ifstream infile;
	infile.open(file_path);

	string str;
	while (getline(infile, str)) {

		if (str[0] == '#') continue;

		vec.push_back(str);

	}
	cerr << "addresses:" << endl;
	for (int i = 0; i < vec.size(); i++) {
		cerr << vec.at(i) << endl;
	}

	infile.close();
}

// Gets the time, formats it in rfc3339
string timeNow() {
	//https://stackoverflow.com/questions/54325137/c-rfc3339-timestamp-with-milliseconds-using-stdchrono
	const auto now_ms = time_point_cast<milliseconds>(system_clock::now());
	const auto now_s = time_point_cast<seconds>(now_ms);
	const auto millis = now_ms - now_s;
	const auto c_now = system_clock::to_time_t(now_s);

	stringstream ss;
	ss << put_time(gmtime(&c_now), "%FT%T")
		<< '.' << setfill('0') << setw(3) << millis.count() << 'Z';
	return ss.str();
}

// prints log information
void printLog(string data, int status_code) {
	cout << timeNow() << "\"" << data << "\"" << status_code << endl;
}

// Receive from the SSL connection
//	used to receive responses after sending requests to web servers
void receive(SSL *servssl, char *outfile_path) {
	int len = 10000;
	char buffer[MAXLEN];


	fstream outfile;
	outfile.open(outfile_path, ios_base::out | ios::binary);



	while (len > 0) {

		len=SSL_read(servssl, buffer, 10000);
		buffer[len]=0;
		outfile << buffer;
	} 
	//cerr << endl;
	outfile.close();

}

// Write over the SSL connection
//	used for sending requests to web servers
void send(const char *buffer, SSL *servssl) {
	SSL_write(servssl, buffer, strlen(buffer));
}

void sighandler(int temp) {
	cerr << "-----------SIGINT RECEIVED--------" << endl;
	//sendPacket(request);
	//receivePacket();
}

// Receive and store the wget request from the client
void receiveClient(int sockfd, sockaddr *pcliaddr, socklen_t clilen) {
	int n;
	socklen_t len;
	char mesg[MAXLEN];

	struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));


	for ( ; ; ) {
		len = clilen;
		n = recvfrom(sockfd, mesg, MAXLEN, 0, pcliaddr, &len);
		cerr << mesg << endl;

		// Check for timeout
		if (n < 0) {
			if (errno == EINTR) {
				break;  // waited long enough
			} else if (errno == EWOULDBLOCK) {
				cerr << "Connection timed out" << endl;
				break;
			} else {
				cerr << "recvfrom error\n";
			}
		}

	}

}


int main(int argc, char *argv[]) {

	// Parse info from command line, fill forbidden sites
	int clientfd;
	struct sockaddr_in cliaddr;

	int listen_port = atoi(argv[1]);
	char *forbidden_sites_file_path = argv[2];
	char *access_log_file_path = argv[3];

	string forbidden_sites_filename = argv[2];

	vector<string>forbidden_sites;
	parseConfig(forbidden_sites_filename, forbidden_sites);



	// Create client socket, receive request from command line
	clientfd = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr = (INADDR_ANY);
	cliaddr.sin_port = htons(listen_port);

	bind(clientfd, (sockaddr *) &cliaddr, sizeof(cliaddr));

	receiveClient(clientfd, (sockaddr *) &cliaddr, sizeof(cliaddr));



	// Parse request from client
	char request[1000];
	
	strcpy(request, "GET https://www.example.com/ HTTP/1.0\r\n\r\n");

	cerr << request << endl;

	char *rest;
	rest = (char *)malloc(1000);
	strcpy(rest, request);
	char *token;
	token = strtok_r(rest, ".", &rest);
	char *hostname = strtok(rest, "./");
	strncat(hostname, ".com", 5);
	cerr << hostname << endl;


	// Get IP address from hostname parsed from command line
	struct hostent *host = gethostbyname(hostname);
	struct in_addr ip_addr = *(struct in_addr *)(host->h_addr);
	cerr << inet_ntoa(ip_addr)<< endl;




	// Declare variables for server connection
	int servfd;
	servfd = socket(AF_INET, SOCK_STREAM, 0);

	if (servfd < 0) {
		cerr << "Error creating socket.\n";
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	
	// Initialize proxy/server connection 
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(inet_ntoa(ip_addr));
	servaddr.sin_port = htons (443);
	socklen_t socklen = sizeof(servaddr);
	if (connect(servfd, (struct sockaddr *)&servaddr, socklen)) {
		cerr << "Error connecting to webserver.\n";
		exit(EXIT_FAILURE);
	}

	SSL_library_init();
	SSL_load_error_strings();
	
	const SSL_METHOD *method = SSLv23_client_method();
	SSL_CTX *ctx = SSL_CTX_new(method);
	SSL *servssl = SSL_new(ctx);

	int sock = SSL_get_fd(servssl);
	SSL_set_fd(servssl, servfd);
	if (SSL_connect(servssl) <= 0) {
		cerr << "Error establishing secure connection.\n";
		exit(EXIT_FAILURE);
	}


	//signal(SIGINT, sighandler);

	send(request, servssl);
	receive(servssl, argv[3]);
	cerr << "Check outfile for file requested." << endl;

	// Cleanup
	SSL_shutdown(servssl);
	SSL_free(servssl);
	SSL_CTX_free(ctx);


	return 0;
}

