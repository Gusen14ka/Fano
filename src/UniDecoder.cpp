#include "UniDecoder.hpp"
#include "Logger.hpp"
#include "Decoder.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>


#define LOG Logger::getInstance()

void UniDecoder::read_alphabet(std::ifstream& input_file){
    if(!input_file.is_open()){
        LOG.error("Error in opening file " + input_path_alphabet_, "UniDecoder::read_alphabet");
        throw std::runtime_error("Error in opening file");
    }

    size_t n = 0;
    input_file >> n;
    LOG.info("Reading alphabet with " + std::to_string(n) + " symbols", "UniDecoder::read_alphabet");

    for(size_t i = 0; i < n; ++i){
        std::string code, token;
        input_file >> token >> code;
        try {
            unsigned char symbol = Decoder::parse_symbol_token(token);
            if(length_ != 0 && length_ != code.size()){
                LOG.error("Codes have not same size", "UniDecoder::read_alphabet");
                throw std::runtime_error("Codes have not same size");
            }
            length_ = code.size();
            unsigned int idx = code_string_to_uint(code);
            codeToSymb_[idx] = symbol;
            LOG.debug("Symbol: " + token + " -> Code: " + code, "Decoder::read_alphabet");
        } catch (const std::exception& e) {
            LOG.error("Failed to parse symbol token: " + token + " - " + e.what(), "Decoder::read_alphabet");
            throw;
        }
    }

    if(length_ > 8){
        LOG.error("Incorrect legth of code " + std::to_string(length_), "UniDecoder::read_alphabet");
        throw std::runtime_error("Incorrect legth of code");
    }
}

unsigned int UniDecoder::code_string_to_uint(const std::string &s) {
    if (s.empty()) return 0u;
    const unsigned int maxBits = std::numeric_limits<unsigned int>::digits;
    if (s.size() > maxBits) {
        LOG.error("Code string too long", "UniDecoder::code_string_to_uint");
        throw std::overflow_error("code string too long");
    }

    unsigned int result = 0u;
    for (char ch : s) {
        if (ch != '0' && ch != '1') {
            LOG.error("Non-binary char in code string", "UniDecoder::code_string_to_uint");
            throw std::invalid_argument("non-binary char in code string");
        }
        result = (result << 1) | static_cast<unsigned int>(ch - '0');
    }
    return result;
}

void UniDecoder::bit_decode(std::ifstream& input_file){
    if(!input_file.is_open()){
        LOG.error("Error in opening file " + input_path_text_, "UniDecoder::bit_decode");
        throw std::runtime_error("Error in opening file");
    }

    std::ofstream output_file(output_path_);
    if(!output_file.is_open()){
        LOG.error("Error in opening file " + output_path_, "UniDecoder::bit_decode");
        throw std::runtime_error("Error in opening file");
    }

    uint8_t padding = 0;
    input_file.read(reinterpret_cast<char *>(&padding), sizeof(padding));
    if(input_file.gcount() != sizeof(padding)){
        LOG.error("Error in reading file " + input_path_text_, "UniDecoder::bit_decode");
        throw std::runtime_error("Error in reading file");
    }
    if (padding > 7) {
        LOG.error("Invalid padding value", "UniDecoder::bit_decode");
        throw std::runtime_error("Invalid padding value");
    }

    uint8_t current = 0;
    uint8_t k = 0;
    uint8_t temp = 0;            // хранит частичный код (старшие биты)
    size_t read_bit_in_code = 0; // сколько бит ещё нужно докопать до полного length_

    const unsigned BITS_PER_BYTE = 8;
    const unsigned CODE_MASK = (length_ == 8) ? 0xFFu : ((1u << length_) - 1u);

    std::string code;
    code.reserve(length_);

    while(input_file.read(reinterpret_cast<char *>(&current), sizeof(current))){
        bool is_last = (input_file.peek() == EOF);
        unsigned read_bit_in_byte = 0; // сколько бит уже прочитано в этом байте
        unsigned invalid_bits = 0;

        if(is_last && padding != 0){
            invalid_bits = static_cast<unsigned>(padding);
        }

        if(length_ < BITS_PER_BYTE){
            while(read_bit_in_byte < BITS_PER_BYTE - invalid_bits){
                if(read_bit_in_code != 0){
                    unsigned need = length_ - read_bit_in_code; // сколько ещё нужно
                    unsigned avail = BITS_PER_BYTE - invalid_bits - read_bit_in_byte; // сколько бит доступно сейчас
                    unsigned take = (avail < need) ? avail : need; // сколько возьмём из current

                    // Извлечём 'take' бит из текущего байта, начиная со старших непрочитанных бит
                    unsigned shift_right = BITS_PER_BYTE - (read_bit_in_byte + take);
                    // Сдвигаем и зануляем все остальные
                    uint8_t part = static_cast<uint8_t>((static_cast<unsigned>(current) >> shift_right) & ((1u << take) - 1u));

                    // Разместим part в temp (temp хранит старшие биты кода)
                    // Сдвинем part слево, если остались не считаные биты для кода
                    // То есть оставим место для оставшихся битов
                    temp = static_cast<uint8_t>(temp | static_cast<uint8_t>(part << (need - take)));

                    read_bit_in_byte += take;
                    read_bit_in_code += take;

                    if(read_bit_in_code != length_){
                        break;
                    }

                    // temp теперь содержит полный код
                    k = temp;
                    temp = 0;
                    read_bit_in_code = 0;

                    // Накладывем маску чтобы явно занулить идущие впереди биты
                    k = static_cast<uint8_t>(k & CODE_MASK);

                    if(codeToSymb_[k] == -1){
                        LOG.error("Code is not in dictionary", "UniDecoder::bit_decode");
                        throw std::runtime_error("Code is not in dictionary");
                    }
                    output_file.put(static_cast<unsigned char>(codeToSymb_[k]));
                }
                else{
                    // Нет временного остатка — пытаемся взять код целиком из текущего байта
                    unsigned avail = BITS_PER_BYTE - invalid_bits - read_bit_in_byte;
                    if(avail < length_){
                        // Не хватает бит в остатке байта -> сохраним их в temp
                        unsigned take = avail; // = avail
                        unsigned shift_right = BITS_PER_BYTE - (read_bit_in_byte + take);
                        uint8_t part = static_cast<uint8_t>((static_cast<unsigned>(current) >> shift_right) & ((1u << take) - 1u));

                        // Разместим эти take бит как старшие биты будущего кода
                        temp = static_cast<uint8_t>(part << (length_ - take));
                        read_bit_in_code += take;
                        read_bit_in_byte += take;

                        // перейдём к следующему байту для дополнения temp
                    }
                    else{
                        // В оставшемся байте хватает места для целого кода
                        unsigned shift_right = BITS_PER_BYTE - (read_bit_in_byte + length_);
                        uint8_t code_bits = static_cast<uint8_t>((static_cast<unsigned>(current) >> shift_right) & CODE_MASK);

                        if(codeToSymb_[code_bits] == -1){
                            LOG.error("Code is not in dictionary", "UniDecoder::bit_decode");
                            throw std::runtime_error("Code is not in dictionary");
                        }

                        output_file.put(static_cast<unsigned char>(codeToSymb_[code_bits]));
                        read_bit_in_byte += length_;
                        read_bit_in_code = 0;

                    }
                }
            }
        }
        else {
            // length_ == 8 (по предпосылке)
            if(codeToSymb_[current] == -1){
                LOG.error("Code is not in dictionary", "UniDecoder::decode");
                throw std::runtime_error("Code is not in dictionary");
            }
            output_file.put(static_cast<unsigned char>(codeToSymb_[current]));
        }
    }

    if (temp != 0 || read_bit_in_code != 0) {
        LOG.error("Remaining unmatched bits at the end of file", "UniDecoder::bit_decode");
        throw std::runtime_error("Corrupted file: unmatched trailing bits");
    }
}

void UniDecoder::start(){
    LOG.info("Starting unidecoder for file: " + input_path_text_, "UniDecoder::start");

    std::ifstream input_text(input_path_text_, std::ios::binary);
    if(!input_text.is_open()){
        LOG.error("Error in opening file " + input_path_text_, "UniDecoder::start");
        throw std::runtime_error("Error in opening file");
    }

    std::ifstream input_alphabet(input_path_alphabet_);
    if(!input_alphabet.is_open()){
        LOG.error("Error in opening file " + input_path_alphabet_, "UniDecoder::start");
        throw std::runtime_error("Error in opening file");
    }

    read_alphabet(input_alphabet);

    LOG.info("Starting text decoding", "UniDecoder::start");
    bit_decode(input_text);

    LOG.info("Decoding completed successfully", "UniDecoder::start");
}
