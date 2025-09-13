#include "Encoder.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

void Encoder::start(){
    compute_prob();
    if(prob_vec_.empty()){
        std::cerr << "LOG: Encoder:start: Error: prob_vec_ is empty\n";
        throw std::runtime_error("Encoder::start: prob_vec_ is empty");
    }
    fill_dict(0, prob_vec_.size() - 1);
    text_encode();
}

void Encoder::compute_prob(){
    auto total = static_cast<double>(compute_frec());
    if(total == 0){
        std::cout << "LOG: Encoder:compute_prob: total = 0\n";
        return;
    }
    for(size_t i = 0; i < frec_dict_.size(); ++i){
        if(frec_dict_[i] != 0){
            prob_vec_.push_back({static_cast<unsigned char>(i), frec_dict_[i] / total});
        }
    }

    std::sort(prob_vec_.begin(), prob_vec_.end()
        , [](auto& a, auto& b)
        { return a.second > b.second; });
}

unsigned Encoder::compute_frec(){
    std::ifstream file;
    file.open(input_file_path_);
    if(!file.is_open()){
        std::cerr << "LOG: Error in opening file " << input_file_path_ << std::endl;
        throw std::runtime_error("Error in opening file");
    }
    char ch;
    unsigned count = 0;
    while(file.get(ch)){
        ++frec_dict_[ch];
        ++count;
    }
    return count;
}

void Encoder::fill_dict(size_t beg, size_t end){
    if(end > beg){
        auto med = find_med(beg, end);
        for(size_t i = beg; i <= end; ++i){
            dict_[prob_vec_[i].first] += std::to_string(static_cast<int>((i >= med)));
        }
        fill_dict(beg, med - 1);
        fill_dict(med, end);
    }
}

size_t Encoder::find_med(size_t beg, size_t end){
    double right_sum = 0;
    for(auto i = beg + 1; i <= end; ++i){
        right_sum += prob_vec_[i].second;
    }
    double left_sum = prob_vec_[beg].second;
    size_t med = beg;

    auto dif = fabs(left_sum - right_sum);
    do{
        dif = fabs(left_sum - right_sum);
        ++med;
        left_sum += prob_vec_[med].second;
        right_sum -= prob_vec_[med].second;
    }while(dif > fabs(left_sum - right_sum));

    return med;
}

void Encoder::write_alphabet(std::ofstream& output_file){
    output_file << prob_vec_.size() << std::endl;
    for(size_t i = 0; i < dict_.size(); ++i){
        if(dict_[i] != ""){
            output_file << format_symbol(static_cast<unsigned char>(i)) << " " << dict_[i] << std::endl;
        }
    }
}

std::string Encoder::format_symbol(unsigned char c) {
    // printable non-space -> single char
    if (std::isprint(c) && !std::isspace(c)) {
        return std::string(1, static_cast<char>(c));
    }

    // Escape-seq
    switch (c) {
        case '\n': return std::string("'\\n'");   // newline
        case '\t': return std::string("'\\t'");   // tab
        case '\r': return std::string("'\\r'");   // carriage return
        case '\\': return std::string("'\\\\'");  // backslash
        case '\'': return std::string("'\\''");   // single quote
        case '\0': return std::string("'\\0'");   // NUL
        default:
            return std::to_string(static_cast<int>(c));
    }
}


void Encoder::text_encode(){
    //TODO: Добавить вывод в файл алфавита фано
    std::ifstream input_file;
    std::ofstream output_file;
    input_file.open(input_file_path_);
    if(!input_file.is_open()){
        std::cerr << "LOG: Error in opening file " << input_file_path_ << std::endl;
        throw std::runtime_error("Error in opening file");
    }
    output_file.open(output_file_path_);
    if(!output_file.is_open()){
        std::cerr << "LOG: Error in opening file " << output_file_path_ << std::endl;
        throw std::runtime_error("Error in opening file");
    }

    write_alphabet(output_file);
    char ch;
    int i = -1;
    while(input_file.get(ch)){
        unsigned char u_ch = static_cast<unsigned char>(ch);
        std::string& code = dict_[u_ch];
        if(code.empty()){
            std::cerr << "LOG: ERROR: no such symbol in dictionary: " << u_ch << std::endl;
            throw  std::runtime_error("Error in encoding");
        }
        if(++i < cout_number){
            std::cout << dict_[u_ch];
        }
        output_file << dict_[u_ch];
    }

    input_file.close();
    output_file.close();
}

void Encoder::bit_encode(){
    std::ifstream input_file(input_file_path_, std::ios::binary);
    std::ofstream output_file(output_file_path_, std::ios::binary);
    if(!input_file.is_open()){
        std::cerr << "LOG: Error in opening file " << input_file_path_ << std::endl;
        throw std::runtime_error("Error in opening file");
    }
    if(!output_file.is_open()){
        std::cerr << "LOG: Error in opening file " << output_file_path_ << std::endl;
        throw std::runtime_error("Error in opening file");
    }
    write_alphabet(output_file);

    // Reserve place for padding in header of file
    // It allow to correct decode from binary
    // We consider the padding in the last byte
    uint8_t padding = 0;
    output_file.write(reinterpret_cast<char const *>(&padding), sizeof(padding));


    // Current symbol in bit appearance
    uint8_t current = 0;
    // Number of fiiled (efficient) bits in byte
    size_t filled = 0;

    char ch;
    int i = -1;
    while(input_file.get(ch)){
        unsigned char u_ch = static_cast<unsigned char>(ch);
        std::string & code = dict_[u_ch];
        if(code.empty()){
            std::cerr << "LOG: ERROR: no such symbol in dictionary: " << u_ch << std::endl;
            throw  std::runtime_error("Error in encoding");
        }
        if(++i < cout_number){
            std::cout << dict_[u_ch];
        }

        // Transform string into unit8_t - create bit appearance
        for(auto& c : code){
            if(c == '1'){
                current |= 1;
            }
            current <<= 1;
            ++filled;

            // If current is full, put it to file
            if(filled == sizeof(current) * 8){
                output_file.put(static_cast<char>(current));
                current = 0;
                filled = 0;
            }
        }
    }

    if(filled != 0){
        padding = sizeof(current) * 8 - filled;
        current <<= padding -1;
        output_file.put(static_cast<char>(current));
        filled = 0;
        current = 0;
    }

    output_file.seekp(0, std::ios::beg);
    output_file.write(reinterpret_cast<char const *>(&padding), sizeof(padding));
}