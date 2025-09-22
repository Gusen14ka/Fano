#include "Encoder.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

#define LOG Logger::getInstance()

void Encoder::start() {
    LOG.info("Starting encoder for file: " + input_path_, "Encoder::start");

    compute_prob();
    if (prob_vec_.empty()) {
        LOG.error("prob_vec_ is empty", "Encoder::start");
        throw std::runtime_error("Encoder::start: prob_vec_ is empty");
    }

    LOG.info("Building Fano dictionary", "Encoder::start");
    if(prob_vec_.size() == 1){
        dict_[prob_vec_[0].first] = "1";
    }
    else{
        fill_dict(0, prob_vec_.size() - 1);
    }

    LOG.info("Starting text encoding", "Encoder::start");
    bit_encode();

    LOG.info("Encoding completed successfully", "Encoder::start");
}

void Encoder::compute_prob() {
    auto total = static_cast<double>(compute_frec());
    if (total == 0) {
        LOG.warning("Total symbols count is 0", "Encoder::compute_prob");
        return;
    }

    LOG.info("Computing probabilities for " + std::to_string(total) + " symbols",
             "Encoder::compute_prob");

    for (size_t i = 0; i < frec_dict_.size(); ++i) {
        if (frec_dict_[i] != 0) {
            prob_vec_.push_back({static_cast<unsigned char>(i), frec_dict_[i] / total});
        }
    }

    if (prob_vec_.size() == 1) {
        LOG.info("Only one unique symbol found", "Encoder::compute_prob");
        dict_[prob_vec_[0].first] = "0";
        return;
    }

    std::sort(prob_vec_.begin(), prob_vec_.end(), [](auto &a, auto &b) { return a.second > b.second; });

    LOG.info("Probability computation completed. Unique symbols: " +
             std::to_string(prob_vec_.size()), "Encoder::compute_prob");
}

unsigned Encoder::compute_frec() {
    std::ifstream file;
    file.open(input_path_);
    if (!file.is_open()) {
        LOG.error("Error in opening file " + input_path_, "Encoder::compute_frec");
        throw std::runtime_error("Error in opening file");
    }

    char ch;
    unsigned count = 0;
    while (file.get(ch)) {
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
        for(size_t i = beg; i <= end; ++i){
            dict_[prob_vec_[i].first] += std::to_string(static_cast<int>((i >= med)));
        }
        fill_dict(beg, med - 1);
        fill_dict(med, end);
    }
}

size_t Encoder::find_med(size_t beg, size_t end) {
    if (beg == end) {
        return beg;
    }

    double right_sum = 0;
    for (auto i = beg + 1; i <= end; ++i) {
        right_sum += prob_vec_[i].second;
    }
    double left_sum = prob_vec_[beg].second;
    size_t med = beg;

    auto dif = fabs(left_sum - right_sum);
    do {
        dif = fabs(left_sum - right_sum);
        ++med;
        left_sum += prob_vec_[med].second;
        right_sum -= prob_vec_[med].second;
    } while (med < end && dif > fabs(left_sum - right_sum));

    LOG.debug("Found median: " + std::to_string(med) + " for range [" +
              std::to_string(beg) + "-" + std::to_string(end) + "]",
              "Encoder::find_med");

    return med;
}

void Encoder::write_alphabet(std::ofstream &output_file) {
    output_file << prob_vec_.size() << std::endl;
    LOG.info("Writing alphabet with " + std::to_string(prob_vec_.size()) + " symbols",
             "Encoder::write_alphabet");

    for (size_t i = 0; i < dict_.size(); ++i) {
        if (dict_[i] != "") {
            output_file << format_symbol(static_cast<unsigned char>(i)) << " " << dict_[i] << std::endl;
            LOG.debug("Symbol: " + format_symbol(static_cast<unsigned char>(i)) + " -> Code: " + dict_[i],
                      "Encoder::write_alphabet");
        }
    }
}

std::string Encoder::format_symbol(unsigned char c) {
    if (std::isprint(c) && !std::isspace(c)) {
        return std::string(1, static_cast<char>(c));
    }

    switch (c) {
        case '\n': return std::string("'\\n'");
        case '\t': return std::string("'\\t'");
        case '\r': return std::string("'\\r'");
        case '\\': return std::string("'\\\\'");
        case '\'': return std::string("'\\''");
        case '\0': return std::string("'\\0'");
        default:
            return '\'' +std::to_string(static_cast<int>(c)) + '\'';
    }
}

void Encoder::text_encode() {
    std::ifstream input_file;
    input_file.open(input_path_);
    if (!input_file.is_open()) {
        LOG.error("Error in opening input file " + input_path_, "Encoder::text_encode");
        throw std::runtime_error("Error in opening file");
    }

    std::ofstream output_text;
    output_text.open(output_path_text_);
    if (!output_text.is_open()) {
        LOG.error("Error in opening output file " + output_path_text_, "Encoder::text_encode");
        throw std::runtime_error("Error in opening file");
    }

    std::ofstream output_alphabet;
    output_alphabet.open(output_path_alphabet_);
    if (!output_text.is_open()) {
        LOG.error("Error in opening output file " + output_path_alphabet_, "Encoder::text_encode");
        throw std::runtime_error("Error in opening file");
    }

    write_alphabet(output_alphabet);
    char ch;
    int i = -1;
    size_t encoded_count = 0;

    while (input_file.get(ch)) {
        unsigned char u_ch = static_cast<unsigned char>(ch);
        std::string &code = dict_[u_ch];
        if (code.empty()) {
            LOG.error("No code found for symbol: " + std::to_string(u_ch), "Encoder::text_encode");
            throw std::runtime_error("Error in encoding");
        }
        if (++i < cout_number) {
            std::cout << dict_[u_ch];
        }
        output_text << dict_[u_ch];
        encoded_count++;
    }

    input_file.close();
    output_text.close();

    LOG.info("Text encoding completed. Symbols encoded: " + std::to_string(encoded_count),
             "Encoder::text_encode");
}

/*
void Encoder::convert_to_binary() {
    std::ifstream text_encoded_file(output_file_path_);
    if (!text_encoded_file.is_open()) {
        LOG.error("Error opening encoded file: " + output_file_path_, "Encoder::convert_to_binary");
        throw std::runtime_error("Cannot open encoded file for binary conversion");
    }

    std::string binary_output_path = output_file_path_ + ".encoded";
    std::ofstream binary_file(binary_output_path, std::ios::binary);
    if (!binary_file.is_open()) {
        LOG.error("Error creating binary file: " + binary_output_path, "Encoder::convert_to_binary");
        throw std::runtime_error("Cannot create binary output file");
    }

    size_t n = 0;
    text_encoded_file >> n;

    std::string line;
    for (size_t i = 0; i < n; ++i) {
        std::getline(text_encoded_file, line);
    }

    std::string encoded_data;
    char ch;
    while (text_encoded_file.get(ch)) {
        if (ch == '0' || ch == '1') {
            encoded_data += ch;
        }
    }

    if (encoded_data.empty()) {
        LOG.error("No encoded data found in file", "Encoder::convert_to_binary");
        throw std::runtime_error("No encoded data to convert");
    }

    LOG.info("Converting " + std::to_string(encoded_data.size()) + " bits to binary",
             "Encoder::convert_to_binary");

    uint8_t current_byte = 0;
    size_t bit_count = 0;
    size_t total_bits = encoded_data.size();

    for (size_t i = 0; i < total_bits; ++i) {
        current_byte <<= 1;
        if (encoded_data[i] == '1') {
            current_byte |= 1;
        }
        bit_count++;

        if (bit_count == 8) {
            binary_file.put(static_cast<char>(current_byte));
            current_byte = 0;
            bit_count = 0;
        }
    }

    if (bit_count > 0) {
        current_byte <<= (8 - bit_count);
        binary_file.put(static_cast<char>(current_byte));

        std::ofstream info_file(output_file_path_ + ".info");
        info_file << bit_count;
        info_file.close();

        LOG.debug("Last byte has " + std::to_string(bit_count) + " significant bits",
                 "Encoder::convert_to_binary");
    }

    text_encoded_file.close();
    binary_file.close();

    LOG.info("Binary conversion completed. Total bits: " + std::to_string(total_bits) +
             ", written bytes: " + std::to_string((total_bits + 7) / 8),
             "Encoder::convert_to_binary");

    std::cout << "Binary file created: " << binary_output_path << std::endl;
    std::cout << "Size reduction: " << total_bits << " bits -> "
              << ((total_bits + 7) / 8) << " bytes" << std::endl;
}
*/


void Encoder::bit_encode(){
    std::ifstream input_file(input_path_);
    if(!input_file.is_open()){
        LOG.error("Error in opening input file " + input_path_, "Encoder::bit_encode");
        throw std::runtime_error("Error in opening file");
    }

    std::ofstream output_text(output_path_text_, std::ios::binary);
    if(!output_text.is_open()){
        LOG.error("Error in opening output file " + output_path_text_, "Encoder::bit_encode");
        throw std::runtime_error("Error in opening file");
    }

    std::ofstream output_alphabet(output_path_alphabet_);
    if(!output_alphabet.is_open()){
        LOG.error("Error in opening output file " + output_path_alphabet_, "Encoder::bit_encode");
        throw std::runtime_error("Error in opening file");
    }
    write_alphabet(output_alphabet);

    uint8_t padding = 0;
    output_text.put(static_cast<char>(padding));

    uint8_t current = 0;
    size_t filled = 0;

    char ch;
    size_t printed = 0;
    while(input_file.get(ch)){
        unsigned char u_ch = static_cast<unsigned char>(ch);
        if(u_ch >= dict_.size()){
            LOG.error("Error symbol is out of range: " + std::to_string(static_cast<char>(u_ch)), "Encoder::bit_encode");
            throw std::runtime_error("Symbol is out of range");
        }
        std::string & code = dict_[u_ch];
        if(code.empty()){
            LOG.error("Error no such symbol in dictionary: " + std::to_string(u_ch), "Encoder::bit_encode");
            throw std::runtime_error("No such symbol in dictionary");
        }
        if(printed < static_cast<size_t>(cout_number)){
            std::cout << code;
            ++printed;
        }

        for(char cb : code){
            uint8_t bit = (cb == '1') ? 1 : 0;
            current = static_cast<uint8_t>((current << 1) | bit);
            ++filled;

            if(filled == 8){
                output_text.put(static_cast<char>(current));
                current = 0;
                filled = 0;
            }
        }
    }

    if(filled != 0){
        uint8_t pad = static_cast<uint8_t>(8 - filled);
        current = static_cast<uint8_t>(current << pad);
        output_text.put(static_cast<char>(current));
        padding = pad;
    } else {
        padding = 0;
    }

    // Записать padding в начало файла
    output_text.seekp(0, std::ios::beg);
    output_text.put(static_cast<char>(padding));
}
