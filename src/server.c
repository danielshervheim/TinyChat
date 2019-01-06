//
// 2018-2019 Daniel Shervheim
// danielshervheim@gmail.com
// github.com/danielshervheim
//

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// network includes
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"



//
// USER_LIST function(s)
//

// the user struct
struct user {
    char username[MAX_NAME_LEN];
    int write_to_child;
    int read_from_child;
    int taken;
};

// initialize user list
void user_list_initialize(struct user *user_list) {
    for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
        strcpy(user_list[i].username, "");
        user_list[i].write_to_child = -1;
        user_list[i].read_from_child = -1;
        user_list[i].taken = 0;
    }
}

// removes the user at position i from the list
void user_list_remove_user(struct user *user_list, int i) {
    if (i >= 0 && i < MAX_CONCURRENT_USERS) {
        strcpy(user_list[i].username, "");
        close(user_list[i].write_to_child);
        user_list[i].write_to_child = -1;
        close(user_list[i].read_from_child);
        user_list[i].read_from_child = -1;
        user_list[i].taken = 0;
    }
}

// returns an empty position, i, or -1 if the server is full
int user_list_get_free_index(struct user *user_list) {
    for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
        if (user_list[i].taken == 0) {
            return i;
        }
    }
    return -1;
}

// returns the index of the user "username", or -1
int user_list_get_index_by_username(struct user *user_list, char *username) {
    for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
        if (user_list[i].taken == 1) {
            if (strcmp(user_list[i].username, username) == 0) {
                return i;
            }
        }
    }
    return -1;
}

// send the userlist to all currently connected users
void broadcast_user_list(struct user *user_list) {
    // generate the userlist...
    char tmp[BUF_SIZE];
    strcpy(tmp, "/userlist");
    for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
        if (user_list[i].taken == 1) {
            strcat(tmp, " ");
            strcat(tmp, user_list[i].username);
        }
    }

    // ...then write it to all connected users
    for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
        if (user_list[i].taken == 1) {
             if (write(user_list[i].write_to_child, tmp, strlen(tmp)) < 0) {
             	perror("write() failed in broadcast_user_list()");
             }
        }
    }
}



//
// USER_DAEMON acts as a relay between the server process and the user's client
//
void user_daemon(int write_to_server, int read_from_server, int socket_fd) {
    while (1) {
        char server_buf[BUF_SIZE];
        memset(server_buf, '\0', BUF_SIZE);
        ssize_t server_nread;

        // if there is data to read from the server
        if ((server_nread = read(read_from_server, server_buf, BUF_SIZE)) != -1) {
            if (server_nread == 0) {
            	// the connection to the server was lost
                close(write_to_server);
                close(read_from_server);
                close(socket_fd);
                exit(1);
            }
            else if (write(socket_fd, server_buf, server_nread) < 0) {
               	perror("write() failed in user_daemon()");
            }
        }

        char client_buf[BUF_SIZE];
        memset(client_buf, '\0', BUF_SIZE);
        ssize_t client_nread;

        // if there is data to read from the socket
        if ((client_nread = read(socket_fd, client_buf, BUF_SIZE)) != -1) {
            if (client_nread == 0) {
            	// the connection to the client was lost
                close(write_to_server);
                close(read_from_server);
                close(socket_fd);
                exit(1);
            }
            else if (write(write_to_server, client_buf, client_nread) < 0){
            	perror("write() failed in user_daemon()");
            }
        }

        usleep(MICRO_SLEEP_DUR);
    }
}



//
// MAIN launches the server then checks for incoming messages from user daemons,
// reformatting and distributing them as appropriate
//
int main(int argc, char* argv[]) {
    // verify that the number of arguments are correct
    if (argc < 2) {
        printf("usage: %s <port>\n", argv[0]);
        exit(-1);
    }

    // verify that the port is within the correct range
    int port = atoi(argv[1]);
    if (port < 1024 || port > 65535) {
        printf("invalid port range\n");
        exit(-1);
    }

    // initialize the socket
    // note: socket needs to be non-blocking as we "accept" new connections every iteration
    int sock_fd; 
    if ((sock_fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
        perror("socket() failed");
        exit(-1);
    }

    // set the socket to allow reuse of the same address
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(-1);
    }

    // create an address structure based on the port
    // note: INADDR_ANY binds to any local ip address (typically there is only one,
    // unless the host has multiple wifi cards, or wifi+ethernet)
    struct sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    // bind the socket to the address structure
    if (bind(sock_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        exit(-1);
    }

    // set the socket to listen for incoming connections
    if (listen(sock_fd, MAX_CONCURRENT_USERS) < 0) {
        perror("listen() failed");
        exit(-1);
    }

    // initialize the user_list data structure
    struct user user_list[MAX_CONCURRENT_USERS];
    user_list_initialize(user_list);

    // begin the main server loop
    while (1) {
        int incoming_fd;

        // a user is trying to connect
        if ((incoming_fd = accept(sock_fd, NULL, NULL)) != -1) {
            
        	// temporary buffer to hold handshake information
            char handshake_buf[BUF_SIZE];
            memset(handshake_buf, '\0', BUF_SIZE);
            ssize_t handshake_nread;

            // wait for handshake from user
            if ((handshake_nread = read(incoming_fd, handshake_buf, BUF_SIZE)) <= 0) {
                perror("read() failed");
                close(incoming_fd);
            }
            else if (memcmp(handshake_buf, "/join", strlen("/join")) != 0) {
                printf("user did not send /join command as expected\n");
                close(incoming_fd);
            }
            else {
            	// handshake_buf is of format: /join <username>
                char* username = handshake_buf + strlen("/join") + 1;
                int index_to_add;

                // check if we are able to add the user to the userlist
                if (user_list_get_index_by_username(user_list, username) != -1) {
                    if (write(incoming_fd, "/joinresponse username_taken", strlen("/joinresponse username_taken")) < 0) {
                    	perror("write() failed while responding to join request");
                    }
                    close(incoming_fd);
                }
                else if ((index_to_add = user_list_get_free_index(user_list)) < 0) {
                    if (write(incoming_fd, "/joinresponse server_full", strlen("/joinresponse server_full")) < 0) {
                    	perror("write() failed while responding to join request");
                    }
                    close(incoming_fd);
                }
                else {
                    // attempt to add the user to the userlist
                    int child_to_server[2], server_to_child[2];
                    pid_t user_daemon_pid = -1;

                    if (pipe(child_to_server) < 0 || pipe(server_to_child) < 0) {
                        perror("pipe() failed");
                        close(incoming_fd);
                    }
                    else if (fcntl(child_to_server[0], F_SETFL, O_NONBLOCK) < 0 ||
                             fcntl(child_to_server[1], F_SETFL, O_NONBLOCK) < 0 ||
                             fcntl(server_to_child[0], F_SETFL, O_NONBLOCK) < 0 ||
                             fcntl(server_to_child[1], F_SETFL, O_NONBLOCK) < 0) {
                        perror("fcntl() failed");
                        close(incoming_fd);
                        close(child_to_server[0]);
                        close(child_to_server[1]);
                        close(server_to_child[0]);
                        close(server_to_child[1]);
                    }
                    else if ((user_daemon_pid = fork()) < 0) {
                        perror("fork() failed");
                        close(incoming_fd);
                        close(child_to_server[0]);
                        close(child_to_server[1]);
                        close(server_to_child[0]);
                        close(server_to_child[1]);
                    }
                    else if (user_daemon_pid == 0) {
                        // we are now in child process
                        close(child_to_server[0]);
                        close(server_to_child[1]);

                        fcntl(incoming_fd, F_SETFL, O_NONBLOCK);
                        user_daemon(child_to_server[1], server_to_child[0], incoming_fd);
                    }
                    else {
                        // we are still in parent process
                        close(child_to_server[1]);
                        close(server_to_child[0]);
                        strcpy(user_list[index_to_add].username, username);
                        user_list[index_to_add].write_to_child = server_to_child[1];
                        user_list[index_to_add].read_from_child = child_to_server[0];
                        user_list[index_to_add].taken = 1;

                        // notify the user that they are connected succesfully
                        if (write(incoming_fd, "/joinresponse ok", strlen("/joinresponse ok")) < 0) {
                        	perror("write() failed while responding to join request");
                        	user_list_remove_user(user_list, index_to_add);
                        }
                        else {
                        	// notify all users that userlist has changed
                        	broadcast_user_list(user_list);
                        }
                    }
                }
            }
        }
        else {
            // check if there are messages to pass
            for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
                if (user_list[i].taken == 1) {
                    
                	// temporary buffer to hold potential message
                    char buf[BUF_SIZE];
                    memset(buf, '\0', BUF_SIZE);
                    ssize_t nread;

                    if ((nread = read(user_list[i].read_from_child, buf, BUF_SIZE)) != -1) {
                        if (nread == 0) {
                            // the connection to this user was lost, so remove them
                            user_list_remove_user(user_list, i);

                            // notify all users that userlist has changed
                            broadcast_user_list(user_list);
                        }
                        else {
                            if (memcmp(buf, "/whisper", strlen("/whisper")) == 0) {
                                // strtok modifies the original string so we must
                                // make another copy to single out the recipient
                                char rec_buf[BUF_SIZE];
                                memset(rec_buf, '\0', BUF_SIZE);
                                strcpy(rec_buf, buf + strlen("/whisper") + 1);

                                char *recipient = strtok(rec_buf, " ");
                                char *message = buf + strlen("/whisper") + 1 + strlen(recipient) + 1;

                                int recipient_index = user_list_get_index_by_username(user_list, recipient);

                                // note: the client does not allow whispering to non-
                                // connected users, so this check is unnecessary... in theory...
                                if (recipient_index >= 0) {
                                	// reformat the message to send it out
                                	char outgoing[BUF_SIZE];
                                	memset(outgoing, '\0', BUF_SIZE);
                                	sprintf(outgoing, "/whispered %s %s", user_list[i].username, message);

                                    if (write(user_list[recipient_index].write_to_child, outgoing, strlen(outgoing)) < 0) {
                                    	perror("write() failed while whispering");
                                    }

                                    // clear the outgoing buffer, use it to store a cc to original sender
                                	memset(outgoing, '\0', BUF_SIZE);
                                	sprintf(outgoing, "/whisperedcc %s %s", recipient, message);

	                                if (write(user_list[i].write_to_child, outgoing, strlen(outgoing)) < 0) {
	                                	perror("write() failed while whispering cc");
	                                }
                                }
                            }
                            else if (memcmp(buf, "/shout", strlen("/shout")) == 0) {
                                char *message = buf + strlen("/shout") + 1;
                                
                                // reformat the message to send it out
                                char outgoing[BUF_SIZE];
                                memset(outgoing, '\0', BUF_SIZE);
                                sprintf(outgoing, "/shouted %s %s", user_list[i].username, message);

                                for (int j = 0; j < MAX_CONCURRENT_USERS; j++) {
                                    if (j != i && user_list[j].taken == 1) {
                                        if (write(user_list[j].write_to_child, outgoing, strlen(outgoing)) < 0) {
                                        	perror("write() failed while shouting");
                                        }
                                    }
                                }

                                // clear the outgoing buffer, use it to store a cc to original sender
                                memset(outgoing, '\0', BUF_SIZE);
                                sprintf(outgoing, "/shoutedcc %s", message);

                                if (write(user_list[i].write_to_child, outgoing, strlen(outgoing)) < 0) {
                                	perror("write() failed while shouting cc");
                                }
                            }
                            // add other commands here, if any
                        }

                        // note: we end the loop early (and check remaining users on the next iteration)
                        // to prevent sending multiple messages to the same user in the same iteration
                        // (else, user would recieve multiple messages as one single message)
                        // since the client and server both "sleep" for the same duration, this should
                        // avoid collisions... in theory...
                        i = MAX_CONCURRENT_USERS;
                    }
                }
            }
        }

        usleep(MICRO_SLEEP_DUR);
    }  // end of while
}  // end of main
