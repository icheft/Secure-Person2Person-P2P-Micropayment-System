#include "Database.hpp"

string Database::client_table_name = "CLIENT";
string Database::db_name = "server.db";
int Database::initial_balance = 10000;
// overload
string Database::user_list_info()
{
    auto online_users = this->list(true);
    string res = "";
    for (auto& user : online_users) {
        res += user.username + "#" + user.ip + "#" + to_string(user.private_port) + "\n";
    }
    return res;
}
string Database::user_list_info(vector<Client> user_list)
{
    if (user_list.size() == 0) {
        return "Online num:0";
    } else {
        /*
        Online num:1
        10000
        public key
        1
        brian#140.112.235.41#63328
        */
        string res = "";
        for (auto& user : user_list) {
            res += user.username + "#" + user.ip + "#" + to_string(user.private_port) + "\n";
        }
        return res;
    }
}

auto Database::connect()
{
    if (db_name.empty()) {
        printf("The database doesn't exist.\n");
        exit(0);
    }
    return make_storage(db_name.c_str(),
        make_table(client_table_name,
            make_column("username", &Client::username, primary_key()),
            make_column("ip", &Client::ip),
            make_column("public_port", &Client::public_port),
            make_column("private_port", &Client::private_port),
            make_column("online_status", &Client::online_status),
            make_column("balance", &Client::balance)));
    // make_column("rsa_key", &Client::rsa_key)
}

Database::Database(bool erase, bool reset)
{
    this->erase = erase;
    this->rc = sqlite3_open(db_name.c_str(), &(this->db));
    if (this->rc) {
        fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(this->db));
        exit(0);
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    // create tables
    char* query = (char*)"CREATE TABLE %s("
                         "username        CHAR(50)  PRIMARY KEY NOT NULL,"
                         "ip              CHAR(50)              NOT NULL,"
                         "public_port     INT," // public port; assigned when login
                         "private_port    INT," // private port; assigned when login
                         "online_status   INT," // 0 or 1 default to 0
                         "balance         NUMERIC," // init
                         "fd              INT);"; // file descriptor

    char sql[MAX_QUERY_LENGTH];
    sprintf(sql, query, this->client_table_name.c_str());
    /* Execute SQL statement */
    this->rc = sqlite3_exec(this->db, sql, this->callback, 0, &(this->zErrMsg));

    if (this->rc != SQLITE_OK) {
        // unable to create table
        fprintf(stderr, "SQL warning: %s\n", this->zErrMsg);
        sqlite3_free(this->zErrMsg);
    } else {
        fprintf(stdout, "Client table created successfully\n");
    }

    if (reset) {
        auto storage = this->connect();
        storage.sync_schema();
        storage.remove_all<Client>();
    }
}

Database::~Database()
{
    if (this->erase) {
        sqlite3_close(this->db);
        try {
            if (filesystem::remove(this->db_name))
                printf("Database %s deleted successfully.\n", this->db_name.c_str());
            else
                printf("Database %s not found.\n", this->db_name.c_str());
        } catch (const filesystem::filesystem_error& err) {
            printf("Filesystem error: %s\n", err.what());
        }
    } else {
        // logout users
        auto storage = this->connect();
        auto check_user = storage.get_all<Client>(where(c(&Client::online_status) == 1));
        for (auto user : check_user) {
            // Client user = check_user[0];
            // Client logout_user { username, "", -1, -1, 0 };
            user.ip = "";
            user.public_port = -1;
            user.private_port = -1;
            user.online_status = 0;
            user.fd = -1;
            storage.update(user);

            printf("%s is forcibly logged out.\n", user.username.c_str());
        }
        sqlite3_close(this->db);
    }
}

int Database::user_register(string username, string ip)
{
    auto storage = this->connect();
    storage.sync_schema();
    Client new_user { username, "", -1, -1, 0, this->initial_balance, -1 };

    auto check_user = storage.get_all<Client>(where(c(&Client::username) == username));
    if (check_user.size() >= 1) {
        return REGISTER_FAIL;
    } else
        storage.replace(new_user);
    return REGISTER_OK;
}

int Database::user_login(string username, string ip, int public_port, int private_port, int fd)
{
    auto storage = this->connect();
    storage.sync_schema();

    auto check_user = storage.get_all<Client>(where(c(&Client::username) == username));
    if (check_user.size() == 0) {
        return LOGIN_AUTH_FAIL;
    } else {
        auto check_online = storage.get_all<Client>(where(c(&Client::username) == username and c(&Client::online_status) == 1));
        if (check_online.size() == 1)
            return LOGIN_EXIST;
        else {
            Client user = check_user[0];
            user.ip = ip;
            user.public_port = public_port;
            user.private_port = private_port;
            user.online_status = 1;
            user.fd = fd;
            // user.rsa_key = rsa_key;
            // Client login_user { username, ip, public_port, private_port, 1 };
            storage.update(user);
            printf("Updated!\n");
        }
    }
    return LOGIN_SUCCESS;
}

int Database::user_logout(string ip, int public_port)
{
    auto storage = this->connect();
    storage.sync_schema();

    auto check_user = storage.get_all<Client>(where(c(&Client::ip) == ip and c(&Client::public_port) == public_port and c(&Client::online_status) == 1));
    if (check_user.size() != 1) {
        return LOGOUT_FAIL;
    } else {
        Client user = check_user[0];
        // Client logout_user { username, "", -1, -1, 0 };
        user.ip = "";
        user.public_port = -1;
        user.private_port = -1;
        user.online_status = 0;
        user.fd = -1;
        storage.update(user);
        printf("%s logged out.\n", user.username.c_str());
    }
    return LOGOUT_SUCCESS;
}

int Database::user_transaction(string snd, string rcv, double pay)
{
    auto storage = this->connect();
    storage.sync_schema();
    if (pay <= 0) {
        return TRANSFER_FAIL;
    }
    auto sender_result = storage.get_all<Client>(where(c(&Client::username) == snd and c(&Client::online_status) == 1));
    auto receiver_result = storage.get_all<Client>(where(c(&Client::username) == rcv and c(&Client::online_status) == 1));
    if (sender_result.size() == 1 && receiver_result.size() == 1) {
        auto sender = sender_result[0];
        auto receiver = receiver_result[0];

        // RSA* snd_key = sender.rsa_key; // will not be used here
        // RSA* rcv_key = receiver.rsa_key; // receiver key is enough for this task

        if (sender.balance - pay < 0) {
            return TRANSFER_SENDER_BANKRUPT;
        } else {
            sender.balance -= pay;
            receiver.balance += pay;
            storage.update(sender);
            storage.update(receiver);
        }
    } else {
        return TRANSFER_FAIL;
    }
    return TRANSFER_OK;
}

Client Database::user_info(string username)
{
    auto storage = this->connect();
    storage.sync_schema();
    return storage.get_all<Client>(where(c(&Client::username) == username))[0];
}

vector<Client> Database::list(bool online) // string ip, int public_port
{
    auto storage = this->connect();
    storage.sync_schema();
    // auto checker = storage.get_all<Client>(where(c(&Client::ip) == ip and c(&Client::public_port) == public_port and c(&Client::online_status) == 1));
    if (online)
        return storage.get_all<Client>(where(c(&Client::online_status) == 1)); // vector<Client>
    else
        return storage.get_all<Client>();
}

int Database::user_num(bool online)
{
    return this->list(online).size();
}

// static
int Database::callback(void* NotUsed, int argc, char** argv, char** azColName)
{
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}