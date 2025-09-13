#include "Decoder.hpp"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <memory>
#include <stdexcept>

void Decoder::read_alphabet(std::ifstream& input_file){
    if(!input_file.is_open()){
        std::cerr << "LOG: Error in opening file " << input_file_path_ << std::endl;
        throw std::runtime_error("Error in opening file");
    }

    size_t n = 0;
    input_file >> n;
    for(size_t i = 0; i < n; ++i){
        std::string code, token;
        input_file >> token >> code;
        match_vec_.emplace_back(parse_symbol_token(token), code);
    }

    std::sort(match_vec_.begin(), match_vec_.end(), [](auto& p1, auto& p2){
        if(!(p1.second.size() == p2.second.size())){
            return p1.second.size() < p2.second.size();
        }
        return p1.second < p2.second;
    });
}

std::unique_ptr<Node> Decoder::make_tree(size_t beg, size_t end, size_t rang){
    
    if(beg > end) return nullptr;
    // Found leaf
    if(beg == end){
        return std::make_unique<Node>(Node(match_vec_[beg].first, true));
    }

    if(match_vec_[beg].second.size() <= rang){
        std::cerr << "Oversize rang " << rang << " " << match_vec_[beg].second << " " << match_vec_[beg + 1].second;
        throw std::runtime_error("Oversize rang");
    }
    
    size_t med = find_med(beg, end, rang);
    auto node = std::make_unique<Node>(Node());
    if(med == beg){
        node->left = nullptr;
        node->right = make_tree(beg, end, rang + 1);
    }
    else if(med == end + 1){
        node->left = make_tree(beg, end, rang + 1);
        node->right = nullptr;
    }
    else{
        node->left = make_tree(beg, med - 1, rang + 1);
        node->right = make_tree(med, end, rang + 1);
    }
    return node;
}

size_t Decoder::find_med(size_t beg, size_t end, size_t rang){
    for(size_t i = beg; i <= end; ++i){
        if(rang < match_vec_[i].second.size() && match_vec_[i].second[rang] == '1'){
            return i;
        }
    }
    std::cout << match_vec_[beg].second << " " << match_vec_[end].second << std::endl;
    return end + 1;
}

void Decoder::decode_text(std::ifstream& input_file){
    if (!tree_) {
        std::cerr << "LOG: Error in decoding: tree_ is nullptr\n";
        throw std::runtime_error("Decoder::decode_text: decoding tree is not built");
    }

    if(!input_file.is_open()){
        std::cerr << "LOG: Error in opening file " << input_file_path_ << std::endl;
        throw std::runtime_error("Decoder::decode_text: Error in opening file");
    }
    std::ofstream output_file(output_file_path_);
    if(!output_file.is_open()){
        std::cerr << "LOG: Error in opening file " << output_file_path_ << std::endl;
        throw std::runtime_error("Decoder::decode_text: Error in opening file");
    }
    
    char ch;
    size_t i = 0;
    Node* cur_node = tree_.get();
    while(input_file.get(ch)){
        if (ch != '0' && ch != '1') continue;
        if(ch == '1'){
            cur_node = cur_node->right.get();
        }
        else{
            cur_node = cur_node->left.get();
        }
        if(!cur_node){
            std::cerr << "LOG: Error in decode - there isn't target leaf in tree\n";
            throw std::runtime_error("Error in decode");
        }
        if(cur_node->is_leaf){
            output_file.put(cur_node->symbol);
            if(++i <= cout_number){
                std::cout << cur_node->symbol;
            }
            cur_node = tree_.get();
        }
    }
    if(cur_node != tree_.get()){
        std::cerr << "LOG: Error in decode - there isn't target leaf in tree\n";
        throw std::runtime_error("Error in decode");
    }
}

void Decoder::start(){
    std::ifstream input_file(input_file_path_);
    if(!input_file.is_open()){
        std::cerr << "LOG: Error in opening file " << input_file_path_ << std::endl;
        throw std::runtime_error("Error in opening file");
    }

    read_alphabet(input_file);
    if(match_vec_.empty()){
        std::cerr << "LOG: Decoder:start: Error: mathc_vec_ is empty\n";
        throw std::runtime_error("Decoder::start: mathc_vec_ is empty");
    }
    tree_ = make_tree(0, match_vec_.size() - 1, 0);
    decode_text(input_file);
}

unsigned char Decoder::parse_symbol_token(const std::string &tok_raw) {
    if (tok_raw.empty()) throw std::runtime_error("empty token");

    std::string tok = tok_raw;

    // 1) single printable non-space char
    if (tok.size() == 1 && !std::isspace(static_cast<unsigned char>(tok[0]))) {
        return static_cast<unsigned char>(tok[0]);
    }

    // 2) quoted escape form: must be '...'
    if (tok.size() >= 3 && tok.front() == '\'' && tok.back() == '\'') {
        std::string inner = tok.substr(1, tok.size() - 2);
        if (inner.empty()) throw std::runtime_error("empty quoted token");

        // if exactly an escape like \n, \t, \\ , \' , \0
        if (inner.size() == 2 && inner[0] == '\\') {
            char esc = inner[1];
            switch (esc) {
                case 'n': return static_cast<unsigned char>('\n');
                case 't': return static_cast<unsigned char>('\t');
                case 'r': return static_cast<unsigned char>('\r');
                case '\\': return static_cast<unsigned char>('\\');
                case '\'': return static_cast<unsigned char>('\'');
                case '0': return static_cast<unsigned char>(0);
                default:
                    throw std::runtime_error(std::string("unsupported escape: \\") + esc);
            }
        }
        // if inner is a single non-escape character: e.g. ' ' (space inside quotes is allowed)
        if (inner.size() == 1) {
            return static_cast<unsigned char>(inner[0]);
        }

        throw std::runtime_error("unsupported quoted token content");
    }

    // 3) numeric representation (decimal or hex 0x..)
    char *endptr = nullptr;
    long val = std::strtol(tok.c_str(), &endptr, 0); // base 0: supports 0x... too
    if (endptr != tok.c_str() && *endptr == '\0') {
        if (val < 0 || val > 255) throw std::runtime_error("numeric symbol out of range 0..255");
        return static_cast<unsigned char>(val);
    }

    throw std::runtime_error("invalid symbol token: " + tok);
}
