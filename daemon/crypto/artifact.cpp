
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

class crypto {

private:
  struct cipher_pair_len {
    size_t cipher_len;
    size_t mssg_len;
  };

  //
  // HexEncoding
  //

  std::string stringToHex(const std::string &input) {
    std::string output;
    CryptoPP::StringSource(
        input, true,
        new CryptoPP::HexEncoder(new CryptoPP::StringSink(output)));
    return output;
  }

  std::string hexToString(const std::string &input) {
    std::string output;
    CryptoPP::StringSource(
        input, true,
        new CryptoPP::HexDecoder(new CryptoPP::StringSink(output)));
    return output;
  }

  std::string hexEncodeCipherPairLen(const char *input) {
    std::string input_str(input, input + sizeof(cipher_pair_len));
    return stringToHex(input_str);
  }
  //
  // Encryption
  //

  std::string encrypt_mssg(std::string message, size_t line_num,
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

  std::string decrypt_mssg(std::string cipher, size_t line_num,
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


public:
  crypto() {
    // TODO:make key persistent in disk/memory
    crypto_secretstream_xchacha20poly1305_keygen(key);
  }



  /*
  
    File Decryption
  
  */




  void encrypt_file(std::string in_filename, std::string out_filename) {
    std::cout << "Started file encryption" << std::endl;
    // initalize input filestream
    std::ifstream infile(in_filename);
    // open output file in binary mode
    std::ofstream outfile(out_filename, std::ios::binary);
    std::string line;

    // keys and metadata needed for encryption
    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char header[HEADER_LEN];

    // initalizing state
    crypto_secretstream_xchacha20poly1305_init_push(&state, header, key);

    // writing header at top of file
    header[crypto_secretstream_xchacha20poly1305_HEADERBYTES] = '\n';
    std::string header_str(
        header, header + crypto_secretstream_xchacha20poly1305_HEADERBYTES);
    std::string hex_encoded_header = stringToHex(header_str);
    if (outfile.is_open()) {
      outfile.write(hex_encoded_header.data(), hex_encoded_header.size());
      outfile.write("\n", 1);
    }
    std::cout << "Finished writing header: " << header << std::endl
              << std::flush;

    if (infile.is_open()) {
      size_t line_num = 0;
      while (getline(infile, line)) {

        // Running encryption and getting cypher
        std::string cipher = encrypt_mssg(line, line_num, state);
        // writing line length info first
        cipher_pair_len pair_len = {cipher.size(), line.size()};
        std::cout << line_num << "  Pair Len:" << pair_len.mssg_len << ","
                  << pair_len.cipher_len << std::endl
                  << std::endl
                  << std::flush;

        const char *pair_len_buffer = reinterpret_cast<const char *>(&pair_len);
        const std::string pair_len_buffer_as_str =
            hexEncodeCipherPairLen(pair_len_buffer);
        outfile.write(pair_len_buffer_as_str.data(),
                      pair_len_buffer_as_str.size());
        outfile.write(PAIR_LEN_DELIMITER, 1);

        // writing encrypted line to file
        outfile.write((const char *)cipher.data(), cipher.size());
        // writing out the encrypted line+ a \n
        outfile.write("\n", 1);

        //line_num++;
      }
      infile.close();
      outfile.close();
    } else {
      std::cerr << "Unable to open file: " << in_filename << std::endl;
    }
    std::cout << "Finished file encryption" << std::endl;
  }

  void encrypt_directory(const std::string &filepath) {
    path source_dir(filepath);

    // Define our base encrypted directory target (e.g., "files/encrypted")
    path encrypted_base_dir = source_dir.parent_path() / "encrypted";

    for (const auto &dir_entry : recursive_directory_iterator(source_dir)) {

      // get the relative path from the source root
      // if source is "files" and current is "files/folder/stuff2.txt",
      // relative path becomes "folder/stuff2.txt"
      path relative_path = relative(dir_entry.path(), source_dir);

      // construct the absolute target path under the encrypted folder
      path target_path = encrypted_base_dir / relative_path;

      // if it's a directory, ensure it exists in the encrypted tree and move on
      if (dir_entry.is_directory()) {
        create_directories(target_path);
        continue;
      }

      // if it's a file, make sure its parent directory chain exists before
      // writing
      create_directories(target_path.parent_path());

      std::cout << "Encrypting: " << dir_entry.path().string() << " -> "
                << target_path.string() << std::endl
                << std::flush;

      // run encryption function using clean string paths
      encrypt_file(dir_entry.path().string(), target_path.string());
    }
  }



  /*
  
    File Decryption
  
  */

  std::string decrypt_window(std::string in_filename, size_t line_num, size_t window_size) {
    // initalize input filestream
    std::ifstream infile(in_filename, std::ios::binary);
    // open output file in binary mode
    

    // keys and metadata needed for encryption
    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];

    // initalizing state
    std::string line;
    std::cout << "Starting file decryption" << std::endl;
    if (infile.is_open() && getline(infile, line)) {
      // infile.read((char*)header,
      // crypto_secretstream_xchacha20poly1305_HEADERBYTES); infile.ignore(1);
      std::string decoded_header = hexToString(line);
      memcpy(header, decoded_header.data(),
             crypto_secretstream_xchacha20poly1305_HEADERBYTES);
    }
    crypto_secretstream_xchacha20poly1305_init_pull(&state, header, key);
    
    std::string decrypted_window;
    std::cout << "Finished reading header: " << header << std::endl << std::flush;
    if (infile.is_open()) {
      size_t curr_line = 0;
      size_t start_line = line_num-window_size/2;
      start_line = (start_line<0)?0:start_line;
      size_t end_line = line_num+window_size/2;
      while (getline(infile, line)) {
        
        cipher_pair_len pair_len;

        // finding cipher-pair len and cipher
        size_t delim_pos = line.find(PAIR_LEN_DELIMITER);
        std::string hex_encoded_cipher_pair_len = line.substr(0, delim_pos);
        std::string hex_encoded_cipher = line.substr(delim_pos + 1);

        std::string cipher_pair_len_str = hexToString(hex_encoded_cipher_pair_len);
        memcpy(&pair_len, cipher_pair_len_str.data(), sizeof(cipher_pair_len));

        std::cout << curr_line << "  Pair Len:" << pair_len.mssg_len << ","
             << pair_len.cipher_len << std::endl
             << std::endl
             << std::flush;
        // decrypt to get original line
        std::string original_line =
            decrypt_mssg(hex_encoded_cipher, 0, state, pair_len);

        if(curr_line>=start_line && curr_line<=end_line){
          decrypted_window += original_line + "\n";
        }

        //outfile.write(original_line.data(), original_line.size());
        //outfile.write("\n", 1);
        curr_line++;
      }
      infile.close();
    } else {
      std::cerr << "Unable to open file: " << in_filename << std::endl;
    }
    std::cout << "Finished file decryption" << std::endl;
    return decrypted_window;
  }

  void decrypt_file(std::string in_filename, std::string out_filename) {
    // initalize input filestream
    std::ifstream infile(in_filename, std::ios::binary);
    // open output file in binary mode
    std::ofstream outfile(out_filename, std::ios::binary);

    // keys and metadata needed for encryption
    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];

    // initalizing state
    std::string line;
    std::cout << "Starting file decryption" << std::endl;
    if (infile.is_open() && getline(infile, line)) {
      // infile.read((char*)header,
      // crypto_secretstream_xchacha20poly1305_HEADERBYTES); infile.ignore(1);
      std::string decoded_header = hexToString(line);
      memcpy(header, decoded_header.data(),
             crypto_secretstream_xchacha20poly1305_HEADERBYTES);
    }
    crypto_secretstream_xchacha20poly1305_init_pull(&state, header, key);

    std::cout << "Finished reading header: " << header << std::endl << std::flush;
    if (infile.is_open()) {
      size_t line_num = 0;
      while (getline(infile, line)) {

        cipher_pair_len pair_len;

        // finding cipher-pair len and cipher
        size_t delim_pos = line.find(PAIR_LEN_DELIMITER);
        std::string hex_encoded_cipher_pair_len = line.substr(0, delim_pos);
        std::string hex_encoded_cipher = line.substr(delim_pos + 1);

        std::string cipher_pair_len_str = hexToString(hex_encoded_cipher_pair_len);
        memcpy(&pair_len, cipher_pair_len_str.data(), sizeof(cipher_pair_len));

        std::cout << line_num << "  Pair Len:" << pair_len.mssg_len << ","
             << pair_len.cipher_len << std::endl
             << std::endl
             << std::flush;
        // decrypt to get original line
        std::string original_line =
            decrypt_mssg(hex_encoded_cipher, line_num, state, pair_len);

        outfile.write(original_line.data(), original_line.size());
        outfile.write("\n", 1);
        //line_num++;
      }
      infile.close();
      outfile.close();
    } else {
      std::cerr << "Unable to open file: " << in_filename << std::endl;
    }
    std::cout << "Finished file decryption" << std::endl;
  }

  void decrypt_directory(const std::string &filepath) {
    path source_dir(filepath);

    // Define our base encrypted directory target (e.g., "files/decrypted")
    path decrypted_base_dir = source_dir.parent_path() / "decrypted";

    for (const auto &dir_entry : recursive_directory_iterator(source_dir)) {

      // get the relative path from the source root
      // if source is "files" and current is "files/folder/stuff2.txt",
      // relative path becomes "folder/stuff2.txt"
      path relative_path = relative(dir_entry.path(), source_dir);

      // construct the absolute target path under the decrypted folder
      path target_path = decrypted_base_dir / relative_path;

      // if it's a directory, ensure it exists in the encrypted tree and move on
      if (dir_entry.is_directory()) {
        create_directories(target_path);
        continue;
      }

      // if it's a file, make sure its parent directory chain exists before
      // writing
      create_directories(target_path.parent_path());

      std::cout << "Decryptign: " << dir_entry.path().string() << " -> "
           << target_path.string() << std::endl
           << std::flush;

      // run encryption function using clean string paths
      decrypt_file(dir_entry.path().string(), target_path.string());
    }
  }


private:
  unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
};