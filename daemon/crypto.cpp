//AES-256-GCM decrypt via libsodium 
//
#include <sodium.h> 
#include <string>
#include <fstream> 
#include <iostream>
#include <vector>
#include <cstring>
#include <cstring>
#include <iomanip>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>

#define MESSAGE (const unsigned char *) "test"
#define MESSAGE_LEN 4
#define ADDITIONAL_DATA (const unsigned char *) "123456"
#define ADDITIONAL_DATA_LEN 6
#define HEADER_LEN crypto_secretstream_xchacha20poly1305_HEADERBYTES+1

using namespace std;

struct cipher_pair_len{
    size_t cipher_len;
    size_t mssg_len;
};


string stringToHex(const string& input){
    stringstream ss; 
    ss << std::hex << std::setfill('0');
    for(unsigned char c: input){
        ss << setw(2) << static_cast<int>(c); 
    }
    return ss.str();
}

string hexToString(const string& input){
    string output;
    if(input.length() % 2 != 0) return ""; 
    for(size_t i=0; i<input.length(); i+=2){
        string byteString = input.substr(i,2); 
        char byte = static_cast<char>(stoi(byteString,nullptr,16));
        output.push_back(byte);
    }
    return output;
}



string encrypt_mssg(string message, size_t line_num,  crypto_secretstream_xchacha20poly1305_state state){
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


void encrypt_file(string filename, unsigned char* key){
    cout<<"Started file encryption"<<endl;
    //initalize input filestream 
    ifstream infile(filename);
    //open output file in binary mode
    ofstream outfile("encrypt.txt",ios::binary);
    string line; 

    //keys and metadata needed for encryption
    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char header[HEADER_LEN];


    //initalizing state
    crypto_secretstream_xchacha20poly1305_keygen(key);
    crypto_secretstream_xchacha20poly1305_init_push(&state, header, key);
    header[crypto_secretstream_xchacha20poly1305_HEADERBYTES] = '\n';
    if(outfile.is_open())
        outfile.write((const char*)header,HEADER_LEN);

    cout<<"Finished writing header: "<<header<<endl<<flush;

    
    if(infile.is_open()){
        size_t line_num  = 0;
        while(getline(infile,line)){
            //cout << line << endl;



            //Running encryption and getting cypher
            string cipher = encrypt_mssg(line,line_num,state);
            

            //writing line length info first 
            cipher_pair_len pair_len = {cipher.size(),line.size()};
            const char* pair_len_buffer = reinterpret_cast<const char*>(&pair_len);
            outfile.write(pair_len_buffer,sizeof(cipher_pair_len));


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
        cerr <<"Unable to open file: "<<filename<<endl; 
    }
    cout<<"Finished file encryption"<<endl;
}

string decrypt_mssg(string cipher, size_t line_num, crypto_secretstream_xchacha20poly1305_state state, cipher_pair_len pair_len){
    

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
void decrypt_file(string filename,unsigned char *key){
    //initalize input filestream 
    ifstream infile(filename);
    //open output file in binary mode
    ofstream outfile("decrypt.txt",ios::binary);
    string line; 

    //keys and metadata needed for encryption
    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];


    //initalizing state
    crypto_secretstream_xchacha20poly1305_keygen(key);
    crypto_secretstream_xchacha20poly1305_init_pull(&state, header, key);
    cout<<"Starting file decryption"<<endl;
    if(infile.is_open())
        infile.read((char *)header,crypto_secretstream_xchacha20poly1305_HEADERBYTES/sizeof(char));

    cout<<"Finished reading header: "<<header<<endl<<flush;

    if(infile.is_open()){
        size_t line_num  = 0;
        while(getline(infile,line)){
            if(line=="")continue;
            cout <<"Line: "<<line << endl<<flush;
            
            //reading cipher pair len first 
            cipher_pair_len pair_len;
            cout<<"We chilling1"<<endl<<flush;
            string cipher_pair_string = line.substr(0,sizeof(cipher_pair_len));
            cout<<"We chilling2"<<endl<<flush;

            char * pair_len_buffer = (char *)cipher_pair_string.c_str(); 
            memcpy(&pair_len,pair_len_buffer,sizeof(cipher_pair_len));

            cout<<"We chilling3"<<endl<<flush;

            //getting cipher portion of string 
            string cipher_string = line.substr(sizeof(cipher_pair_len));
            cout<<"We chilling4"<<endl<<flush;

            //Running encryption and getting cypher
            string original_line = decrypt_mssg(cipher_string,cipher_string.size(),state,pair_len);
            
            //writing encrypted line to file
            outfile.write((const char *)original_line.data(), original_line.size());
            outfile.write("\n",1);
            
            line_num++;
        }
        infile.close();
        outfile.close();
    }
    else{
        cerr <<"Unable to open file: "<<filename<<endl; 
    }

}

int main(void){
    
    
    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    crypto_secretstream_xchacha20poly1305_keygen(key);

    cout<< (bool)(hexToString(stringToHex("Hello"))==stringToHex("Hello"))<<endl;
    //encrypt_file("stuff.txt",key);
    //decrypt_file("encrypt.txt",key);

    return 0;
}