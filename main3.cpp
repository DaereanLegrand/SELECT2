/*
** selectserver.c -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <vector>
#include <fstream>
#include <map>
#include <queue>

#define PORT "9034"   // port we're listening on

fd_set master;    // master file descriptor list
fd_set read_fds;  // temp file descriptor list for select()

std::map<int, std::queue<char *>> mymap;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void
reader(int i)
{
    /*
    int nbytes;
    FILE *mfile;
    char nfile[256] = "file";
    strcat(nfile, ".txt");
    mfile = fopen(nfile, "w");
    fprintf(mfile, "HOLA %s", nfile);

    char id[10];
    char sz[8];
    char data[100];
    char chsum[11];

    char buf2[100000];    // buffer for client data
    char *buf3 = strdup(mymap[i].front());

    char *token = strtok(buf3, "\n");
    while (!mymap[i].empty()) {
        while (token != NULL) {
            //printf("%s\n", token);
            char *tkn = strtok(token, ":");
            while (tkn != NULL) {
                printf("%s ", tkn);
                fprintf(mfile, "%s ", tkn);
                tkn = strtok(NULL, ":");
            }
            printf("\n");
            fprintf(mfile, "\n");
            token = strtok(NULL, "\n");
        }
        mymap[i].pop();
        buf3 = strdup(mymap[i].front());
    }
    fclose(mfile);
    */
}

int main(void)
{
    char buf[100000];    // buffer for client data
                          //
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    int nbytes;

	char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

	struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
    	listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) { 
			continue;
		}
		
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    std::vector<std::thread> thrds;

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
					newfd = accept(listener,
						(struct sockaddr *)&remoteaddr,
						&addrlen);

					if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
							inet_ntop(remoteaddr.ss_family,
								get_in_addr((struct sockaddr*)&remoteaddr),
								remoteIP, INET6_ADDRSTRLEN),
							newfd);
    
                        //mymap[newfd].push("");
                        //thrds.push_back(std::thread(reader, newfd));
                    }
                } else {
                    int maxR = 65000;
                    int n = read(i, buf, maxR);
                    buf[maxR] = '\0';
                    mymap[i].push(strdup(buf));
                    if (n > 0)
                        printf("%d", strlen(buf));
                    //thrds.push_back(std::thread(reader, i));
                    //printf("HELLO\n");
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors

            for (auto &i: thrds) {
                i.join();
            }
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}

