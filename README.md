# TinyChat

**TinyChat** is a lightweight chat server and client for unix, written in C.

I made it for fun, as an exercise in network and GUI programming. **TinyChat** uses TCP sockets to connect client(s) to a central server, and GTK to display the client GUI.

![login window](https://i.imgur.com/oFXRBS4.png)

![chat window](https://i.imgur.com/gyDsvkT.png)

## Setup

### Dependencies

**TinyChat** requires the following package be installed:

- libgtk-3-dev

### Network Requirements

The machine hosting the server must have a static IP address assigned, with an open port forwarded to that IP address.

### Compilation

```
$ git clone https://github.com/danielshervheim/TinyChat
$ cd TinyChat
$ make
$ sudo make install
```

## Use

### Starting the server

(install first)

```
$ tinychat_server <port>
```

### Starting the client

(install first)

```
$ tinychat_client
```

Additionally, the client can be started from the application launcher like any other Linux application.

### Login into the server

**Server Address**: The public IP address of the host machine (i.e. the IP address of the router it is connected to). This must be known ahead of time.

**Port**: The port that the host machine has forwarded to itself. This must be known ahead of time as well.

**Username**: How you want to identify yourself to others in the chat.

### Sending messages

The dropdown on the bottom left displays a list of all currently connected users. Selecting a users name from the list will send all your messages only to that user.

Selecting the top "Everyone" option will send your messages to all currently connected users. This is the default option.
