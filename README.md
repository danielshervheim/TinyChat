# Tiny-Chat

**Tiny Chat** is a lightweight chat server and client, written in C.

I made it for fun, as an exercise in network and GUI programming. **Tiny Chat** uses TCP sockets to connect client(s) to the server, and GTK to display the client GUI.

![screenshots showing the login and chat interface windows](https://i.imgur.com/yeGxEm3.png)

## Setup

### Dependencies

**Tiny Chat** requires the following package be installed on the client machine(s):

```
libgtk-3-dev
```

### Network Requirements

The server machine must have a static IP address with a specific port forwarded to it from the router.

### Compilation

```
$ cd path/to/cloned/directory
$ cd Tiny-Chat
$ make
```

## Use

### Starting the server

```
$ cd path/to/cloned/directory
$ cd Tiny-Chat
$ ./build/tinychat_server.app <port>
```

### Starting the client

```
$ cd path/to/cloned/directory
$ cd Tiny-Chat
$ ./build/tinychat_client.app
```

### Login into the server

**Server Address**: the public IP address of the host machine (i.e. the IP address of the router it is connected to). This must be known ahead of time.

**Port**: the port the host machine is running the server on. This must be known ahead of time.

**Username**: how you want to identify yourself to others in the chat.

### Sending messages

Messages prefixed with "@" followed by a username will be sent only to that user. e.g.

`@john Hey John` will send "Hey John" to the user `john`alone.

Otherwise, messages will be sent to all currently connected users by default.