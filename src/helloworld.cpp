#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <span>
#include <array>

std::string get_hello_message() {
    std::array<std::string, 6> msg_array {"Hello", "C++", "World", "from", "VS Code", "and the C++ extension!"};
    std::span<std::string> msg(msg_array);
    
    std::stringstream result;
    for (const std::string& word : msg) {
        result << word << " ";
    }
    return result.str();
}


int main() {
    std::string message = get_hello_message();
    std::cout << message << std::endl;
    return 0;
}