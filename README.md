<h1 align="center">A Secure Person2Person (P2P) Micropayment System - Server</h1>

<h3 align="center">IM 3010 Programming Assignment: Phase 02 Implementation</h3>


## Development

### Progress

本階段為 Server 端程式的開發，同學所撰寫的 Server 端程式必須要能:



**Submission**:

```sh
sh sub.sh <SID>
```


### Trouble-shooting
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

+ [ ] Creation of a database for handling multiple input and querying
    + see [test.c](./test/test.c)
+ [ ] Deletion of the database (`*.db`)
    + [Filesystem Library in `C++17`](https://stackoverflow.com/a/59424074/10871988) see [file.cpp](./test/file.cpp)
+ [ ] More on sqlite C++
    + <https://github.com/fnc12/sqlite_orm>
    + <https://www.runoob.com/sqlite/sqlite-c-cpp.html>
+ [ ] Thread and Worker Pool
    + <https://ncona.com/2019/05/using-thread-pools-in-cpp/>
+ [x] VSC not showing errors
    + Fixed as mentioned in [this issue](https://github.com/microsoft/vscode-cpptools/issues/2164#issuecomment-399232736)