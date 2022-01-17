#pragma once
#include <iostream>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <random>
#include <stdlib.h> // system
#include <string.h>
#include <string>
#include <vector>

using namespace std;

string random_string(std::string::size_type length)
{
    // https://stackoverflow.com/a/24586587/10871988
    static auto& chrs = "0123456789"
                        "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg { std::random_device {}() };
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;

    s.reserve(length);

    while (length--)
        s += chrs[pick(rg)];

    return s;
}

string create_key_and_certificate(const char* cert_path, const char* target, const char* pem_name)
{
    string uid = random_string(5);
    char* syscmd = new char[1024];
    // generate private key and certificate using the config
    // ssc stands for self-signed certificate
    printf("Creating dynamic Key and Certificate with target ID [%s]...\n", uid.c_str());
    // sprintf(syscmd, "openssl req -new -x509 -config %s/%s_ssc.conf -keyout %s/%s_%s.key -out %s/%s_%s.crt", cert_path, target, cert_path, uid.c_str(), pem_name, cert_path, uid.c_str(), pem_name);
    sprintf(syscmd, "openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 -keyout %s/%s_%s.key -out %s/%s_%s.crt -subj \"/C=TW/ST=Taiwan/L=TPE/O=NTU/OU=IM/CN=localhost/emailAddress=%s_%s@icheft\"", cert_path, uid.c_str(), pem_name, cert_path, uid.c_str(), pem_name, uid.c_str(), pem_name);
    // openssl req -new -x509 -config %s/%s_ssc.conf -keyout %s/%s_%s.key -out %s/%s_%s.crt", cert_path, target, cert_path, uid.c_str(), pem_name, cert_path, uid.c_str(), pem_name);
    // openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 -keyout a.key -out a.crt -subj "/C=TW/ST=Taiwan/L=TPE/O=NTU/OU=IM/CN=localhost/emailAddress=uid_client@icheft"

    system(syscmd);
    printf("Done.\n");

    printf("Regenerating CA file...\n");
    sprintf(syscmd, "rm %s/CA.pem\n"
                    "for i in %s/*.crt ; do\n"
                    "\topenssl x509 -in $i -text >> %s/CA.pem\n"
                    "done\n",
        cert_path, cert_path, cert_path);

    system(syscmd);
    printf("CA file rewritten.\n");

    return uid;
}

void delete_key_and_certificate(string uid, const char* cert_path, const char* target, const char* pem_name, bool reset = true)
{
    // delete key when system (client) is shutting down
    char* syscmd = new char[1024];
    printf("Deleting dynamic Key and Certificate with target ID [%s]...\n", uid.c_str());
    sprintf(syscmd, "rm %s/%s_%s.key", cert_path, uid.c_str(), pem_name);
    system(syscmd);
    sprintf(syscmd, "rm %s/%s_%s.crt", cert_path, uid.c_str(), pem_name);
    system(syscmd);
    printf("Done.\n");

    if (reset) {
        printf("Regenerating CA file...\n");
        sprintf(syscmd, "rm %s/CA.pem\n"
                        "for i in %s/*.crt ; do\n"
                        "\topenssl x509 -in $i -text >> %s/CA.pem\n"
                        "done\n",
            cert_path, cert_path, cert_path);

        system(syscmd);
        printf("CA file rewritten.\n");
    }

    return;
}

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    // Load certificate into SSL_CTX structure
    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    // Load private key into SSL_CTX structure
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    // Check certificate and private key compatibility
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

void ShowCerts(SSL* ssl)
{
    X509* cert;
    char* line;

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
        printf("Digital certificate information:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Certificate: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    } else
        printf("No certificate information.\n");
}

/*
SSL read with decryption

>>> int tmp_byte_read = SSL_read_D(ssl, rsa_key, plaintext, MAX_LENGTH);
*/
int SSL_read_D(SSL* ssl, RSA* rsa_key, char* decrypted, int MAX_LENGTH, bool verbose = false)
{
    char buffer[MAX_LENGTH];
    // int tmp_byte_read = recv(connection, buffer, sizeof(buffer), 0); // RECV_SIGNAL
    int tmp_byte_read = SSL_read(ssl, buffer, 256 + 1);
    // int len = RSA_size(rsa_key);

    char* plaintext = new char[RSA_size(rsa_key) + 1];

    if (verbose)
        printf("\n[System Info] Encrypted message received - <%s>\n", buffer);

    int decrypt_err = RSA_public_decrypt(256, (unsigned char*)buffer, (unsigned char*)plaintext, rsa_key, RSA_PKCS1_PADDING);

    if (decrypt_err == -1) {
        printf("decrypt error\n");
        // exit(1);
    }

    string tmp_raw(plaintext);
    strcpy(decrypted, tmp_raw.c_str());
    return tmp_byte_read;
}

/*
SSL write with encryption

>>> int tmp_byte_write = SSL_write_E(ssl, key_path, response, MAX_LENGTH);
*/
int SSL_write_E(SSL* ssl, string key_path, string response, int MAX_LENGTH, bool verbose = false)
{
    // verbose option is kept for future implementation
    FILE* fp = fopen(key_path.c_str(), "r");
    RSA* p_key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);

    int s_len = RSA_size(p_key);
    char* ciphertext = new char[s_len + 1];
    memset(ciphertext, 0, s_len + 1);
    int encrypt_err = RSA_private_encrypt(response.size() + 1, (const unsigned char*)response.c_str(), (unsigned char*)ciphertext, p_key, RSA_PKCS1_PADDING);
    if (encrypt_err == -1) {
        printf("encrypt error\n");
        // exit(1);
    }

    int tmp_byte_write = SSL_write(ssl, ciphertext, RSA_size(p_key) + 1);

    return tmp_byte_write;
}