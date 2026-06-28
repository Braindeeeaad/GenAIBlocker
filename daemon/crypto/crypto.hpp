
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sodium.h>
#include <string>
#include <vector>
#define MESSAGE (const unsigned char *)"test"
#define MESSAGE_LEN 4
#define ADDITIONAL_DATA (const unsigned char *)"123456"
#define ADDITIONAL_DATA_LEN 6
#define HEADER_LEN crypto_secretstream_xchacha20poly1305_HEADERBYTES + 1
#define PAIR_LEN_DELIMITER ":"

using path = std::filesystem::path;
using recursive_directory_iterator =
    std::filesystem::recursive_directory_iterator;



namespace crypto{
    
    struct cipher_pair_len {
        size_t cipher_len; 
        size_t mssg_len;
    };

    void generateKey(unsigned char *key);
    std::string stringToHex(const std::string &input); 
    std::string hexToString(const std::string &input); 
    std::string hexEncodeCipherPairLen(const char *input); 

    std::string encrypt_mssg(std::string message, size_t line_num, 
                    crypto_secretstream_xchacha20poly1305_state &state); 
    std::string decrypt_mssg(std::string cipher, size_t line_num,
                    crypto_secretstream_xchacha20poly1305_state &state,
                    cipher_pair_len pair_len);
}