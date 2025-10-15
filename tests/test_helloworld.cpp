#include "helloworld.h"
#include <cassert>
#include <string>

int main() {
    std::string expected_msg = "Hello C++ World from VS Code and the C++ extension! ";
    assert(get_hello_message() == expected_msg);
    return 0;
}
