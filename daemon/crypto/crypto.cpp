#include "crypto.hpp"

void crypto::generateKey(unsigned char *key){
    crypto_secretstream_xchacha20poly1305_keygen(key);
}

std::string crypto::stringToHex(const std::string &input) {
    std::string output;
    CryptoPP::StringSource(
        input, true,
        new CryptoPP::HexEncoder(new CryptoPP::StringSink(output)));
    return output;
  }

std::string crypto::hexToString(const std::string &input) {
    std::string output;
    CryptoPP::StringSource(
        input, true,
        new CryptoPP::HexDecoder(new CryptoPP::StringSink(output)));
    return output;
  }


std::string crypto::hexEncodeCipherPairLen(const char *input) {
    std::string input_str(input, input + sizeof(cipher_pair_len));
    return stringToHex(input_str);
}
  //
  // Encryption
  //

std::string crypto::encrypt_mssg(std::string message, size_t line_num,
                         crypto_secretstream_xchacha20poly1305_state &state) {
  size_t message_len = message.size();
  // unsigned char ciphertext[message_len + crypto_aead_aegis256_ABYTES];
  std::vector<unsigned char> ciphertext(
      message_len + crypto_secretstream_xchacha20poly1305_ABYTES);
  unsigned long long ciphertext_len;
  // Getting nonce for current line
  const unsigned char *ad_ptr = (const unsigned char *)&line_num;
  unsigned long long ad_len = sizeof(line_num);
  // Running encryption and getting cypher
  if (crypto_secretstream_xchacha20poly1305_push(
          &state, ciphertext.data(), &ciphertext_len, // Output ciphertext
          (const unsigned char *)message.c_str(),
          message_len,    // Input plaintext line
          ad_ptr, ad_len, // line number as metadata
          0               // Tag (0 for normal chunk)
          ) != 0) {
    std::cerr << "Error when encrypting mssg";
  }
  ciphertext.resize(ciphertext_len);
  std::string cipher_string(ciphertext.begin(), ciphertext.end());
  return stringToHex(cipher_string);
}
  


  //
  // Decryption
  //

std::string crypto::decrypt_mssg(std::string cipher, size_t line_num,
                    crypto_secretstream_xchacha20poly1305_state &state,
                    cipher_pair_len pair_len) {
  // Turning the hexcode string back to encrypted string
  cipher = hexToString(cipher);
  std::vector<unsigned char> message(pair_len.mssg_len +
                                crypto_secretstream_xchacha20poly1305_ABYTES);
  unsigned long long message_len;
  // Getting nonce for current line
  const unsigned char *ad_ptr = (const unsigned char *)&line_num;
  unsigned long long ad_len = sizeof(line_num);
  // Running decryption and getting original mssg
  if (crypto_secretstream_xchacha20poly1305_pull(
          &state, (unsigned char *)message.data(), &message_len, 0,
          (unsigned char *)cipher.data(), cipher.size(), ad_ptr,
          ad_len) != 0) {
    std::cerr << "Error when encrypting mssg";
  }
  message.resize(message_len);
  std::string output(message.begin(), message.end());
  return output;
}

