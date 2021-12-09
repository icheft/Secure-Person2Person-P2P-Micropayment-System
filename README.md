<center>
<h1>A Secure Person2Person (P2P) Micropayment System - Client</h1>

<h3>IM 3010 Programming Assignment: Phase 01 Implementation</h3>
</center>

+ [Introduction](#introduction)
+ [Environment](#environment)
    + [macOS](#macos)
    + [Ubuntu](#ubuntu)
+ [Usage](#usage)
    + [Running the Client Program](#running-the-client-program)
    + [Registering a User](#registering-a-user)
    + [Logging in to the Server](#logging-in-to-the-server)
    + [Listing System Information](#listing-system-information)
    + [Making Transaction with Peers](#making-transaction-with-peers)
    + [Logging out from the Server](#logging-out-from-the-server)
+ [How to Compile](#how-to-compile)
+ [References](#references)
    + [Client-side Implementation](#client-side-implementation)
    + [User Manual](#user-manual)
+ [Development](#development)
    + [Progress](#progress)
    + [Trouble-shooting](#trouble-shooting)

## Introduction

In **Phase 01**, we are asked to implement a client-side function for the Person-to-Person Transaction. The existing functions for the client-side program include *registering*, *login*, *listing*, *transacting*, and *exiting*. Simply start running the program by `./client <SERVER_IP> <SERVER_PORT>` after compilation (which can be done by `make client`).

The user manual will cover the running environment used when developing the program, the environment that this code could be used in, the usage of the client-side program, the compilation, and the references when doing this assignment.

## Environment

### macOS

The environment used to develop this project is:

> Operating System: macOS 12.0.1  
> CPP Standard: C++11

It means that **you can run this program in a macOS environment** if the program is also compiled in the exact environment.

### Ubuntu

For the given `client` binary, you can compile and run it in:

> Operating System: Ubuntu 20.04  
> CPP Standard: C++11

I am testing out the Linux-formatted `client` binary on [Karton](https://karton.github.io) from my MacBook. Karton is developed based on Docker containers. 

**Notice that the user interface requires [Nerd Fonts](https://github.com/ryanoasis/nerd-fonts) to render**.

## Usage

### Running the Client Program

Before running the client program, you have to make sure that the `server` is running. In the following case, I am running the client program on 127.0.0.1 with port 8888. 

![](docs/img/2021-12-10-01-04-52.png)

After logging in, a menu should popup:

![](docs/img/2021-12-10-01-05-03.png)

### Registering a User

Press `1` to register a user. You can register more than one user in one go.

![](docs/img/2021-12-10-01-06-16.png)

### Logging in to the Server

You can log in to a user by telling the program to use the JUST registered user account by typing `Y` (case sensitive) like what the following screenshot shows.

![](docs/img/2021-12-10-01-07-45.png)

Or you can also type in `n` (case sensitive) to manually key in the username:

![](docs/img/2021-12-10-01-09-36.png)

We also have to give the user a port to login on. If you enter `0`, my program will assign an available port for you automatically. Otherwise, you have to make sure that you pick an available one. 

The following screenshot (user `brian`) shows the case when `0` is recorded:

![](docs/img/2021-12-10-01-11-25.png)

The following screenshot (user `annie`) shows the case when a custom port is entered, but not available:

![](docs/img/2021-12-10-01-12-49.png)

If successfully logged in to the server, you should see that the prompt becomes `[username]> ` rather than `> `.

### Listing System Information

Now that I have logged in to `brian` and `annie`.

To request for a list of the system information, simply type in `3`:

![](docs/img/2021-12-10-01-14-55.png)

### Making Transaction with Peers

Making transaction with peers is simple, type in `4` to proceed. 

![](docs/img/2021-12-10-01-16-29.png)

If server receives the message and acknowledges the transaction, you should see `Transfer OK!` message, and there should be another listing right after:

![](docs/img/2021-12-10-01-21-41.png)

On the receiver side (the one who receives the money), there should be a notification telling that a transaction has been made.

![](docs/img/2021-12-10-01-22-50.png)


### Logging out from the Server

You can log out by typing `5`:

![](docs/img/2021-12-10-01-23-32.png)

The program will show you a goodbye message along with *bytes written* and *read* and the *elapsed time* in this session.


## How to Compile

You can start the program already by typing `./client <SERVER_IP> <SERVER_PORT>` into your terminal if you are on a Linux distribution (*Ubuntu* is used for testing).

To rebuild the program, on either **macOS** or a **Linux** system, run `make client` in the terminal app.

If `client` binary already exists, you may want to run `make clean` first to remove the file.

## References

### Client-side Implementation



+ Repositories
    + [Learn Network Protocol and Programming Using C](https://github.com/apsrcreatix/Socket-Programming-With-C) at <https://github.com/apsrcreatix/Socket-Programming-With-C>
    + [Peer to peer program in C](https://github.com/um4ng-tiw/Peer-to-Peer-Socket-C) at <https://github.com/um4ng-tiw/Peer-to-Peer-Socket-C>
    + [C Multithreaded Client-Server](https://github.com/RedAndBlueEraser/c-multithreaded-client-server) at <https://github.com/RedAndBlueEraser/c-multithreaded-client-server>
    + [Socket programming examples in C++](https://github.com/zappala/socket-programming-examples-c) at <https://github.com/zappala/socket-programming-examples-c>
+ Others
    + [Parse (split) a string in C++ using string delimiter (standard C++)](https://stackoverflow.com/a/14266139/10871988) at <https://stackoverflow.com/a/14266139/10871988>
    + [Finding Unused Port in C++](https://stackoverflow.com/a/1107242/10871988) at <https://stackoverflow.com/a/1107242/10871988>
    + [Unix Specification (link to `bind()`)](https://pubs.opengroup.org/onlinepubs/007908799/xns/bind.html) at <https://pubs.opengroup.org/onlinepubs/007908799/xns/bind.html>, but of course many more functions are looked up
    + [Port Forwarding for a Docker Container](https://docs.docker.com/config/containers/container-networking/) at <https://docs.docker.com/config/containers/container-networking/>
    + [Karton for not running on virtual machine](https://karton.github.io) at <https://karton.github.io>

### User Manual

+ [Eisvogel](https://github.com/Wandmalfarbe/pandoc-latex-template) at <https://github.com/Wandmalfarbe/pandoc-latex-template>


## Development

### Progress

本階段為 Client 端程式的開發，同學所撰寫的 Client 端程式必須要能:

+ [x] 向 Server 註冊(註冊時輸入名稱)。(register)
+ [x] 登入助教所提供 Server 端程式(填入使用者名稱、Port Number)。(port number)
+ [x] 向 Server 要最新的帳戶餘額、上線清單、Publickey 並接收 Server 端的回覆。 (list)
+ [x] 和其它 Client 執行轉帳功能 (不可透過 Server 端轉送)。(transaction)
+ [x] 進行離線動作前需主動告知 Server 端程式。(exit)

### Trouble-shooting
On Parallels:

1. `10.211.55.4`
2. on port `8888`

On workstation:

1. `sudo ufw allow 60100`
2. `./client 140.112.106.45 60100`