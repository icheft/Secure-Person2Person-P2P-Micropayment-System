#include <Database.hpp>

string Database::client_table_name = "CLIENT";
string Database::db_name = "server.db";
double Database::initial_balance = 10000;

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
                         "balance         NUMERIC;"; // init

    char sql[MAX_QUERY_LENGTH];
    sprintf(sql, query, this->client_table_name.c_str());
    /* Execute SQL statement */
    this->rc = sqlite3_exec(this->db, sql, this->callback, 0, &(this->zErrMsg));

    if (this->rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", this->zErrMsg);
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
    sqlite3_close(this->db);
    if (this->erase) {
        try {
            if (filesystem::remove(this->db_name))
                cout << "Database " << this->db_name << " deleted.\n";
            else
                cout << "Database " << this->db_name << " not found.\n";
        } catch (const filesystem::filesystem_error& err) {
            cout << "Filesystem error: " << err.what() << '\n';
        }
    }
}

int Database::user_register(string username, string ip)
{
    auto storage = this->connect();
    storage.sync_schema();
    Client new_user { username, "", -1, -1, 0, this->initial_balance };

    auto check_user = storage.get_all<Client>(where(c(&Client::username) == username));
    if (check_user.size() >= 1) {
        return REGISTER_FAIL;
    } else
        storage.replace(new_user);
    return REGISTER_OK;
}

int Database::user_login(string username, string ip, int public_port, int private_port)
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
            // Client login_user { username, ip, public_port, private_port, 1 };
            storage.update(user);
            cout << "Updated!\n";
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
        storage.update(user);
        cout << user.username << " logged out!\n";
    }
    return LOGOUT_SUCCESS;
}

int Database::user_transaction(string snd, string rcv, double pay)
{
    auto storage = this->connect();
    storage.sync_schema();

    auto sender_result = storage.get_all<Client>(where(c(&Client::username) == snd and c(&Client::online_status) == 1));
    auto receiver_result = storage.get_all<Client>(where(c(&Client::username) == rcv and c(&Client::online_status) == 1));
    if (sender_result.size() == 1 && receiver_result.size() == 1) {
        auto sender = sender_result[0];
        auto receiver = receiver_result[0];

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

vector<Client> Database::online_list() // string ip, int public_port
{
    auto storage = this->connect();
    storage.sync_schema();
    // auto checker = storage.get_all<Client>(where(c(&Client::ip) == ip and c(&Client::public_port) == public_port and c(&Client::online_status) == 1));
    return storage.get_all<Client>(where(c(&Client::online_status) == 1)); // vector<Client>
}

// static
int Database::callback(void* NotUsed, int argc, char** argv, char** azColName)
{
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("");
    return 0;
}