//AES-256-GCM decrypt via libsodium 
//
#include <sodium.h> 
#include <string>
#include <fstream> 
#include <iostream>
#include <vector>
#include <cstring>
#include <cstring>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <filesystem> 
#define MESSAGE (const unsigned char *) "test"
#define MESSAGE_LEN 4
#define ADDITIONAL_DATA (const unsigned char *) "123456"
#define ADDITIONAL_DATA_LEN 6
#define HEADER_LEN crypto_secretstream_xchacha20poly1305_HEADERBYTES+1
#define PAIR_LEN_DELIMITER ":"


using namespace std;
using path = filesystem::path;
using recursive_directory_iterator = filesystem::recursive_directory_iterator;


struct cipher_pair_len{
    size_t cipher_len;
    size_t mssg_len;
};


string stringToHex(const string& input){
    string output;
    CryptoPP::StringSource(input,true, 
        new CryptoPP::HexEncoder(
            new CryptoPP::StringSink(output)
        )
    );
    return output;
}

string hexToString(const string& input){
    string output;
    CryptoPP::StringSource(input,true,
        new CryptoPP::HexDecoder(
            new CryptoPP::StringSink(output)
        )
    );
    return output;
}

string hexEncodeCipherPairLen( const char* input){
    string input_str(input,input+sizeof(cipher_pair_len));
    return stringToHex(input_str);
}


string encrypt_mssg(string message, size_t line_num,  crypto_secretstream_xchacha20poly1305_state &state){
    size_t message_len = message.size();
    //unsigned char ciphertext[message_len + crypto_aead_aegis256_ABYTES];
    vector<unsigned char> ciphertext(message_len + crypto_secretstream_xchacha20poly1305_ABYTES);
    unsigned long long ciphertext_len;
    
    //Getting nonce for current line
    const unsigned char *ad_ptr = (const unsigned char *)&line_num;
    unsigned long long ad_len = sizeof(line_num);

    //Running encryption and getting cypher
    if(
        crypto_secretstream_xchacha20poly1305_push(
            &state, 
            ciphertext.data(), &ciphertext_len,      // Output ciphertext
            (const unsigned char*)message.c_str(), message_len,     // Input plaintext line
            ad_ptr, ad_len,               // line number as metadata
            0                             // Tag (0 for normal chunk)
        )
        !=0
    ){
        cerr<<"Error when encrypting mssg";
    }    
    ciphertext.resize(ciphertext_len);
    string cipher_string(ciphertext.begin(),ciphertext.end());
    return stringToHex(cipher_string);
    
}


void encrypt_file(string in_filename, string out_filename, unsigned char* key){
    cout<<"Started file encryption"<<endl;
    //initalize input filestream 
    ifstream infile(in_filename);
    //open output file in binary mode
    ofstream outfile(out_filename,ios::binary);
    string line; 

    //keys and metadata needed for encryption
    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char header[HEADER_LEN];


    //initalizing state
    crypto_secretstream_xchacha20poly1305_init_push(&state, header, key);
    

    //writing header at top of file
    header[crypto_secretstream_xchacha20poly1305_HEADERBYTES] = '\n';
    string header_str(header,header+crypto_secretstream_xchacha20poly1305_HEADERBYTES);
    string hex_encoded_header = stringToHex(header_str);
    if(outfile.is_open()){
        outfile.write(hex_encoded_header.data(),hex_encoded_header.size());
        outfile.write("\n",1);
    }
    cout<<"Finished writing header: "<<header<<endl<<flush;

    
    if(infile.is_open()){
        size_t line_num  = 0;
        while(getline(infile,line)){

             //Running encryption and getting cypher
            string cipher = encrypt_mssg(line,line_num,state); 
            //writing line length info first 
            cipher_pair_len pair_len = {cipher.size(),line.size()};
            cout<<line_num<<"  Pair Len:"<<pair_len.mssg_len<<","<<pair_len.cipher_len<<endl<<endl<<flush;

            const char* pair_len_buffer = reinterpret_cast<const char*>(&pair_len);
            const string pair_len_buffer_as_str = hexEncodeCipherPairLen(pair_len_buffer);
            outfile.write(pair_len_buffer_as_str.data(),pair_len_buffer_as_str.size());
            outfile.write(PAIR_LEN_DELIMITER,1);

            //writing encrypted line to file
            outfile.write((const char *)cipher.data(), cipher.size());
            //writing out the encrypted line+ a \n
            outfile.write("\n",1);
            
            line_num++;    
        }
        infile.close();
        outfile.close();
    }
    else{
        cerr <<"Unable to open file: "<<in_filename<<endl; 
    }
    cout<<"Finished file encryption"<<endl;
}

void encrypt_directory(const string& filepath, unsigned char* key) {
    path source_dir(filepath);
    
    // Define our base encrypted directory target (e.g., "files/encrypted")
    path encrypted_base_dir = source_dir.parent_path() / "encrypted";

    for (const auto& dir_entry : recursive_directory_iterator(source_dir)) {
        
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

        // if it's a file, make sure its parent directory chain exists before writing
        create_directories(target_path.parent_path());

        cout << "Encrypting: " << dir_entry.path().string() 
             << " -> " << target_path.string() << endl << flush;

        // run encryption function using clean string paths
        encrypt_file(dir_entry.path().string(), target_path.string(), key);
    }
}



string decrypt_mssg(string cipher, size_t line_num, crypto_secretstream_xchacha20poly1305_state &state, cipher_pair_len pair_len){
    

    //Turning the hexcode string back to encrypted string
    cipher = hexToString(cipher);
    vector<unsigned char> message(pair_len.mssg_len + crypto_secretstream_xchacha20poly1305_ABYTES);
    unsigned long long message_len;
    
    //Getting nonce for current line
    const unsigned char *ad_ptr = (const unsigned char *)&line_num;
    unsigned long long ad_len = sizeof(line_num);

    //Running decryption and getting original mssg
    if(
        crypto_secretstream_xchacha20poly1305_pull(
            &state,
            (unsigned char *)message.data(),&message_len,
            0,(unsigned char *)cipher.data(),cipher.size(),ad_ptr,ad_len)
        !=0
    ){
        cerr<<"Error when encrypting mssg";
    }

    message.resize(message_len);
    string output(message.begin(),message.end());
    return output;


}



void decrypt_file(string in_filename, string out_filename,unsigned char *key){
    //initalize input filestream 
    ifstream infile(in_filename, ios::binary);
    //open output file in binary mode
    ofstream outfile(out_filename,ios::binary);

    //keys and metadata needed for encryption
    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];


    //initalizing state
    string line;
    cout<<"Starting file decryption"<<endl;
    if(infile.is_open() && getline(infile,line)){
        //infile.read((char*)header, crypto_secretstream_xchacha20poly1305_HEADERBYTES);
        //infile.ignore(1);
        string decoded_header = hexToString(line);
        memcpy(header,decoded_header.data(),crypto_secretstream_xchacha20poly1305_HEADERBYTES);
    }
    crypto_secretstream_xchacha20poly1305_init_pull(&state, header, key);

    cout<<"Finished reading header: "<<header<<endl<<flush;
    if(infile.is_open()){
        size_t line_num  = 0;
        while(getline(infile,line)){

            cipher_pair_len pair_len;

            //finding cipher-pair len and cipher
            size_t delim_pos = line.find(PAIR_LEN_DELIMITER);
            string hex_encoded_cipher_pair_len = line.substr(0,delim_pos);
            string hex_encoded_cipher = line.substr(delim_pos+1);
            
            string cipher_pair_len_str = hexToString(hex_encoded_cipher_pair_len);
            memcpy(&pair_len,cipher_pair_len_str.data(),sizeof(cipher_pair_len));            
           
            cout<<line_num<<"  Pair Len:"<<pair_len.mssg_len<<","<<pair_len.cipher_len<<endl<<endl<<flush;
            //decrypt to get original line
            string original_line = decrypt_mssg(hex_encoded_cipher, line_num, state, pair_len);
            
            outfile.write(original_line.data(), original_line.size());
            outfile.write("\n", 1);
            line_num++;
        }
        infile.close();
        outfile.close();
    }
    else{
        cerr <<"Unable to open file: "<<in_filename<<endl; 
    }
    cout<<"Finished file decryption"<<endl;
    
}

void decrypt_directory(const string& filepath, unsigned char* key){
    path source_dir(filepath);
    
    // Define our base encrypted directory target (e.g., "files/decrypted")
    path decrypted_base_dir = source_dir.parent_path() / "decrypted";

    for (const auto& dir_entry : recursive_directory_iterator(source_dir)) {
        
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

        // if it's a file, make sure its parent directory chain exists before writing
        create_directories(target_path.parent_path());

        cout << "Decryptign: " << dir_entry.path().string() 
             << " -> " << target_path.string() << endl << flush;

        // run encryption function using clean string paths
        decrypt_file(dir_entry.path().string(), target_path.string(), key);
    }

}

int main(void){
    
    
    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    crypto_secretstream_xchacha20poly1305_keygen(key);
    //string plain = "Hello"; 
    //string encoded = stringToHex(plain);
    //string decoded = hexToString(encoded);
    //cout<<plain<<endl<<encoded<<endl<<decoded<<endl;
    //cout<<(plain==decoded)<<endl;
    
    //encrypt_file("stuff.txt",key);
    //decrypt_file("encrypt.txt",key);

    encrypt_directory("files",key);
    decrypt_directory("encrypted",key);
    //print_directory("./files");
    return 0;
}