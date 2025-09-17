#include "Decoder.hpp"
#include "Logger.hpp"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <memory>
#include <stdexcept>

#define LOG Logger::getInstance()

void Decoder::start(){
    LOG.info("Starting decoder for file: " + input_file_path_, "Decoder::start");

    std::ifstream input_file(input_file_path_);
    if(!input_file.is_open()){
        LOG.error("Error in opening file " + input_file_path_, "Decoder::start");
        throw std::runtime_error("Error in opening file");
    }

    read_alphabet(input_file);
    if(match_vec_.empty()){
        LOG.error("match_vec_ is empty", "Decoder::start");
        throw std::runtime_error("Decoder::start: match_vec_ is empty");
    }

    LOG.info("Building decoding tree", "Decoder::start");
    tree_ = make_tree(0, match_vec_.size() - 1, 0);

    LOG.info("Starting text decoding", "Decoder::start");
    decode_text(input_file);

    LOG.info("Decoding completed successfully", "Decoder::start");
}

void Decoder::read_alphabet(std::ifstream& input_file){
    if(!input_file.is_open()){
        LOG.error("Error in opening file " + input_file_path_, "Decoder::read_alphabet");
        throw std::runtime_error("Error in opening file");
    }

    size_t n = 0;
    input_file >> n;
    LOG.info("Reading alphabet with " + std::to_string(n) + " symbols", "Decoder::read_alphabet");

    input_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    for(size_t i = 0; i < n; ++i){
        std::string line;
        std::getline(input_file, line);

        if (line.empty()) continue;

        size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos) {
            LOG.error("Invalid format in alphabet line: " + line, "Decoder::read_alphabet");
            throw std::runtime_error("Invalid alphabet format");
        }

        std::string token = line.substr(0, space_pos);
        std::string code = line.substr(space_pos + 1);

        try {
            unsigned char symbol = parse_symbol_token(token);
            match_vec_.emplace_back(symbol, code);
            LOG.debug("Symbol: " + token + " -> Code: " + code, "Decoder::read_alphabet");
        } catch (const std::exception& e) {
            LOG.error("Failed to parse symbol token: " + token + " - " + e.what(), "Decoder::read_alphabet");
            throw;
        }
    }

    std::sort(match_vec_.begin(), match_vec_.end(), [](auto const & p1, auto const & p2){
        return p1.second < p2.second;
    });

    LOG.info("Alphabet read and sorted successfully", "Decoder::read_alphabet");
}

std::unique_ptr<Node> Decoder::make_tree(size_t beg, size_t end, size_t rang){

    if(beg > end) return nullptr;

    if(beg == end){
        LOG.debug("Creating leaf node for symbol: " + std::string(1, match_vec_[beg].first),
                 "Decoder::make_tree");
        return std::make_unique<Node>(Node(match_vec_[beg].first, true));
    }

    if(match_vec_[beg].second.size() <= rang){
        std::string error_msg = "Oversize rang " + std::to_string(rang) + " " +
                               match_vec_[beg].second + " " + match_vec_[beg + 1].second;
        LOG.error(error_msg, "Decoder::make_tree");
        throw std::runtime_error(error_msg);
    }

    size_t med = find_med(beg, end, rang);
    auto node = std::make_unique<Node>(Node());

    LOG.debug("Creating node at range [" + std::to_string(beg) + "-" + std::to_string(end) +
             "], median: " + std::to_string(med), "Decoder::make_tree");

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
    LOG.warning("No '1' found at rang " + std::to_string(rang) +
               " in range [" + std::to_string(beg) + "-" + std::to_string(end) + "]",
               "Decoder::find_med");
    return end + 1;
}

void Decoder::decode_text(std::ifstream& input_file){
    if (!tree_) {
        LOG.error("Decoding tree is nullptr", "Decoder::decode_text");
        throw std::runtime_error("Decoder::decode_text: decoding tree is not built");
    }

    if(!input_file.is_open()){
        LOG.error("Error in opening file " + input_file_path_, "Decoder::decode_text");
        throw std::runtime_error("Decoder::decode_text: Error in opening file");
    }

    std::ofstream output_file(output_file_path_);
    if(!output_file.is_open()){
        LOG.error("Error in opening file " + output_file_path_, "Decoder::decode_text");
        throw std::runtime_error("Decoder::decode_text: Error in opening file");
    }

    LOG.info("Starting text decoding", "Decoder::decode_text");

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
            LOG.error("Null node encountered during decoding", "Decoder::decode_text");
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
        LOG.error("Decoding ended in non-root node", "Decoder::decode_text");
        throw std::runtime_error("Error in decode");
    }

    LOG.info("Text decoding completed. Symbols decoded: " + std::to_string(i),
             "Decoder::decode_text");
}

unsigned char Decoder::parse_symbol_token(const std::string &token_raw) {
    if (token_raw.empty()) {
        LOG.error("Empty token received", "Decoder::parse_symbol_token");
        throw std::runtime_error("empty token");
    }

    std::string token = token_raw;

    if (token.size() == 1 && !std::isspace(static_cast<unsigned char>(token[0]))) {
        LOG.debug("Parsed printable symbol: " + token, "Decoder::parse_symbol_token");
        return static_cast<unsigned char>(token[0]);
    }

    if (token.size() >= 3 && token.front() == '\'' && token.back() == '\'') {
        std::string inner = token.substr(1, token.size() - 2);
        if (inner.empty()) {
            LOG.error("Empty quoted token", "Decoder::parse_symbol_token");
            throw std::runtime_error("empty quoted token");
        }

        if (inner.size() == 2 && inner[0] == '\\') {
            char esc = inner[1];
            unsigned char result;
            switch (esc) {
                case 'n': result = '\n'; break;
                case 't': result = '\t'; break;
                case 'r': result = '\r'; break;
                case '\\': result = '\\'; break;
                case '\'': result = '\''; break;
                case '0': result = 0; break;
                default:
                    LOG.error("Unsupported escape sequence: \\" + std::string(1, esc),
                             "Decoder::parse_symbol_token");
                    throw std::runtime_error(std::string("unsupported escape: \\") + esc);
            }
            LOG.debug("Parsed escape sequence: " + token + " -> " + std::to_string(result),
                     "Decoder::parse_symbol_token");
            return result;
        }

        LOG.error("Unsupported quoted token: " + token, "Decoder::parse_symbol_token");
        throw std::runtime_error("unsupported quoted token content");
    }

    char *endptr = nullptr;
    long val = std::strtol(token.c_str(), &endptr, 0);
    if (endptr != token.c_str() && *endptr == '\0') {
        if (val < 0 || val > 255) {
            LOG.error("Numeric symbol out of range: " + token, "Decoder::parse_symbol_token");
            throw std::runtime_error("numeric symbol out of range 0..255");
        }
        LOG.debug("Parsed numeric symbol: " + token + " -> " + std::to_string(val),
                 "Decoder::parse_symbol_token");
        return static_cast<unsigned char>(val);
    }

    LOG.error("Invalid symbol token: " + token, "Decoder::parse_symbol_token");
    throw std::runtime_error("invalid symbol token: " + token);
}