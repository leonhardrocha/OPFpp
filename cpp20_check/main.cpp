#include <iostream>
#include <span>

int main() {
    int arr[] = {1, 2, 3};
    std::span<int> s(arr);
    std::cout << "Hello C++20!" << std::endl;
    return 0;
}
