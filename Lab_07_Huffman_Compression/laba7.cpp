#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <string>
#include <cstdint>
#include <cmath>
#include <climits>

using namespace std;

struct Node {
    int value;
    int freq;
    Node* left;
    Node* right;

    Node(int v, int f) : value(v), freq(f), left(nullptr), right(nullptr) {}
};

struct Compare {
    bool operator()(Node* a, Node* b) {
        if (a->freq != b->freq)
            return a->freq > b->freq;
        return getMinValueInSubtree(a) > getMinValueInSubtree(b);
    }
private:
    int getMinValueInSubtree(Node* node) {
        if (!node) return INT_MAX;
        if (!node->left && !node->right) return node->value;
        return min(getMinValueInSubtree(node->left), getMinValueInSubtree(node->right));
    }
};

class HuffmanCompressor {
private:
    unordered_map<int, string> huffmanCodes;
    unordered_map<int, int> frequencies;
    Node* root;

    void buildCodes(Node* node, const string& code) {
        if (!node) return;
        if (!node->left && !node->right) {
            huffmanCodes[node->value] = code;
            return;
        }
        buildCodes(node->left, code + "0");
        buildCodes(node->right, code + "1");
    }

    void deleteTree(Node* node) {
        if (!node) return;
        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }

public:
    HuffmanCompressor() : root(nullptr) {}
    ~HuffmanCompressor() { deleteTree(root); }

    string compress(const vector<int>& data) {
        if (data.empty()) return "";
        
        deleteTree(root);
        root = nullptr;
        frequencies.clear();

        for (int value : data) {
            frequencies[value]++;
        }

        priority_queue<Node*, vector<Node*>, Compare> pq;
        for (const auto& pair : frequencies) {
            pq.push(new Node(pair.first, pair.second));
        }

        while (pq.size() > 1) {
            Node* left = pq.top(); pq.pop();
            Node* right = pq.top(); pq.pop();

            int minValue = min(
                (left->left || left->right) ? INT_MAX : left->value,
                (right->left || right->right) ? INT_MAX : right->value
            );

            if (minValue == INT_MAX) minValue = -1;

            Node* parent = new Node(minValue, left->freq + right->freq);
            parent->left = left;
            parent->right = right;
            pq.push(parent);
        }

        if (!pq.empty()) {
            root = pq.top();
        } else {
            return "";
        }

        huffmanCodes.clear();
        buildCodes(root, "");

        string compressed;
        for (int value : data) {
            compressed += huffmanCodes[value];
        }

        return compressed;
    }

    vector<int> decompress(const string& compressedData) {
        vector<int> result;
        if (!root) {
            cerr << "Ошибка: дерево Хаффмана не построено!" << endl;
            return result;
        }

        Node* current = root;
        for (char bit : compressedData) {
            if (bit == '0') current = current->left;
            else if (bit == '1') current = current->right;

            if (!current->left && !current->right) {
                result.push_back(current->value);
                current = root;
            }
        }
        return result;
    }

    void saveCompressed(const string& filename, const string& compressedData) {
        ofstream file(filename, ios::binary);
        if (!file.is_open()) {
            cerr << "Ошибка создания файла: " << filename << endl;
            return;
        }

        int uniqueCount = frequencies.size();
        file.write(reinterpret_cast<const char*>(&uniqueCount), sizeof(uniqueCount));

        for (const auto& pair : frequencies) {
            file.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first));
            file.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
        }

        vector<unsigned char> bytes;
        string temp = compressedData;
        int padding = 8 - (temp.length() % 8);
        if (padding == 8) padding = 0;
        if (padding > 0) temp.append(padding, '0');

        for (size_t i = 0; i < temp.length(); i += 8) {
            string byteStr = temp.substr(i, 8);
            unsigned char byte = 0;
            for (int j = 0; j < 8; j++) {
                if (byteStr[j] == '1') {
                    byte |= (1 << (7 - j));
                }
            }
            bytes.push_back(byte);
        }

        int dataSize = bytes.size();
        file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        file.close();

        int totalFileSize = 4 + (uniqueCount * 8) + 4 + dataSize;
        cout << "Сохранено: уникальных символов = " << uniqueCount
             << ", данные = " << dataSize << " байт"
             << ", общий размер файла = " << totalFileSize << " байт" << endl;
    }

    int calculateTotalCompressedSize(const string& compressedData) const {
        int uniqueCount = frequencies.size();
        int dataSize = (compressedData.length() + 7) / 8;
        return 4 + (uniqueCount * 8) + 4 + dataSize;
    }

    int calculateOriginalSize(const vector<int>& data) const {
        return data.size() * sizeof(int);
    }

    double calculateCompressionRatio(const vector<int>& originalData, const string& compressedData) const {
        double originalSize = calculateOriginalSize(originalData);
        double compressedSize = calculateTotalCompressedSize(compressedData);
        return originalSize / compressedSize;
    }

    void printHuffmanCodes() const {
        cout << "\n+----------+----------+--------------+" << endl;
        cout << "|      ТАБЛИЦА КОДОВ ХАФФМАНА      |" << endl;
        cout << "+----------+----------+--------------+" << endl;
        cout << "|  Символ  | Частота  |     Код      |" << endl;
        cout << "+----------+----------+--------------+" << endl;

        vector<pair<int, int>> sortedFreq;
        for (const auto& pair : frequencies) {
            sortedFreq.push_back(pair);
        }
        sort(sortedFreq.begin(), sortedFreq.end(),
             [](const pair<int, int>& a, const pair<int, int>& b) {
                 return a.second > b.second;
             });

        for (const auto& pair : sortedFreq) {
            int symbol = pair.first;
            int freq = pair.second;
            string code = huffmanCodes.at(symbol);
            cout << "| " << setw(8) << right << symbol 
                 << " | " << setw(8) << freq 
                 << " | " << setw(12) << code << " |" << endl;
        }
        cout << "+----------+----------+--------------+" << endl;
    }
};

class NSynthParser {
public:
    static vector<int> parseInstrumentFamilies(const string& filename) {
        vector<int> families;
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Ошибка: Не удалось открыть файл " << filename << endl;
            return families;
        }

        string line;
        while (getline(file, line)) {
            size_t pos = line.find("\"instrument_family\"");
            if (pos != string::npos) {
                size_t colonPos = line.find(":", pos);
                if (colonPos != string::npos) {
                    string numberStr;
                    for (size_t i = colonPos + 1; i < line.length(); i++) {
                        if (isdigit(line[i])) {
                            numberStr += line[i];
                        } else if (!numberStr.empty()) {
                            break;
                        }
                    }
                    if (!numberStr.empty()) {
                        try {
                            families.push_back(stoi(numberStr));
                        } catch (...) {
                            cerr << "Ошибка преобразования: " << numberStr << endl;
                        }
                    }
                }
            }
        }
        file.close();
        return families;
    }
};

class CompressionResearch {
public:
    static vector<vector<int>> createTestFiles(const vector<int>& baseData, int count) {
        vector<vector<int>> testFiles;
        for (int i = 0; i < count; i++) {
            double multiplier = pow(1.5, i);
            size_t targetSize = static_cast<size_t>(baseData.size() * multiplier);
            vector<int> testData;
            while (testData.size() < targetSize) {
                size_t toCopy = min(baseData.size(), targetSize - testData.size());
                testData.insert(testData.end(), baseData.begin(), baseData.begin() + toCopy);
            }
            testFiles.push_back(testData);
        }
        return testFiles;
    }

    static void conductResearch(const vector<vector<int>>& testFiles) {
        cout << "\n=== ИССЛЕДОВАНИЕ СЖАТИЯ ===" << endl;
        cout << setw(16) << "Размер" << setw(20) << "Исходный(Б)" << setw(20) << "Сжатый(Б)"
             << setw(20) << "Коэффициент" << endl;
        cout << string(76, '-') << endl;

        HuffmanCompressor compressor;
        for (size_t i = 0; i < testFiles.size(); i++) {
            const auto& data = testFiles[i];
            double originalSize = data.size() * sizeof(int);
            string compressed = compressor.compress(data);
            double compressedSize = compressor.calculateTotalCompressedSize(compressed);
            double ratio = originalSize / compressedSize;

            cout << setw(16) << data.size()
                 << setw(20) << fixed << setprecision(1) << originalSize
                 << setw(20) << fixed << setprecision(1) << compressedSize
                 << setw(19) << fixed << setprecision(2) << ratio << "x" << endl;
        }
    }
};

int main() {
    cout << "=== СЖАТИЕ ДАННЫХ NSynth АЛГОРИТМОМ ХАФФМАНА ===" << endl;
    cout << "\n1. Чтение данных из examples.json" << endl;

    vector<int> instrumentFamilies = NSynthParser::parseInstrumentFamilies("nsynth-test/examples.json");

    if (instrumentFamilies.empty()) {
        cerr << "Не удалось прочитать данные! Возможно, отсутствует файл JSON." << endl;
        return 1;
    }

    cout << "Прочитано " << instrumentFamilies.size() << " значений instrument_family" << endl;

    HuffmanCompressor compressor;
    string compressed = compressor.compress(instrumentFamilies);

    double originalSize = compressor.calculateOriginalSize(instrumentFamilies);
    double totalCompressedSize = compressor.calculateTotalCompressedSize(compressed);
    double compressionRatio = compressor.calculateCompressionRatio(instrumentFamilies, compressed);

    cout << "Размер исходных данных: " << originalSize << " байт" << endl;
    cout << "Общий размер сжатого файла: " << totalCompressedSize << " байт" << endl;
    cout << "Коэффициент сжатия: " << fixed << setprecision(2) << compressionRatio << "x" << endl;

    compressor.printHuffmanCodes();
    compressor.saveCompressed("compressed_nsynth.bin", compressed);
    cout << "Сжатые данные сохранены в compressed_nsynth.bin" << endl;

    cout << "\n2. Проведение исследования" << endl;
    auto testFiles = CompressionResearch::createTestFiles(instrumentFamilies, 10);
    CompressionResearch::conductResearch(testFiles);

    return 0;
}
