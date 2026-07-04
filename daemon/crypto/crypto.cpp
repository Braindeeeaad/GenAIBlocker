// crypto.cpp
#include "crypto.hpp"
#include <stdexcept>

void crypto::generateKey(unsigned char* key) {
    crypto_secretbox_keygen(key);
}

std::string crypto::binToHex(const std::string& input) {
    std::string hex(input.size() * 2 + 1, '\0');
    sodium_bin2hex(
        hex.data(), hex.size(),
        reinterpret_cast<const unsigned char*>(input.data()),
        input.size()
    );
    hex.resize(input.size() * 2);
    return hex;
}

std::string crypto::hexToBin(const std::string& input) {
    std::string bin(input.size() / 2, '\0');
    size_t bin_len;
    if (sodium_hex2bin(
            reinterpret_cast<unsigned char*>(bin.data()), bin.size(),
            input.data(), input.size(),
            nullptr, &bin_len, nullptr
        ) != 0) {
        throw std::runtime_error("hexToBin: invalid hex input");
    }
    bin.resize(bin_len);
    return bin;
}

std::string crypto::encryptLine(
    const std::string& plaintext,
    const unsigned char* key
) {
    // generate fresh random nonce for this line
    unsigned char nonce[NONCE_SIZE];
    randombytes_buf(nonce, NONCE_SIZE);

    // encrypt
    std::vector<unsigned char> ciphertext(MAC_SIZE + plaintext.size());
    crypto_secretbox_easy(
        ciphertext.data(),
        reinterpret_cast<const unsigned char*>(plaintext.data()),
        plaintext.size(),
        nonce,
        key
    );

    // prepend nonce to ciphertext before hex encoding
    // stored format: hex(nonce || mac || ciphertext)
    std::string combined;
    combined.append(reinterpret_cast<char*>(nonce), NONCE_SIZE);
    combined.append(reinterpret_cast<char*>(ciphertext.data()), ciphertext.size());

    return binToHex(combined);
}

std::string crypto::decryptLine(
    const std::string& ciphertext_hex,
    const unsigned char* key
) {
    std::string combined = hexToBin(ciphertext_hex);

    // minimum size = nonce + mac
    if (combined.size() < NONCE_SIZE + MAC_SIZE) {
        throw std::runtime_error("decryptLine: input too short");
    }

    // split nonce from the rest
    const unsigned char* nonce = 
        reinterpret_cast<const unsigned char*>(combined.data());
    const unsigned char* ciphertext = 
        reinterpret_cast<const unsigned char*>(combined.data() + NONCE_SIZE);
    size_t ciphertext_len = combined.size() - NONCE_SIZE;

    std::vector<unsigned char> plaintext(ciphertext_len - MAC_SIZE);

    if (crypto_secretbox_open_easy(
            plaintext.data(),
            ciphertext,
            ciphertext_len,
            nonce,
            key
        ) != 0) {
        throw std::runtime_error("decryptLine: authentication failed");
    }

    return std::string(plaintext.begin(), plaintext.end());
}