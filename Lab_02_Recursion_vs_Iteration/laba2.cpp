#include <iostream>
#include <chrono>
#include <string>
#include <random>
#include <cstdint>
#include <limits>

struct Timer {
    Timer() {
        start = std::chrono::high_resolution_clock::now();
        end = std::chrono::high_resolution_clock::now();
    }

    void start_time() { start = std::chrono::high_resolution_clock::now(); }
    void stop_time() { end = std::chrono::high_resolution_clock::now(); }

    void get_time(const std::string& method_name) {
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        std::cout << "Время выполнения " << method_name << " алгоритма составило " << duration.count() << " наносекунд\n";
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
};

uint64_t get_random_value(int digits_count) {
    uint64_t min = 1;
    for (int i = 1; i < digits_count; ++i)
        min *= 10;
    uint64_t max = min * 10 - 1;

    static std::random_device rd;
    static std::mt19937_64 gen(rd());

    std::uniform_int_distribution<uint64_t> dis(min, max);
    return dis(gen);
}

bool isNumber(const std::string& s) {
    if (s.empty()) 
        return false;
    for (char c : s) {
        if (!isdigit(c)) 
            return false;
    }
    return true;
}

uint64_t safeInputUint64() {
    std::string input;
    while (true) {
        std::cin >> input;

        if (!isNumber(input)) {
            std::cout << "Это не число. Попробуйте снова: \n";
            continue;
        }

        try {
            size_t pos;
            uint64_t temp = std::stoull(input, &pos);
            if (pos != input.size()) throw std::invalid_argument("");
            return temp;
        } 
        catch (...) { 
            std::cout << "Число вне допустимого диапазона (слишком большое). Попробуйте снова: \n"; 
        }
    }
}

uint64_t sum_digits_recursion(uint64_t n) {
    if (n == 0) return 0;
    return (n % 10) + sum_digits_recursion(n / 10);
}

uint64_t sum_digits_non_recursion(uint64_t n) {
    uint64_t sum = 0;
    while (n != 0) {
        sum += n % 10;
        n /= 10;
    }
    return sum;
}

void mode_user() {
    std::cout << "Введите натуральное целое число:\n";
    uint64_t num = safeInputUint64();
    std::cout << "Сумма цифр числа с использованием рекурсии: " << sum_digits_recursion(num) << "\n";
    std::cout << "Сумма цифр числа без использования рекурсии: " << sum_digits_non_recursion(num) << "\n";
}

void mode_test() {
    uint64_t num, temp_res;
    for (int i = 1; i <= 10; i++) {
        std::cout << "Замер для " << i << "-значных чисел:\n"; 
        num = get_random_value(i);
        Timer time1, time2;
        
        time1.start_time();
        for (int k = 0; k < 10000; k++)
            temp_res = sum_digits_recursion(num);
        time1.stop_time();
        time1.get_time("рекурсивного  ");

        time2.start_time();
        for (int k = 0; k < 10000; k++)
            temp_res = sum_digits_non_recursion(num);
        time2.stop_time();
        time2.get_time("нерекурсивного");
    }
}

int main() {
    int menu;   
    std::cout << "Меню:\n" 
              << "Введите 1 для выбора пользовательского режима\n"
              << "Введите 2 для выбора режима массированного замера\n";
            
    std::string input;
    do { 
        std::cin >> input;
        if (input == "1") {
            menu = 1;
        } else if (input == "2") {
            menu = 2;
        } else {
            std::cout << "Введите 1 или 2\n";
            menu = 0;
        }
    } while (menu != 1 && menu != 2);

    if (menu == 1) 
        mode_user();
    else
        mode_test();
   
    return 0;
}
