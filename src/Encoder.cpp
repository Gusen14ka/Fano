#include "Encoder.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

#define LOG Logger::getInstance()

void Encoder::start(){
    LOG.info("Starting encoder for file: " + input_file_path_, "Encoder::start");

    compute_prob();
    if(prob_vec_.empty()){
        LOG.error("prob_vec_ is empty", "Encoder::start");
        throw std::runtime_error("Encoder::start: prob_vec_ is empty");
    }

    LOG.info("Building Fano dictionary", "Encoder::start");
    fill_dict(0, prob_vec_.size() - 1);

    LOG.info("Starting text encoding", "Encoder::start");
    text_encode();

    LOG.info("Encoding completed successfully", "Encoder::start");
}

void Encoder::compute_prob(){
    auto total = static_cast<double>(compute_frec());
    if(total == 0){
        LOG.warning("Total symbols count is 0", "Encoder::compute_prob");
        return;
    }

    LOG.info("Computing probabilities for " + std::to_string(total) + " symbols",
             "Encoder::compute_prob");

    for(size_t i = 0; i < frec_dict_.size(); ++i){
        if(frec_dict_[i] != 0){
            prob_vec_.push_back({static_cast<unsigned char>(i), frec_dict_[i] / total});
        }
    }

    std::sort(prob_vec_.begin(), prob_vec_.end()
        , [](auto& a, auto& b)
        { return a.second > b.second; });

    LOG.info("Probability computation completed. Unique symbols: " +
             std::to_string(prob_vec_.size()), "Encoder::compute_prob");
}

unsigned Encoder::compute_frec(){
    std::ifstream file;
    file.open(input_file_path_);
    if(!file.is_open()){
        LOG.error("Error in opening file " + input_file_path_, "Encoder::compute_frec");
        throw std::runtime_error("Error in opening file");
    }

    char ch;
    unsigned count = 0;
    while(file.get(ch)){
        ++frec_dict_[ch];
        ++count;
    }

    LOG.info("Frequency computed. Total symbols: " + std::to_string(count),
             "Encoder::compute_frec");
    return count;
}

void Encoder::fill_dict(size_t beg, size_t end){
    if(end > beg){
        auto med = find_med(beg, end);
        LOG.debug("Filling dictionary for range [" + std::to_string(beg) + "-" +
                 std::to_string(end) + "], median: " + std::to_string(med),
                 "Encoder::fill_dict");

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

    LOG.debug("Found median: " + std::to_string(med) + " for range [" +
             std::to_string(beg) + "-" + std::to_string(end) + "]",
             "Encoder::find_med");

    return med;
}

void Encoder::write_alphabet(std::ofstream& output_file){
    output_file << prob_vec_.size() << std::endl;
    LOG.info("Writing alphabet with " + std::to_string(prob_vec_.size()) + " symbols",
             "Encoder::write_alphabet");

    for(size_t i = 0; i < dict_.size(); ++i){
        if(dict_[i] != ""){
            output_file << format_symbol(static_cast<unsigned char>(i)) << " " << dict_[i] << std::endl;
            LOG.debug("Symbol: " + format_symbol(static_cast<unsigned char>(i)) + " -> Code: " + dict_[i],
                     "Encoder::write_alphabet");
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
        case '\n': return std::string("'\\n'");
        case '\t': return std::string("'\\t'");
        case '\r': return std::string("'\\r'");
        case '\\': return std::string("'\\\\'");
        case '\'': return std::string("'\\''");
        case '\0': return std::string("'\\0'");
        default:
            return std::to_string(static_cast<int>(c));
    }
}

void Encoder::text_encode(){
    std::ifstream input_file;
    std::ofstream output_file;
    input_file.open(input_file_path_);
    if(!input_file.is_open()){
        LOG.error("Error in opening input file " + input_file_path_, "Encoder::text_encode");
        throw std::runtime_error("Error in opening file");
    }
    output_file.open(output_file_path_);
    if(!output_file.is_open()){
        LOG.error("Error in opening output file " + output_file_path_, "Encoder::text_encode");
        throw std::runtime_error("Error in opening file");
    }

    write_alphabet(output_file);
    char ch;
    int i = -1;
    size_t encoded_count = 0;

    while(input_file.get(ch)){
        unsigned char u_ch = static_cast<unsigned char>(ch);
        std::string& code = dict_[u_ch];
        if(code.empty()){
            LOG.error("No code found for symbol: " + std::to_string(u_ch), "Encoder::text_encode");
            throw std::runtime_error("Error in encoding");
        }
        if(++i < cout_number){
            std::cout << dict_[u_ch];
        }
        output_file << dict_[u_ch];
        encoded_count++;
    }

    input_file.close();
    output_file.close();

    LOG.info("Text encoding completed. Symbols encoded: " + std::to_string(encoded_count),
             "Encoder::text_encode");
}

void Encoder::bit_encode(){
    std::ifstream input_file(input_file_path_, std::ios::binary);
    std::ofstream output_file(output_file_path_, std::ios::binary);
    if(!input_file.is_open()){
        LOG.error("Error in opening input file " + input_file_path_, "Encoder::bit_encode");
        throw std::runtime_error("Error in opening file");
    }
    if(!output_file.is_open()){
        LOG.error("Error in opening output file " + output_file_path_, "Encoder::bit_encode");
        throw std::runtime_error("Error in opening file");
    }

    write_alphabet(output_file);

    uint8_t padding = 0;
    output_file.write(reinterpret_cast<char const *>(&padding), sizeof(padding));

    uint8_t current = 0;
    size_t filled = 0;
    char ch;
    int i = -1;
    size_t encoded_count = 0;

    while(input_file.get(ch)){
        unsigned char u_ch = static_cast<unsigned char>(ch);
        std::string & code = dict_[u_ch];
        if(code.empty()){
            LOG.error("No code found for symbol: " + std::to_string(u_ch), "Encoder::bit_encode");
            throw std::runtime_error("Error in encoding");
        }
        if(++i < cout_number){
            std::cout << dict_[u_ch];
        }

        for(auto& c : code){
            if(c == '1'){
                current |= 1;
            }
            current <<= 1;
            ++filled;

            if(filled == sizeof(current) * 8){
                output_file.put(static_cast<char>(current));
                current = 0;
                filled = 0;
            }
        }
        encoded_count++;
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

    LOG.info("Bit encoding completed. Symbols encoded: " + std::to_string(encoded_count) +
             ", padding: " + std::to_string(padding), "Encoder::bit_encode");
}