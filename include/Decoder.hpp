#include <cstddef>
#include <memory>
#include <string>
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
    Decoder(std::string input_path_text, std::string input_path_alphabet,
    std::string output_path = "encoded.txt")
        : input_path_text_(input_path_text),
        input_path_alphabet_(input_path_alphabet), output_path_(output_path) {}

    void start();

    static unsigned char parse_symbol_token(const std::string &token_raw);
private:
    std::string input_path_text_;
    std::string input_path_alphabet_;
    std::string output_path_;
    std::unique_ptr<Node> tree_;

    int cout_number = 10;

    std::vector<std::pair<unsigned char, std::string>> match_vec_;

    void read_alphabet(std::ifstream& input_file);

    std::unique_ptr<Node> make_tree(size_t beg, size_t end, size_t rang);

    size_t find_med(size_t beg, size_t end, size_t rang);

    void decode_text(std::ifstream& input_file);

    void bit_decode();
};