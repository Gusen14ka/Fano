#include <array>
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

class Encoder{
public:
    Encoder(std::string input_path, std::string output_path = "decoded.txt")
        : input_file_path_(input_path), output_file_path_(output_path) {}

    void start();

    void convert_to_binary();

    static std::string format_symbol(unsigned char c);
private:
    std::string input_file_path_;
    std::string output_file_path_;
    std::array<std::string, 256> dict_{};
    std::array<unsigned, 256> frec_dict_{};
    std::vector<std::pair<unsigned char, double>> prob_vec_;

    int cout_number = 10;

    void compute_prob();

    unsigned compute_frec();

    void fill_dict(size_t beg, size_t end);

    size_t find_med(size_t beg, size_t end);

    void text_encode();

    void write_alphabet(std::ofstream& output_file);
};