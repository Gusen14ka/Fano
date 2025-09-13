#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct Node{
    Node(char c = '\0', bool leaf = false) : symbol(c), is_leaf(leaf){}
    char symbol;
    std::unique_ptr<Node> left = nullptr;
    std::unique_ptr<Node> right = nullptr;
    bool is_leaf;
};

class Decoder{
public:
    Decoder(std::string input_path, std::string output_path = "encoded.txt")
        : input_file_path_(input_path), output_file_path_(output_path) {}

    void start();
private:
    std::string input_file_path_;
    std::string output_file_path_;
    std::unique_ptr<Node> tree_;

    // Number of symbols to cout while decoding
    int cout_number = 10;

    // Vector of matching code and symbol pairs
    std::vector<std::pair<unsigned char, std::string>> match_vec_;

    // Read Fano alphabet from file
    void read_alphabet(std::ifstream& input_file);

    // Make Fano tree for efficient decoding
    std::unique_ptr<Node> make_tree(size_t beg, size_t end, size_t rang);

    // Find median indx
    size_t find_med(size_t beg, size_t end, size_t rang);

    // Decode the text
    void decode_text(std::ifstream& input_file);

    // Parse format symbols (for alphabet)
    unsigned char parse_symbol_token(const std::string &tok_raw);
};