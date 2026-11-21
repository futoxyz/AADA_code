#include <cstdlib>
#include <iostream>

int main() {
    int result = std::system(
        "cmd /c \"main.exe < tests.txt > tests_result.txt\"");

    if (result != 0) {
        std::cerr << "ERROR: main.exe failed\n";
        return 1;
    }

    return 0;
}
