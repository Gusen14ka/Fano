#include <vector>
#include <fstream>
#include <string>

class UniDecoder{
public:
    UniDecoder(std::string input_path_text, std::string input_path_alphabet,
    std::string output_path = "encoded.txt")
        : input_path_text_(input_path_text),
        input_path_alphabet_(input_path_alphabet), output_path_(output_path) {}

    void start();

private:
    std::string input_path_text_;
    std::string input_path_alphabet_;
    std::string output_path_;

    // Number of symbols to cout while decoding
    int cout_number_ = 10;

    // Lenght of code for each symbol
    unsigned int length_ = 0;

    // Index is code that transformed into unsigned int, unsigned char - symbol
    std::vector<int> codeToSymb_ {std::vector<int>(256, -1)};

    // Read alhpabet
    void read_alphabet(std::ifstream& input_file);

    // Decode text
    void bit_decode(std::ifstream& input_file);

    // Transform code string to unsigned int (MSB-first)
    unsigned int code_string_to_uint(const std::string &s);

    
};