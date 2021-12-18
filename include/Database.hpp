#pragma once
#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#define MAX_QUERY_LENGTH 2048

#define REGISTER_OK 100
#define REGISTER_FAIL 210

#define LOGIN_SUCCESS 110
#define LOGIN_AUTH_FAIL 220
#define LOGIN_FAIL 221
#define LOGIN_EXIST 230
#define USER_NOT_LOGIN 240

#define LOGOUT_SUCCESS 120
#define LOGOUT_FAIL 250

#define TRANSFER_OK 130
#define TRANSFER_FAIL 260
#define TRANSFER_SENDER_BANKRUPT 270

#define QUERY_OK 300
#define QUERY_ERROR 400

using namespace std;
using namespace sqlite_orm;

struct Client
{
    string username;
    string ip;
    int public_port;
    int private_port;
    int online_status;
    int balance;
    int fd; // when establish a connection
};

class Database
{
private:
    sqlite3* db;
    char* zErrMsg;
    int rc;
    bool erase;
    static int initial_balance;

public:
    Database(bool erase = false, bool reset = false);
    ~Database();

    int user_register(string username, string IP);
    int user_login(string username, string IP, int public_port, int private_port, int fd);
    int user_logout(string IP, int public_port);
    int user_transaction(string snd, string rcv, double pay);

    vector<Client> list(bool online = true); // string IP, int public_port
    Client user_info(string username);
    int user_num(bool online = true);
    string user_list_info(vector<Client>);
    string user_list_info();

    static string client_table_name;
    static string db_name;
    static int callback(void* NotUsed, int argc, char** argv, char** azColName);
    static auto connect();
};