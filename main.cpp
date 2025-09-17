#include <iostream>
#include <string>
#include <cctype>
#include "Decoder.hpp"
#include "Encoder.hpp"
#include "Logger.hpp"

std::string getProjectRoot() {
    return std::filesystem::current_path().parent_path().string();
}

int main() {
    std::string projectRoot = getProjectRoot();
    Logger& logger = Logger::getInstance();
    logger.setLogFile("application.log");
    logger.setLogToFile(false);
    logger.setLogLevel(Logger::Level::INFO);

    logger.info("Application started", "main");

    while(true) {
        std::cout << "What you want? Enter D for decode or E for encode: ";
        char mode;
        std::cin >> mode;
        std::cout << "Enter path to input file:\n";
        std::string input, output;
        std::cin >> input;
        std::cout << "Enter path to output file:\n";
        std::cin >> output;

        std::string fullInputPath = projectRoot + "/" + input;
        std::string fullOutputPath = projectRoot + "/" + output;

        mode = std::toupper(mode);

        logger.info("User input: mode=" + std::string(1, mode) +
                   ", input=" + input + ", output=" + output, "main");

        switch (mode) {
            case 'D': {
                try {
                    Decoder decoder(fullInputPath, fullOutputPath);
                    decoder.start();
                    std::cout << "\nDecoding is finished. Check results: " << output << std::endl;
                    logger.info("Decoding completed successfully", "main");
                } catch (const std::exception& e) {
                    logger.error("Decoding failed: " + std::string(e.what()), "main");
                }
                break;
            }
            case 'E': {
                try {
                    Encoder encoder(fullInputPath, fullOutputPath);
                    encoder.start();
                    std::cout << "\nEncoding is finished. Check results: " << output << std::endl;

                    std::cout << "Convert to binary format? (y/n): ";
                    char convert_choice;
                    std::cin >> convert_choice;
                    convert_choice = std::toupper(convert_choice);

                    if (convert_choice == 'Y') {
                        encoder.convert_to_binary();
                    }

                    logger.info("Encoding completed successfully", "main");
                } catch (const std::exception& e) {
                    logger.error("Encoding failed: " + std::string(e.what()), "main");
                }
                break;
            }
            default:
                std::cout << "Invalid mode. Please enter 'D' or 'E'." << std::endl;
                logger.warning("Invalid mode entered: " + std::string(1, mode), "main");
        }

        std::cout << "Do you want to continue? (y/n): ";
        char continue_choice;
        std::cin >> continue_choice;
        continue_choice = std::toupper(continue_choice);

        logger.info("Continue choice: " + std::string(1, continue_choice), "main");

        if (continue_choice != 'Y') {
            logger.info("Application terminating", "main");
            break;
        }
    }

    return 0;
}