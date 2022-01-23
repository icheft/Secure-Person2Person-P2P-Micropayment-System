#include <cstring>
#include <fstream>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <sstream>
#include <stdio.h>
#include <string>

int padding = RSA_PKCS1_PADDING;

RSA* createRSA(unsigned char* key, int public_key)
{
    RSA* rsa = NULL;
    BIO* keybio;
    keybio = BIO_new_mem_buf(key, -1);
    if (keybio == NULL) {
        printf("Failed to create key BIO");
        return 0;
    }
    if (public_key) {
        rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
    } else {
        rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    }
    if (rsa == NULL) {
        printf("Failed to create RSA");
    }

    return rsa;
}

int public_encrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* encrypted)
{
    RSA* rsa = createRSA(key, 1);
    int result = RSA_public_encrypt(data_len, data, encrypted, rsa, padding);
    return result;
}
int private_decrypt(unsigned char* enc_data, int data_len, unsigned char* key, unsigned char* decrypted)
{
    RSA* rsa = createRSA(key, 0);
    int result = RSA_private_decrypt(data_len, enc_data, decrypted, rsa, padding);
    return result;
}

int private_encrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* encrypted)
{
    RSA* rsa = createRSA(key, 0);
    int result = RSA_private_encrypt(data_len, data, encrypted, rsa, padding);
    return result;
}
int public_decrypt(unsigned char* enc_data, int data_len, unsigned char* key, unsigned char* decrypted)
{
    RSA* rsa = createRSA(key, 1);
    int result = RSA_public_decrypt(data_len, enc_data, decrypted, rsa, padding);
    return result;
}

void printLastError(char* msg)
{
    char* err = new char[130];
    ;
    memset(err, 0, 130);
    ERR_load_crypto_strings();
    ERR_error_string(ERR_get_error(), err);
    printf("%s ERROR: %s\n", msg, err);
    free(err);
}

std::string readKey(std::string path)
{
    std::ifstream file(path);
    char key[4096] = {};
    memset(key, 0, 4096);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // using printf() in all tests for consistency
            line += "\n";
            // printf("%s", line.c_str());
            strcat(key, line.c_str());
        }
        file.close();
    }
    return (std::string)key;
}

int main()
{

    char plainText[2048 / 8] = "an#200#brian"; // key length : 2048
    // openssl x509 -pubkey -noout -in client.crt  > pubkey.pem
    std::string publicKey = readKey("certs/9zii1_client.pem");
    // printf("%s\n", publicKey.c_str());
    std::string privateKey = readKey("certs/9zii1_client.key");
    // printf("%s\n", privateKey.c_str());

    unsigned char encrypted[4098] = {};
    unsigned char decrypted[4098] = {};

    int encrypted_length = public_encrypt((unsigned char*)plainText, strlen(plainText), (unsigned char*)publicKey.c_str(), encrypted);
    if (encrypted_length == -1) {
        printLastError("Public Encrypt failed ");
        exit(0);
    }
    printf("Encrypted length =%d\n", encrypted_length);

    int decrypted_length = private_decrypt(encrypted, encrypted_length, (unsigned char*)privateKey.c_str(), decrypted);
    if (decrypted_length == -1) {
        printLastError("Private Decrypt failed ");
        exit(0);
    }
    printf("Decrypted Text =%s\n", decrypted);
    printf("Decrypted Length =%d\n", decrypted_length);

    encrypted_length = private_encrypt((unsigned char*)plainText, strlen(plainText), (unsigned char*)privateKey.c_str(), encrypted);
    if (encrypted_length == -1) {
        printLastError("Private Encrypt failed");
        exit(0);
    }
    printf("Encrypted length =%d\n", encrypted_length);

    decrypted_length = public_decrypt(encrypted, encrypted_length, (unsigned char*)publicKey.c_str(), decrypted);
    if (decrypted_length == -1) {
        printLastError("Public Decrypt failed");
        exit(0);
    }
    printf("Decrypted Text =%s\n", decrypted);
    printf("Decrypted Length =%d\n", decrypted_length);
}