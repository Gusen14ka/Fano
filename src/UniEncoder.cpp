#include "UniEncoder.hpp"
#include "Logger.hpp"
#include "Encoder.hpp"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <ios>
#include <string>
#include <iostream>

#define LOG Logger::getInstance()

void UniEncoder::make_alphabet(){
    fill_chars();
    unsigned symb_idx = 0;
    for(size_t i = 0; i < symbToCode_.size(); ++i){
        if(chars_[i] == 0){
            continue;
        }

        symbToCode_[i].resize(length_);
        encode_sigle_symbol(symb_idx, symbToCode_[i]);
        ++symb_idx;
    }
}

void UniEncoder::encode_sigle_symbol(unsigned index, std::string& buf){
    for(unsigned bit = 0; bit < length_; ++bit){
        unsigned shift = length_ - 1 - bit;
        buf[bit] = (index >> shift & 1u) ? '1' : '0';
    }
}

void UniEncoder::fill_chars(){
    std::ifstream inputfile(input_path_);
    if(!inputfile.is_open()){
        LOG.error("Error in opening file " + input_path_, "UniEncoder::make_alphabet");
        throw std::runtime_error("Error in opening file");
    }

    unsigned total = 0;
    char ch;
    while(inputfile.get(ch)){
        if(++chars_[static_cast<unsigned char>(ch)] == 1){
            ++total;
        } 
    }
    // bit_width(x) returns the floor(log2(x)) + 1
    // We encode total number of symbols starting with index = 0, so we use:
    length_ = std::max<unsigned>(1u, std::bit_width(total - 1));
    symb_num_ = total;
}

void UniEncoder::write_alphabet(std::ofstream& output_file){
    if(!output_file.is_open()){
        LOG.error("Error in opening file " + output_path_alphabet_, "UniEncoder::write_alphabet");
        throw std::runtime_error("Error in opening file");
    }
    output_file << symb_num_ << std::endl;
    LOG.info("Writing alphabet with " + std::to_string(symb_num_) + " symbols",
             "UniEncoder::write_alphabet");

    for(size_t i = 0; i < symbToCode_.size(); ++i){
        if(symbToCode_[i] != ""){
            output_file << Encoder::format_symbol(static_cast<unsigned char>(i)) << " " << symbToCode_[i] << std::endl;
            LOG.debug("Symbol: " + Encoder::format_symbol(static_cast<unsigned char>(i)) + " -> Code: " + symbToCode_[i],
                     "Encoder::write_alphabet");
        }
    }
}

void UniEncoder::start(){
    LOG.info("Starting encoder for file: " + input_path_, "UniEncoder::start");

    make_alphabet();
    // TODO: Add some checking making alphabet
    LOG.info("Starting text encoding", "UniEncoder::start");
    bit_encode();

    LOG.info("Encoding completed successfully", "UniEncoder::start");
}

void UniEncoder::bit_encode(){
    std::ofstream output_text(output_path_text_, std::ios::binary);
    if(!output_text.is_open()){
        LOG.error("Error in opening file " + output_path_text_, "UniEncoder::bit_encode");
        throw std::runtime_error("Error in opening file");
    }
    
    std::ofstream output_alphabet(output_path_alphabet_);
    if(!output_alphabet.is_open()){
        LOG.error("Error in opening file " + output_path_alphabet_, "UniEncoder::bit_encode");
        throw std::runtime_error("Error in opening file");
    }

    std::ifstream input_file(input_path_);
    if(!input_file.is_open()){
        LOG.error("Error in opening output file " + input_path_, "UniEncoder::bit_encode");
        throw std::runtime_error("Error in opening file");
    }

    write_alphabet(output_alphabet);

    // Reserve place for padding in header of file
    uint8_t padding = 0;
    output_text.put(static_cast<char>(padding));

    uint8_t current = 0;
    size_t filled = 0;
    char ch;
    size_t encoded_count = 0;

    while(input_file.get(ch)){
        unsigned char u_ch = static_cast<unsigned char>(ch);
        std::string& code = symbToCode_[ch];
         if(code.empty()){
            LOG.error("No code found for symbol: " + std::to_string(u_ch), "Encoder::bit_encode");
            throw std::runtime_error("Error in encoding");
        }
        if(encoded_count <= cout_number){
            std::cout << code;
        }

        for(auto& c : code){
            current = static_cast<uint8_t>((current << 1) | (c == '1' ? 1u : 0u));
            ++filled;
            if(filled == sizeof(current) * 8){
                output_text.put(static_cast<char>(current));
                current = 0;
                filled = 0;
            }
        }
        ++encoded_count;
    }

    if(filled != 0){
        padding = sizeof(current) * 8 - filled;
        current <<= padding;
        output_text.put(static_cast<char>(current));
        filled = 0;
        current = 0;
    }

    output_text.seekp(0, std::ios::beg);
    output_text.put(static_cast<char>(padding));
}