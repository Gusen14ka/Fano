#include <array>
#include <cstddef>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

class Encoder{
public:
    Encoder(std::string input_path, std::string output_path = "decoded.txt")
        : input_file_path_(input_path), output_file_path_(output_path) {}

    void start();
private:
    std::string input_file_path_;
    std::string output_file_path_;
    std::array<std::string, 256> dict_{};
    std::array<unsigned, 256> frec_dict_{};
    std::vector<std::pair<unsigned char, double>> prob_vec_;

    // Number of symbols to cout while encoding
    int cout_number = 10;

    // Fill prob_vec_ with probability of chars in input text
    void compute_prob();

    // Fill frec_dict_ with frequencies of char in input text
    unsigned compute_frec();

    // Fill dict_ (index = input char, value = encoded char)
    void fill_dict(size_t beg, size_t end);

    // Find median index
    size_t find_med(size_t beg, size_t end);

    // Encode text to text format file
    void text_encode();

    // Encode text to binary (bit) format file
    void bit_encode();

    // Write fano alphabet to file;
    void write_alphabet(std::ofstream& output_file);

    // Format (pritable and non-printable) symbol (for alphabet)
    std::string format_symbol(unsigned char c);
};