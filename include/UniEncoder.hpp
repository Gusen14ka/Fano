#include <cstddef>
#include <fstream>
#include <string>
#include <array>

class UniEncoder{
public:
    UniEncoder(std::string input_path, std::string output_path_text = "encoded.bin",
    std::string output_path_alphabet = "encoded_alphabet.txt")
        : input_path_(input_path), output_path_text_(output_path_text),
        output_path_alphabet_(output_path_alphabet) {}
    
    void start();
private:
    std::string input_path_;
    std::string output_path_text_;
    std::string output_path_alphabet_;

    // Index is unsigned char symbol, string - code
    std::array<std::string, 256> symbToCode_ {};

    // Index is unsigned char symbol, unsigned int - its number in text
    std::array<unsigned, 256> chars_ {};

    // Lenght of code for each symbol
    unsigned int length_ = 0;

    // Number of different symbols in text
    size_t symb_num_ = 0;

    // Number of symbols to cout while encoding
    int cout_number = 10;

    // Fill symbToCode
    void make_alphabet();

    // Fill chars
    void fill_chars();

    // Encode sigle symbol
    // index is symbol's sequence number
    void encode_sigle_symbol(unsigned index, std::string& buf);

    // Write the alphabet
    void write_alphabet(std::ofstream& output_file);

    // Encode text to binary (bit) format file
    void bit_encode();
};