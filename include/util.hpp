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
    sprintf(syscmd, "openssl req -new -x509 -config %s/%s_ssc.conf -keyout %s/%s_%s.key -out %s/%s_%s.crt", cert_path, target, cert_path, uid.c_str(), pem_name, cert_path, uid.c_str(), pem_name);

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