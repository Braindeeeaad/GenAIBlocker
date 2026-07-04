// crypto.hpp
#pragma once
#include <sodium.h>
#include <string>
#include <vector>

class crypto {
public:

    static constexpr size_t KEY_SIZE   = crypto_secretbox_KEYBYTES;
    static constexpr size_t NONCE_SIZE = crypto_secretbox_NONCEBYTES;
    static constexpr size_t MAC_SIZE   = crypto_secretbox_MACBYTES;

    static void generateKey(unsigned char* key);

    static std::string binToHex(const std::string& input);
    static std::string hexToBin(const std::string& input);

    static std::string encryptLine(
        const std::string& plaintext,
        const unsigned char* key
    );

    static std::string decryptLine(
        const std::string& ciphertext_hex,
        const unsigned char* key
    );

};