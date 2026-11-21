// В первой лабе в примере страница помирала из-за 500 тысяч тестов. Очевидно, что так задумано не было, а эти же 500 тысяч тестов и проверяются в алгоритме. Они сохранены в tests.txt
// Более короткий вариант представлен в tests_short.txt. Отдельно здесь лежит run_tests.cpp для прогона тестов main.exe с выводом результата в новый файл.

#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Node {
    std::string part;
    bool isWord = false;
    std::uint64_t value = 0;
    std::vector<Node> children;
};

bool parseNumber(const std::string& text, std::uint64_t& value) {
    if (text.empty()) {
        return false;
    }

    value = 0;
    for (char letter : text) {
        if (letter < '0' || letter > '9') {
            return false;
        }
        unsigned int digit = letter - '0';
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) {
            return false;
        }
        value = value * 10 + digit;
    }
    return true;
}

std::size_t commonPrefix(const std::string& part, const std::string& word, std::size_t position) {
    std::size_t length = 0;
    while (length < part.size() && position + length < word.size() &&
           part[length] == word[position + length]) {
        length++;
    }
    return length;
}

bool findWord(const Node& node, const std::string& word, std::size_t position, std::uint64_t& value) {
    if (position == word.size()) {
        value = node.value;
        return node.isWord;
    }

    for (const Node& child : node.children) {
        if (word[position] == child.part[0] &&
            word.compare(position, child.part.size(), child.part) == 0) {
            return findWord(child, word, position + child.part.size(), value);
        }
    }
    return false;
}

bool addWord(Node& node, const std::string& word, std::size_t position, std::uint64_t value) {
    if (position == word.size()) {
        if (node.isWord) {
            return false;
        }
        node.isWord = true;
        node.value = value;
        return true;
    }

    for (Node& child : node.children) {
        std::size_t common = commonPrefix(child.part, word, position);
        if (common == 0) {
            continue;
        }
        if (common == child.part.size()) {
            return addWord(child, word, position + common, value);
        }

        Node middle;
        middle.part = child.part.substr(0, common);

        Node oldBranch;
        oldBranch.part = child.part.substr(common);
        oldBranch.isWord = child.isWord;
        oldBranch.value = child.value;
        middle.children.push_back(std::move(oldBranch));

        addWord(middle, word, position + common, value);
        middle.children[0].children.swap(child.children);
        child = std::move(middle);
        return true;
    }

    Node leaf;
    leaf.part = word.substr(position);
    leaf.isWord = true;
    leaf.value = value;
    node.children.push_back(std::move(leaf));
    return true;
}

void mergeWithChild(Node& node, std::size_t index) {
    Node& child = node.children[index];
    Node merged;
    merged.part = node.part + child.part;
    merged.isWord = child.isWord;
    merged.value = child.value;
    merged.children.swap(child.children);
    node = std::move(merged);
}

bool eraseWord(Node& node, const std::string& word, std::size_t position) {
    for (std::size_t i = 0; i < node.children.size(); i++) {
        Node& child = node.children[i];
        if (word[position] != child.part[0] ||
            word.compare(position, child.part.size(), child.part) != 0) {
            continue;
        }

        std::size_t next = position + child.part.size();
        if (next < word.size()) {
            return eraseWord(child, word, next);
        }
        if (!child.isWord) {
            return false;
        }

        if (child.children.size() == 1) {
            mergeWithChild(child, 0);
        } else if (!child.children.empty()) {
            child.isWord = false;
        } else if (!node.part.empty() && !node.isWord && node.children.size() == 2) {
            mergeWithChild(node, 1 - i);
        } else {
            node.children.erase(node.children.begin() + i);
        }
        return true;
    }
    return false;
}

bool normalizeWord(std::string& word) {
    if (word.empty() || word.size() > 256) {
        return false;
    }

    for (char& letter : word) {
        if (letter >= 'A' && letter <= 'Z') {
            letter = static_cast<char>(letter - 'A' + 'a');
        } else if (letter < 'a' || letter > 'z') {
            return false;
        }
    }
    return true;
}

std::uint64_t countWords(const Node& node) {
    std::uint64_t count = 0;
    if (node.isWord) {
        count = 1;
    }
    for (const Node& child : node.children) {
        count += countWords(child);
    }
    return count;
}

void saveEntries(const Node& node, std::string& word, std::ofstream& output) {
    std::size_t oldLength = word.size();
    word += node.part;

    if (node.isWord) {
        std::uint16_t length = static_cast<std::uint16_t>(word.size());
        output.write(reinterpret_cast<const char*>(&length), sizeof(length));
        output.write(word.data(), length);
        output.write(reinterpret_cast<const char*>(&node.value), sizeof(node.value));
        if (!output) {
            throw std::runtime_error("Error writing file");
        }
    }

    for (const Node& child : node.children) {
        saveEntries(child, word, output);
    }
    word.resize(oldLength);
}

void saveDictionary(const Node& root, const std::string& path) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Cannot open file for writing");
    }

    std::uint64_t count = countWords(root);
    output.write(reinterpret_cast<const char*>(&count), sizeof(count));
    std::string word;
    saveEntries(root, word, output);
    output.close();

    if (!output) {
        throw std::runtime_error("Error writing file");
    }
}

void loadDictionary(Node& root, const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Cannot open file for reading");
    }

    std::uint64_t count = 0;
    if (!input.read(reinterpret_cast<char*>(&count), sizeof(count))) {
        throw std::runtime_error("Invalid dictionary file");
    }

    Node loaded;
    for (std::uint64_t i = 0; i < count; i++) {
        std::uint16_t length = 0;
        input.read(reinterpret_cast<char*>(&length), sizeof(length));
        if (!input || length == 0 || length > 256) {
            throw std::runtime_error("Invalid dictionary file");
        }

        std::string word(length, '\0');
        std::uint64_t value = 0;
        input.read(&word[0], length);
        input.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (!input || !normalizeWord(word) || !addWord(loaded, word, 0, value)) {
            throw std::runtime_error("Invalid dictionary file");
        }
    }

    char extra;
    if (input.get(extra) || !input.eof() || input.bad()) {
        throw std::runtime_error("Invalid dictionary file");
    }
    root = std::move(loaded);
}

std::string trim(const std::string& text) {
    std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

int main() {
    Node root;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        try {
            if (line[0] == '+') {
                std::istringstream input(line.substr(1));
                std::string word;
                std::string number;
                std::string extra;
                if (!(input >> word >> number) || (input >> extra) ||
                    !normalizeWord(word)) {
                    std::cout << "ERROR: Invalid add command\n";
                    continue;
                }

                std::uint64_t value;
                if (!parseNumber(number, value)) {
                    std::cout << "ERROR: Invalid number\n";
                    continue;
                }
                if (addWord(root, word, 0, value)) {
                    std::cout << "OK\n";
                } else {
                    std::cout << "Exist\n";
                }
            } else if (line[0] == '-') {
                std::string word = trim(line.substr(1));
                if (normalizeWord(word) && eraseWord(root, word, 0)) {
                    std::cout << "OK\n";
                } else {
                    std::cout << "NoSuchWord\n";
                }
            } else if (line[0] == '!') {
                std::istringstream input(line.substr(1));
                std::string command;
                std::string path;
                input >> command;
                std::getline(input, path);
                path = trim(path);
                if (path.empty()) {
                    throw std::runtime_error("Missing filename");
                }

                if (command == "Save") {
                    saveDictionary(root, path);
                } else if (command == "Load") {
                    loadDictionary(root, path);
                } else {
                    throw std::runtime_error("Unknown file command");
                }
                std::cout << "OK\n";
            } else {
                std::uint64_t value;
                if (normalizeWord(line) && findWord(root, line, 0, value)) {
                    std::cout << "OK: " << value << '\n';
                } else {
                    std::cout << "NoSuchWord\n";
                }
            }
        } catch (const std::exception& error) {
            std::cout << "ERROR: " << error.what() << '\n';
        }
    }
}
