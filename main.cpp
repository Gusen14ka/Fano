#include "Encoder.hpp"
#include "Decoder.hpp"
#include <iostream>

int main(){
    while(true){
        std::cout << "What you want? Enter D for decode or E for encode: ";
        std::string mode;
        std::cin >> mode;
        std::cout << "Enter path to input file:\n";
        std::string input, output;
        std::cin >> input;
        std::cout << "Enter path to output file:\n";
        std::cin >> output;
        if(mode == "D"){
            Decoder decoder(input, output);
            decoder.start();
            std::cout << "\nDecoding is finished. Check results: " << output << std::endl;
        }
        if(mode == "E"){
            Encoder encoder(input, output);
            encoder.start();
            std::cout << "\nEncoding is finished. Check results: " << output << std::endl;
        }
        std::cout << "Enter 1 to restart or 0 to finish the programm: ";
        std::string next;
        std::cin >> next;
        if(next == "0"){
            break;
        }
    }
}