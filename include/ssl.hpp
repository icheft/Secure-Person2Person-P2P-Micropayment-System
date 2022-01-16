#include <iostream>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <random>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#define S string("")
#define CRLF "\r\n"
#define BLEN 1200

using namespace std;

FILE* fp;
string syscmd;

string random_string(size_t length)
{
    static auto& chrs = "0123456789"
                        "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg { std::random_device {}() };
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    string s;
    s.reserve(length);
    while (length--)
        s += chrs[pick(rg)];

    return s;
}

string create_key_and_certificate(string rec)
{
    //    syscmd = S + "rm *" + rec + ".key *" + rec + ".crt *" + rec + ".conf ";
    //    system(syscmd.c_str());

    string uid = random_string(8);
    string config_path = uid + "_" + rec + "_ssl.conf";
    syscmd = S + "touch " + config_path;
    system(syscmd.c_str());

    fp = fopen((uid + "_" + rec + "_ssl.conf").c_str(), "w");
    // DEFAULT CONFIGURATION CONTENTS
    fputs((S + "[req]\n" + "prompt = no\n" + "default_md = sha256\n" + "default_bits = 2048\n" + "distinguished_name = dn\n" + "x509_extensions = v3_req\n").c_str(), fp);
    fputs((S + "[dn]\n" + "C = TW\n" + "ST = Taiwan\n" + "L = Taipei\n" + "O = National Taiwan University\n" + "OU = IM Department\n" + "emailAddress = " + rec + "@example.com\n" + "CN = localhost\n").c_str(), fp);
    fputs(("[v3_req]"), fp);
    // DEFAULT CONFIGURATION CONTENTS
    fclose(fp);

    // generate private key and certificate using the
    syscmd = S + "openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 -keyout " + uid + "_" + rec + ".key -out " + uid + "_" + rec + ".crt -config " + config_path;
    system(syscmd.c_str());

    return uid;
}
