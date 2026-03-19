#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <string>
#include <sstream>

struct CompressedMatrix {
    int rows;
    int cols;
    std::vector<double> an; 
    std::vector<int> nr;    
    std::vector<int> nc;    
    std::vector<int> jr;    
    std::vector<int> jc;    
};

void writeCompressedMatrix(std::ostream& output, const CompressedMatrix& cm) {
    output << "AN: "; for (double val : cm.an) output << val << " ";
    output << "\nJR: "; for (int idx : cm.jr) output << idx << " ";
    output << "\nJC: "; for (int idx : cm.jc) output << idx << " ";
    output << "\nNR: "; for (int idx : cm.nr) output << idx << " ";
    output << "\nNC: "; for (int idx : cm.nc) output << idx << " ";
    output << "\n";
}

void writeDecompressedMatrix(std::ostream& output, const std::vector<std::vector<double>>& matrix) {
    for (size_t i = 0; i < matrix.size(); i++) {
        for (size_t j = 0; j < matrix[i].size(); j++) {
            output << matrix[i][j] << " ";
        }
        output << "\n";
    }
}

CompressedMatrix compress(const std::vector<std::vector<double>>& matrix) {
    CompressedMatrix res;
    res.rows = static_cast<int>(matrix.size());
    res.cols = static_cast<int>(matrix[0].size());
    res.jr.assign(res.rows, -1);
    res.jc.assign(res.cols, -1);

    std::vector<int> last_row(res.rows, -1);
    std::vector<int> last_col(res.cols, -1);

    for (int i = 0; i < res.rows; ++i) {
        for (int j = 0; j < res.cols; ++j) {
            if (matrix[i][j] != 0.0) {
                int idx = static_cast<int>(res.an.size());
                res.an.push_back(matrix[i][j]);
                res.nr.push_back(-1);
                res.nc.push_back(-1);

                if (res.jr[i] == -1) res.jr[i] = idx;
                else res.nr[last_row[i]] = idx;
                last_row[i] = idx;

                if (res.jc[j] == -1) res.jc[j] = idx;
                else res.nc[last_col[j]] = idx;
                last_col[j] = idx;
            }
        }
    }

    for (int i = 0; i < res.rows; ++i) {
        if (res.jr[i] != -1) res.nr[last_row[i]] = res.jr[i];
    }
    for (int j = 0; j < res.cols; ++j) {
        if (res.jc[j] != -1) res.nc[last_col[j]] = res.jc[j];
    }

    return res;
}

std::vector<std::vector<double>> decompress(const CompressedMatrix& comp) {
    std::vector<std::vector<double>> res(comp.rows, std::vector<double>(comp.cols, 0.0));
    
    std::vector<int> row_of(comp.an.size(), -1);
    std::vector<int> col_of(comp.an.size(), -1);

    for (int i = 0; i < comp.rows; ++i) {
        if (comp.jr[i] != -1) {
            int curr = comp.jr[i];
            do {
                row_of[curr] = i;
                curr = comp.nr[curr];
            } while (curr != comp.jr[i]);
        }
    }

    for (int j = 0; j < comp.cols; ++j) {
        if (comp.jc[j] != -1) {
            int curr = comp.jc[j];
            do {
                col_of[curr] = j;
                curr = comp.nc[curr];
            } while (curr != comp.jc[j]);
        }
    }

    for (size_t k = 0; k < comp.an.size(); ++k) {
        if (row_of[k] != -1 && col_of[k] != -1) {
            res[row_of[k]][col_of[k]] = comp.an[k];
        }
    }
    return res;
}

CompressedMatrix transpose(const CompressedMatrix& A) {
    CompressedMatrix T;
    T.rows = A.cols;
    T.cols = A.rows;
    T.an = A.an; 
    T.jr = A.jc; 
    T.jc = A.jr; 
    T.nr = A.nc; 
    T.nc = A.nr; 
    return T;
}

double arithmetic_mean(const CompressedMatrix& comp) {
    double sum = 0.0;
    for (double num : comp.an) sum += num;
    return sum / (comp.cols * comp.rows);
}

std::vector<std::vector<double>> readMatrixFromFile(const std::string& filename) {
    std::ifstream matrix_input(filename);
    if (!matrix_input) throw std::runtime_error("Ошибка: не удалось открыть файл " + filename);

    int rows, cols;
    if (!(matrix_input >> rows >> cols)) throw std::runtime_error("Ошибка: неверный формат размеров матрицы");
    if (rows <= 0 || cols <= 0) throw std::runtime_error("Ошибка: размеры матрицы должны быть положительными");

    std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols));
    std::string line;
    std::getline(matrix_input, line); 

    for (int i = 0; i < rows; i++) {
        if (!std::getline(matrix_input, line)) throw std::runtime_error("Ошибка: недостаточно строк в матрице");
        std::stringstream ss(line);
        for (int j = 0; j < cols; j++) {
            std::string token;
            if (!(ss >> token)) throw std::runtime_error("Ошибка: недостаточно элементов в строке " + std::to_string(i + 1));
            try {
                size_t pos;
                matrix[i][j] = std::stod(token, &pos);
                if (pos != token.length()) throw std::invalid_argument("");
            } catch (...) {
                throw std::runtime_error("Ошибка парсинга значения: [" + std::to_string(i + 1) + "][" + std::to_string(j + 1) + "]");
            }
        }
    }
    return matrix;
}

int main() {
    try {
        std::vector<std::vector<double>> matrix = readMatrixFromFile("original_matrices.txt");
        
        std::ofstream output("result.txt");
        if (!output.is_open()) {
            std::cerr << "Не удалось открыть файл result.txt для записи\n";
            return 1;
        }

        output << "Исходная матрица " << matrix.size() << "x" << matrix[0].size() << ": \n";
        writeDecompressedMatrix(output, matrix);
        output << "\n";

        CompressedMatrix comp = compress(matrix);
        std::vector<std::vector<double>> decomp = decompress(comp);
        double mean = arithmetic_mean(comp);

        output << "Сжатая матрица: \n";
        writeCompressedMatrix(output, comp);
        output << "\nРаспакованная матрица: \n";
        writeDecompressedMatrix(output, decomp);
        output << "\nСреднее арифметическое матрицы: " << mean << "\n\n";

        CompressedMatrix comp_transposed = transpose(comp);
        std::vector<std::vector<double>> decomp_transposed = decompress(comp_transposed);
        
        output << "Сжатая транспонированная матрица: \n";
        writeCompressedMatrix(output, comp_transposed);
        output << "\nРаспакованная транспонированная матрица: \n";
        writeDecompressedMatrix(output, decomp_transposed);

        std::cout << "Работа программы завершена. Результаты записаны в result.txt\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
