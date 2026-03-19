#include <iostream>
#include <chrono>
#include <limits>
#include <vector>

using namespace std;
using namespace chrono;

const int rnd_a = 16385;
const int rnd_c = 1;
const int rnd_m = 65536;

struct incorrect_size_error { 
    incorrect_size_error(string exceptionMessage) { cerr << "!!!ERROR!!! " << exceptionMessage << endl; } 
};

unsigned get_random_value() {
    static unsigned seed = 1;
    seed = (seed * rnd_a + rnd_c) % rnd_m;
    return seed;
}

int safe_input() {
    int digit;
    while (true) {
        int temp; 
        cin >> temp;
        if (cin.fail()) {
            cin.clear();
            cout << "A non-numeric value has been entered. Try to enter again" << endl;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        } else {
            digit = temp;
            break;
        }
    }
    return digit;
}

struct Timer {
    Timer() {
        start = high_resolution_clock::now();
        end = high_resolution_clock::now();
    }
    void start_time() { start = high_resolution_clock::now(); }
    void stop_time() { end = high_resolution_clock::now(); }
    void get_time() {
        auto duration = duration_cast<nanoseconds>(end - start);
        cout << "The algorithm execution time was " << duration.count() << " nanoseconds" << endl;
    }
private:
    time_point<high_resolution_clock> start;
    time_point<high_resolution_clock> end;
};

struct Matrix {
    int rows, cols;
    int* values;

    Matrix(int r = 0, int c = 0) : rows(r), cols(c) {
        if (rows > 0 && cols > 0) {
            values = new int[rows * cols]{0};
        } else {
            values = nullptr;
        }
    }

    ~Matrix() {
        delete[] values;
    }

    Matrix(const Matrix&) = delete;
    Matrix& operator=(const Matrix&) = delete;

    void enter() {
        for (int i = 0; i < rows * cols; i++)
            values[i] = safe_input();
    }

    void fill_random_values() {
        for (int i = 0; i < rows * cols; i++)
            values[i] = get_random_value();
    }

    void print() const {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++)
                cout << values[i * cols + j] << " ";
            cout << endl;
        }
    }

    int* operator[](int index) { return &values[index * cols]; }
    const int* operator[](int index) const { return &values[index * cols]; }
};

void classic_multiply_algorithm(Matrix& mat1, Matrix& mat2, bool print_result) {
    cout << "~~~ Classic multiplication algorythm~~~" << endl;
    Matrix mat3(mat1.rows, mat2.cols);
    Timer time;

    time.start_time();
    for (int i = 0; i < mat1.rows; i++)
        for (int j = 0; j < mat2.cols; j++)
            for (int k = 0; k < mat1.cols; k++)
                mat3[i][j] += (mat1[i][k] * mat2[k][j]);
    time.stop_time();
    time.get_time();
    
    if (print_result) mat3.print();
}

void winograd_algorithm(Matrix& mat1, Matrix& mat2, bool print_result) {
    cout << "~~~ Winograd algorythm ~~~" << endl;
    Matrix mat3(mat1.rows, mat2.cols);
    int remainder = mat1.cols % 2;
    
    vector<int> rowMult(mat1.rows, 0);
    vector<int> colMult(mat2.cols, 0);
    Timer time;

    time.start_time();
    for (int i = 0; i < mat1.rows; i++) 
        for (int k = 0; k < mat1.cols - 1; k+=2) 
            rowMult[i] += mat1[i][k] * mat1[i][k + 1];

    for (int i = 0; i < mat2.cols; i++) 
        for (int k = 0; k < mat2.rows - 1; k+=2) 
            colMult[i] += mat2[k][i] * mat2[k + 1][i];

    for (int i = 0; i < mat1.rows; i++) {
        for (int j = 0; j < mat2.cols; j++) {
            for (int k = 0; k < mat1.cols - 1; k+=2) 
                mat3[i][j] += ((mat1[i][k] + mat2[k + 1][j]) * (mat1[i][k + 1] + mat2[k][j]));
            mat3[i][j] = mat3[i][j] - (rowMult[i] + colMult[j]) + mat1[i][mat1.cols - 1] * mat2[mat1.cols - 1][j] * remainder; 
        } 
    }
    time.stop_time();
    time.get_time();
    
    if (print_result) mat3.print();
}

void winograd_optimized_algorithm(Matrix& mat1, Matrix& mat2, bool print_result) {
    cout << "~~~ Optimized Winograd algorythm ~~~" << endl;
    Matrix mat3(mat1.rows, mat2.cols);
    int remainder = mat1.cols % 2;
    
    vector<int> rowMult(mat1.rows, 0);
    vector<int> colMult(mat2.cols, 0);
    
    int N_decrement = mat1.cols - 1;
    int N_diveded = mat1.cols / 2;
    int M_diveded = mat2.rows / 2;
    Timer time;

    time.start_time();
    for (int i = 0; i < mat1.rows; i++) 
        for (int k = 0; k < N_diveded; k++) 
            rowMult[i] += mat1[i][k<<1] * mat1[i][(k<<1) + 1];

    for (int i = 0; i < mat2.cols; i++) 
        for (int k = 0; k < M_diveded; k++) 
            colMult[i] += mat2[k<<1][i] * mat2[(k<<1) + 1][i];

    for (int i = 0; i < mat1.rows; i++) {
        for (int j = 0; j < mat2.cols; j++) {
            for (int k = 0; k < N_diveded; k++) 
                mat3[i][j] += ((mat1[i][k<<1] + mat2[(k<<1) + 1][j]) * (mat1[i][(k<<1) + 1] + mat2[k<<1][j]));
            mat3[i][j] = mat3[i][j] - (rowMult[i] + colMult[j]) + mat1[i][N_decrement] * mat2[N_decrement][j] * remainder; 
        } 
    }
    time.stop_time();
    time.get_time();

    if (print_result) mat3.print();
}

void matricies_size_reader(int& m1_rows, int& m1_cols, int& m2_rows, int& m2_cols) {
    cout << "Enter The number of rows in matrix 1" << endl;
    m1_rows = safe_input();
    cout << "Enter The number of columns in matrix 1" << endl;
    m1_cols = safe_input();
    cout << "Enter The number of rows in matrix 2" << endl;
    m2_rows = safe_input();
    cout << "Enter The number of columns in matrix 2" << endl;
    m2_cols = safe_input();

    if ((m1_rows < 1) || (m1_cols < 1) || (m2_rows < 1) || (m2_cols < 1))
        throw incorrect_size_error("The size of the matrix must be at least 1");
    if (m1_cols != m2_rows) 
        throw incorrect_size_error("The number of columns in matrix 1 must be equal to the number of rows in matrix 2.");
}

void mode_test() {
    cout << "~~~ Testing mode for n-by-n matrices ~~~" << endl;
    for (int n = 10; n < 1300; n+=100) {
        Matrix mat(n, n);
        mat.fill_random_values();

        cout << "~~~~~ Results for n = " << n << " ~~~~~" << endl;
        classic_multiply_algorithm(mat, mat, false);
        winograd_algorithm(mat, mat, false);
        winograd_optimized_algorithm(mat, mat, false);
    }
}

void mode_user() {
    int m1_rows, m1_cols, m2_rows, m2_cols;
    try { matricies_size_reader(m1_rows, m1_cols, m2_rows, m2_cols); }
    catch (incorrect_size_error) { return; }

    Matrix mat1(m1_rows, m1_cols), mat2(m2_rows, m2_cols);
    cout << "Enter the elements of matrix 1:" << endl;
    mat1.enter();

    cout << "Enter the elements of matrix 2:" << endl;
    mat2.enter();

    classic_multiply_algorithm(mat1, mat2, true);
    winograd_algorithm(mat1, mat2, true);
    winograd_optimized_algorithm(mat1, mat2, true);
}

int main() {
    int menu;
    while (true) {
        cout << "Menu:" << endl << "Type 1 to use all matrix multipliers with your configuration"
             << endl << "Type 2 to test algorithms with random data" << endl;
            
        menu = safe_input();
        if (menu == 1) { mode_user(); break; }
        else if (menu == 2) { mode_test(); break; }
        else { cout << "Type 1 or 2" << endl; }
    }
    return 0;
}