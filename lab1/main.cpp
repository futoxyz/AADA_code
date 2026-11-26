// В первой лабе в примере страница помирала из-за 500 тысяч примеров ввода. Очевидно, что так задумано не было, а эти же 500 тысяч тестов и проверяются в алгоритме. Они сохранены в tests.txt
// Более короткий вариант представлен в tests_short.txt. Отдельно здесь лежит run_tests.cpp для прогона тестов main.exe с бенчмарком и записью в файл.

#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

struct Node;

struct Link {
    std::shared_ptr<Node> child;
    std::weak_ptr<Node> back;

    std::shared_ptr<Node> target() const {
        if (child) {
            return child;
        }
        return back.lock();
    }
};

struct Node {
    std::string word;
    std::uint64_t value;
    int bit;
    Link next[2];

    Node(const std::string& key, std::uint64_t number, int bitIndex)
        : word(key), value(number), bit(bitIndex) {}
};

struct SearchPath {
    std::shared_ptr<Node> parent;
    std::shared_ptr<Node> branch;
    std::shared_ptr<Node> found;
};

void setLink(Node& node, int side, const std::shared_ptr<Node>& target) {
    Link& link = node.next[side];
    if (target->bit > node.bit) {
        link.child = target;
        link.back.reset();
    } else {
        link.back = target;
        link.child.reset();
    }
}

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

int getBit(const std::string& word, int bit) {
    if (bit == 0) {
        return 0;
    }
    int index = (bit - 1) / 8;
    if (index >= static_cast<int>(word.size())) {
        return 0;
    }
    int shift = 7 - (bit - 1) % 8;
    return (static_cast<unsigned char>(word[index]) >> shift) & 1;
}

int firstDifferentBit(const std::string& a, const std::string& b) {
    std::size_t i = 0;
    while (i < a.size() && i < b.size() && a[i] == b[i]) {
        ++i;
    }
    int bit = static_cast<int>(i) * 8 + 1;
    while (getBit(a, bit) == getBit(b, bit)) {
        ++bit;
    }
    return bit;
}

SearchPath searchPath(const std::shared_ptr<Node>& header, const std::string& word) {
    SearchPath path = {header, header, header->next[0].target()};
    while (path.found->bit > path.branch->bit) {
        path.parent = std::move(path.branch);
        path.branch = std::move(path.found);
        int side = getBit(word, path.branch->bit);
        path.found = path.branch->next[side].target();
    }
    return path;
}

bool findWord(const std::shared_ptr<Node>& header, const std::string& word, std::uint64_t& value) {
    if (header == nullptr) {
        return false;
    }
    std::shared_ptr<Node> found = searchPath(header, word).found;
    if (found->word != word) {
        return false;
    }
    value = found->value;
    return true;
}

bool addWord(std::shared_ptr<Node>& header, const std::string& word, std::uint64_t value) {
    if (header == nullptr) {
        header = std::make_shared<Node>(word, value, 0);
        setLink(*header, 0, header);
        return true;
    }
    std::shared_ptr<Node> found = searchPath(header, word).found;
    if (found->word == word) {
        return false;
    }
    int bit = firstDifferentBit(word, found->word);
    std::shared_ptr<Node> parent = header;
    int side = 0;
    std::shared_ptr<Node> child = parent->next[side].target();
    while (child->bit > parent->bit && child->bit < bit) {
        parent = child;
        side = getBit(word, parent->bit);
        child = parent->next[side].target();
    }
    std::shared_ptr<Node> added = std::make_shared<Node>(word, value, bit);
    int ownSide = getBit(word, bit);
    setLink(*added, ownSide, added);
    setLink(*added, 1 - ownSide, child);
    setLink(*parent, side, added);
    return true;
}

bool eraseWord(std::shared_ptr<Node>& header, const std::string& word) {
    if (header == nullptr) {
        return false;
    }
    SearchPath path = searchPath(header, word);
    const std::shared_ptr<Node>& x = path.found;
    const std::shared_ptr<Node>& q = path.branch;
    if (x->word != word) {
        return false;
    }
    if (!header->next[0].child) {
        header.reset();
        return true;
    }

    int toX = getBit(word, q->bit);
    int toQ = getBit(word, path.parent->bit);
    if (q != x) {
        std::shared_ptr<Node> source = searchPath(header, q->word).branch;
        int side = getBit(q->word, source->bit);
        setLink(*source, side, x);
        x->word.swap(q->word);
        x->value = q->value;
    }
    setLink(*path.parent, toQ, q->next[1 - toX].target());
    return true;
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

std::uint64_t saveEntries(const std::shared_ptr<Node>& node, std::ofstream& output) {
    if (node == nullptr) {
        return 0;
    }
    std::uint16_t length = static_cast<std::uint16_t>(node->word.size());
    output.write(reinterpret_cast<const char*>(&length), sizeof(length));
    output.write(node->word.data(), length);
    output.write(reinterpret_cast<const char*>(&node->value), sizeof(node->value));
    if (!output) {
        throw std::runtime_error("Error writing file");
    }

    std::uint64_t count = 1;
    for (int side = 0; side < 2; side++) {
        count += saveEntries(node->next[side].child, output);
    }
    return count;
}

void saveDictionary(const std::shared_ptr<Node>& header, const std::string& path) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Cannot open file for writing");
    }
    std::uint64_t count = 0;
    output.write(reinterpret_cast<const char*>(&count), sizeof(count));
    count = saveEntries(header, output);
    output.seekp(0);
    output.write(reinterpret_cast<const char*>(&count), sizeof(count));
    output.close();
    if (!output) {
        throw std::runtime_error("Error writing file");
    }
}

void loadDictionary(std::shared_ptr<Node>& root, const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Cannot open file for reading");
    }

    std::uint64_t count = 0;
    if (!input.read(reinterpret_cast<char*>(&count), sizeof(count))) {
        throw std::runtime_error("Invalid dictionary file");
    }

    std::shared_ptr<Node> loaded;
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
        if (!input || !normalizeWord(word) || !addWord(loaded, word, value)) {
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
    std::shared_ptr<Node> root;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        try {
            switch (line[0]) {
            case '+': {
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
                if (addWord(root, word, value)) {
                    std::cout << "OK\n";
                } else {
                    std::cout << "Exist\n";
                }
                break;
            }
            case '-': {
                std::string word = trim(line.substr(1));
                if (normalizeWord(word) && eraseWord(root, word)) {
                    std::cout << "OK\n";
                } else {
                    std::cout << "NoSuchWord\n";
                }
                break;
            }
            case '!': {
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
                break;
            }
            default: {
                std::uint64_t value;
                if (normalizeWord(line) && findWord(root, line, value)) {
                    std::cout << "OK: " << value << '\n';
                } else {
                    std::cout << "NoSuchWord\n";
                }
                break;
            }
            }
        } catch (const std::exception& error) {
            std::cout << "ERROR: " << error.what() << '\n';
        }
    }
}
