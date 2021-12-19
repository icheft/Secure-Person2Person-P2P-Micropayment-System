<h1 align="center">A Secure Person2Person (P2P) Micropayment System - Server</h1>

<h3 align="center">IM 3010 Programming Assignment: Phase 02 Implementation</h3>


## Development

### Progress

本階段為 Server 端程式的開發，同學所撰寫的 Server 端程式必須要能:



**Submission**:

```sh
sh sub.sh <SID>
```

### `Database` Usage

```cpp
Database* db = new Database(true, true);
cout << db->user_register("brian", "127.0.0.1") << endl;
cout << db->user_login("brian", "127.0.0.1", 65312, 8888) << endl;
cout << db->user_logout("127.0.0.1", 65313) << endl;
cout << db->user_transaction("brian", "michael", 2000) << endl;

auto online_users = db->list(); // online is true by default

cout << "Online num: " << online_users.size() << endl;
for (auto& user : online_users) {
    cout << user.username << "#" << user.ip << "#" << user.private_port << endl;
}
cout << "\n";
```

```cpp
#define REGISTER_OK 100
#define REGISTER_FAIL 210

#define LOGIN_SUCCESS 110
#define LOGIN_AUTH_FAIL 220
#define LOGIN_EXIST 230
#define USER_NOT_LOGIN 240

#define LOGOUT_SUCCESS 120
#define LOGOUT_FAIL 250

#define TRANSFER_OK 130
#define TRANSFER_FAIL 260
#define TRANSFER_SENDER_BANKRUPT 270

#define QUERY_OK 300
#define QUERY_ERROR 400

```



### Trouble-shooting

#### Environment Setup on Linux

+ Make sure you have your GCC version `>= 8`
    ```sh
    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo apt update
    sudo apt install gcc-9 g++-9
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-9 # to make sure gcc is using the latest version of GCC
    ```
+ Also have to install `sqlite3`
    ```sh
    sudo apt install sqlite3
    sudo apt-get install libsqlite3-dev
    ```

#### Server 

Since we are using a thread pool here, all clients must disconnect before leaving the server to ensure a better shutdown process.

#### IP and Port # Setup
**On Parallels**:

1. `10.211.55.4`
2. on port `8888`

**On workstation**:

1. `sudo ufw allow 60100`
2. `./client 140.112.106.45 60100`

**Karton**: 

Execute everything within Karton.

#### `sqlite3`

**Linux**:

```
sudo apt install sqlite3
sudo apt-get install libsqlite3-dev
```

**macOS**:

`sqlite` is pre-installed on macOS.


### ToDos and References

+ [x] Creation of a database for handling multiple input and querying
    + see [test.c](./test/test.c)
+ [x] Deletion of the database (`*.db`)
    + [Filesystem Library in `C++17`](https://stackoverflow.com/a/59424074/10871988) see [file.cpp](./test/file.cpp)
+ [x] More on sqlite C++
    + <https://github.com/fnc12/sqlite_orm>
    + <https://www.runoob.com/sqlite/sqlite-c-cpp.html>
+ [x] Thread and Worker Pool - mine works between 1 and 2
    + <https://ncona.com/2019/05/using-thread-pools-in-cpp/>
    + <https://stackoverflow.com/questions/15752659/thread-pooling-in-c11>
    + <https://stackoverflow.com/questions/48943929/killing-thread-from-another-thread-c>
    + [x] <https://github.com/vit-vit/ctpl>
+ [x] Handling `SIGINT`
    + <https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event>
+ [x] VSC not showing errors
    + Fixed as mentioned in [this issue](https://github.com/microsoft/vscode-cpptools/issues/2164#issuecomment-399232736)
+ [x] `TIME WAIT`
    + Background: Server couldn't close connection after the socket is closed:
        ```sh
        sudo netstat -tanl | grep 8888
        ```
        ```
        tcp4       0      0  127.0.0.1.64480                               127.0.0.1.8888                                TIME_WAIT
        ```
        <https://stackoverflow.com/questions/23915304/how-to-avoid-time-wait-for-server-sockets>
+ [ ] cmd args
    + <https://github.com/mirror/tclap>
    + <https://github.com/vietjtnguyen/argagg>
+ [ ] Makefile: Nicer linkage
    + <https://stackoverflow.com/questions/451413/make-makefile-progress-indication>
    + <https://stackoverflow.com/a/16945143/10871988>
    + <https://www.gnu.org/software/make/manual/make.html>
+ [x] Catch SIGPIPE from sudden death of a client
    + <https://stackoverflow.com/questions/61688091/catching-client-exit-from-server-on-socket-programing>
    + <https://stackoverflow.com/questions/26752649/so-nosigpipe-was-not-declared>
    + <https://stackoverflow.com/questions/18935446/program-received-signal-sigpipe-broken-pipe/18963142>