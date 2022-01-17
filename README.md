<h1 align="center">A Secure Person2Person (P2P) Micropayment System - OpenSSL</h1>

<h3 align="center">IM 3010 Programming Assignment: Phase 03 Implementation</h3>



## Development

### Progress

+ [x] Two-stage transaction 
+ [ ] [fswatch](https://github.com/emcrisostomo/fswatch) for file changes
+ [ ] [filewatcher](https://github.com/sol-prog/cpp17-filewatcher)

### Add Certificates

```sh
sh create_ca.sh cert server/client <cert_key_name>
sh create_ca.sh CA
```

### **Submission**

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

#### OpenSSL Environment Settings

+ macOS

    ```sh
    brew install openssl
    brew link openssl # optional
    ```

    ```sh
    If you need to have openssl@3 first in your PATH, run:
        echo 'export PATH="/usr/local/opt/openssl@3/bin:$PATH"' >> ~/.zshrc

    For compilers to find openssl@3 you may need to set:
        export LDFLAGS="-L/usr/local/opt/openssl@3/lib"
        export CPPFLAGS="-I/usr/local/opt/openssl@3/include"
    ```
+ Linux

    ```sh
    sudo apt-get install openssl
    sudo apt-get install libssl-dev
    ```

#### Keys

```sh
openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 -keyout a.key -out a.crt
# openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout mycert.pem -out mycert.pem
```

#### Client 


#### Server


### ToDos and References

+ OpenSSL
    + [SSL socket 小攻略](https://hackmd.io/@J-How/B1vC_LmAD#FAQ)
    + [OpenSSL 範例](http://neokentblog.blogspot.com/2012/10/openssl-ssl.html)
    + [ssl server client programming using openssl in c](https://aticleworld.com/ssl-server-client-using-openssl-in-c/)
    + [SSL 通道小攻略](https://hackmd.io/@G9IwPB5oTmOK_qFXzKABGg/rJkvqdgJ_#SSL通道小攻略)
    + [Creating Keys](https://stackoverflow.com/questions/10175812/how-to-generate-a-self-signed-ssl-certificate-using-openssl)