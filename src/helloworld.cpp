#include "helloworld.h"
#include <vector>
#include <string>
#include <sstream>

std::string get_hello_message() {
    std::vector<std::string> msg {"Hello", "C++", "World", "from", "VS Code", "and the C++ extension!"};
    std::stringstream result;
    for (const std::string& word : msg) {
        result << word << " ";
    }
    return result.str();
}
