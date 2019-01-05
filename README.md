# Tiny-Chat

TinyChat is a lightweight chatroom server and client written in C. I made it for fun, as an exercise in network and GUI programming.

Internally it uses sockets to connect the server and client over a TCP connection, and displays the client GUI with GTK.

![gui example](https://i.imgur.com/dG2WZqM.png)

### Content

- [Compiling the server and client](#comp)
- [Setting up the server](#sett)
- [Using the client GUI](#usin)
- [How it works](#howi)
- [Still to do](#stil)

<a name=comp></a>
### Compiling the server and client

I've only tested this on Ubuntu 16.04, but it \*should\* work on any distribution. The only requirement is **libgtk-3-dev** which can be installed by the following shell command:

```
$ sudo apt-get install libgtk-3-dev
```

Once those are installed, simply `cd` into the `Tiny-Chat`directory you cloned, and run the `make` command. If you cloned this repo into your documents, that would look like:

```
$ cd
$ cd Documents/Tiny-Chat
$ make
```

*(note: by default, the makefile uses clang as the C compiler. If you don't have clang installed, you can probably change the makefile from `CXX = clang` to `CXX = gcc`, but I haven't tested this myself)...*

<a name=sett></a>
### Setting up the server

Before any client(s) can connect, the server must be running, and before the server can run, your network must be setup properly.

##### Designate a static IP address to the host machine
You will most likely want to designate a static IP address to your host machine, so it doesn't change. This can be done in your routers user preferences (typically you just add the MAC address of your host machine, and reboot it).

##### Pick a port and forward it to the static IP address
You **must** enable port forwarding on your router. This can also be done in your routers user preferences.

It will ask you to pick a port. I arbitrarily chose 6002, but it doesn't really matter as long as its greater than 1023 and less than 65536).

It will then ask you to bind the port to an IP address, this would be the IP address of your host machine (or the static one, if you set up a static IP for your host machine).

##### Run the server
Once that is setup, then you can run the server by `cd`'ing into the `Tiny-Chat` directory (the same as when you compiled it) and entering the following command:

```
$ ./build/tinychat_server.app <port>
```

where &lt;port&gt; is the port you setup port forwarding for in your router.

<a name=usin></a>
### Using the client GUI

No special network setup is needed to use the clients. Just `cd` into the `Tiny-Chat` directory and run the following command:

```
$ ./build/tinychat_client.app
```

The GUI will open, and prompt you to enter login credentials.

<!-- https://imgur.com/a/XFH0YDG -->

![login_window](https://i.imgur.com/o3y3sTb.png)

**server address**: the public IP address of the router of the network the host computer (running the server) is on.

**port**: the port the host computer is running the server on.

**username**: self-explanatory.

If the client is able to connect to the server, then the login window will transition into the chat window.

*(note: if the username you selected is taken, or the client is otherwise unable to connect to the server, an error dialog will popup to notify you).*

![chat_window](https://i.imgur.com/U4Lmscc.png)

##### Messages

Messages appear sequentially, and you can type your own in the entry form at the bottom of the window.

Messages can be sent either by pressing the enter key, or clicking the send button to the right of the entry.

##### Connected users

Clicking the people icon in the bottom left will show a list of all the currently connected users. Clicking on any of the users names will fill the entry form with a template to send a private message to that user.

##### Private messages

Private messages (only read by a single recipient) can be sent by appending an "@" symbol before the recipients username.

For instance, in a chat full of people "@john hey John!" would cause "hey John!" to appear only on John's screen.

Note that you cannot @ yourself, or any user who is not currently connected. Trying to do so will cause a warning dialog to appear.

<a name=howi></a>
### How it works

##### Process overview

The server runs in a single process. It initializes the tcp socket and loops, polling for incoming connections every 1 millisecond. When a user attempts to connect, it verifies the username is not taken and the server is not full, then responds that they may join. The server then forks a new process for that user to ensure a crash does not bring down the entire server.

The client runs the GUI initially, then connects to the server when the user presses the connect button. It then waits until the send button is pressed, at which point it sends the entry buffer across the socket, to the server. This process is illustrated in the diagram below.

<!-- https://imgur.com/aDMuw1y -->
![message flow overview](https://imgur.com/aDMuw1y.png)

The server has a daemon process for every user (which polls for input from the user via socket). If input is read, it passes it on to the server which interprets it, and sends it back out to the appropriate user(s) daemon processes, which in turn send it back to the user's client.

##### Message structure

In the client, messages can be entered in two formats:

1. &lt;message&gt;
2. @recipient &lt;message&gt;

In the first case, the message is sent to all users, while in the second case, it is sent to only the recipient.

Before the client sends the messages to the server, it formats them in a special way.

general messages are formatted as `/shout <message>` while private messages are formatted as `/whisper <recipient> <message>`.

The server then reads these in, and parses them, sending them back to clients as

`/shouted <sender> <message>` and `/whispered <sender> <message>`.

Additionally, copies of the sent messages are returned to the user in a special format `/shoutedcc <message>` and `/whisperedcc <recipient> <message>`.

Finally, when a new user joins or leaves, a message of the format `/userlist user1 user2 user3` ... etc, is sent to all remaining clients.

All of these, when read by the client, are displayed according to their nature.

*(note: because the "/" character is used to denote messages, users are not allowed to begin messages with it. Attempting to do so will cause an error dialog to pop up).*

<a name=stil></a>
### Still to do

- Administrative commands in the server (e.g. `/kick user`).
- Logging server activity to a file.
- The ability to password-protect a server.
- When a user leaves or joins, the remaining clients should print a message or somehow otherwise notify the user.
- When a new message arrives, the chat window should scroll to the bottom, or somehow otherwise notify the user.
- ... suggestions?