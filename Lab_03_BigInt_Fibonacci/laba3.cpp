#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

class BigIntFibonacci {
public:
    std::vector<int> digits;

    explicit BigIntFibonacci(const std::string& s) {
        if (s.empty()) {
            digits = {0};
            return;
        }
        for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
            digits.push_back(s[i] - '0');
        }
        normalize();
    }

    explicit BigIntFibonacci(const std::vector<int>& v) : digits(v) {
        normalize();
    }

    BigIntFibonacci() {
        digits = {0};
    }

    BigIntFibonacci(const BigIntFibonacci& other) = default;
    BigIntFibonacci& operator=(const BigIntFibonacci& other) = default;

    bool operator==(const BigIntFibonacci& other) const {
        return digits == other.digits;
    }

    bool operator!=(const BigIntFibonacci& other) const {
        return !(*this == other);
    }

    bool operator<(const BigIntFibonacci& other) const {
        size_t n = std::max(digits.size(), other.digits.size());
        for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
            int a_digit = (i < digits.size()) ? digits[i] : 0;
            int b_digit = (i < other.digits.size()) ? other.digits[i] : 0;
            
            if (a_digit < b_digit) return true;
            if (a_digit > b_digit) return false;
        }
        return false;
    }

    bool operator<=(const BigIntFibonacci& other) const {
        return (*this < other) || (*this == other);
    }

    bool operator>(const BigIntFibonacci& other) const {
        return !(*this <= other);
    }

    bool operator>=(const BigIntFibonacci& other) const {
        return !(*this < other);
    }

    void normalize() {
        bool changed;
        do {
            changed = false;
            for (size_t i = 0; i < digits.size(); ++i) {
                if (digits[i] > 1) {
                    int carry = digits[i] / 2;
                    digits[i] %= 2;
                    if (i + 1 >= digits.size()) digits.push_back(0);
                    digits[i + 1] += carry;
                    changed = true;
                }
            }
            for (size_t i = 0; i + 1 < digits.size(); ++i) {
                if (digits[i] == 1 && digits[i + 1] == 1) {
                    digits[i] = 0;
                    digits[i + 1] = 0;
                    if (i + 2 >= digits.size()) digits.push_back(0);
                    digits[i + 2] += 1;
                    changed = true;
                }
            }
            while (digits.size() > 1 && digits.back() == 0) {
                digits.pop_back();
            }
        } while (changed);
    }

    BigIntFibonacci operator+(const BigIntFibonacci& other) const {
        std::vector<int> result;
        size_t maxSize = std::max(digits.size(), other.digits.size());
        result.resize(maxSize + 1, 0);

        for (size_t i = 0; i < maxSize; ++i) {
            int a_digit = (i < digits.size()) ? digits[i] : 0;
            int b_digit = (i < other.digits.size()) ? other.digits[i] : 0;
            result[i] = a_digit + b_digit;
        }
        BigIntFibonacci res(result);
        return res;
    }

    BigIntFibonacci operator-(const BigIntFibonacci& other) const {
        if (*this < other) return BigIntFibonacci("0");
        
        std::vector<int> result = digits;
        for (size_t i = 0; i < other.digits.size(); ++i) {
            result[i] -= other.digits[i];
        }
        
        for (size_t i = 0; i + 1 < result.size(); ++i) {
            if (result[i] < 0) {
                result[i] += 2;
                result[i + 1] -= 1;
            }
        }
        BigIntFibonacci res(result);
        return res;
    }

    BigIntFibonacci operator*(const BigIntFibonacci& other) const {
        std::vector<int> result(digits.size() + other.digits.size(), 0);
        for (size_t i = 0; i < digits.size(); ++i) {
            for (size_t j = 0; j < other.digits.size(); ++j) {
                result[i + j] += digits[i] * other.digits[j];
            }
        }
        BigIntFibonacci res(result);
        return res;
    }

    BigIntFibonacci operator%(int divisor) const {
        if (divisor <= 0) return BigIntFibonacci("0");
        
        BigIntFibonacci remainder("0");
        BigIntFibonacci divisor_fib(std::to_string(divisor));

        for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
            remainder.digits.insert(remainder.digits.begin(), 0);
            if (remainder.digits.empty()) remainder.digits = {0};
            remainder.digits[0] = digits[i];
            remainder.normalize();

            while (remainder >= divisor_fib) {
                remainder = remainder - divisor_fib;
            }
        }
        return remainder;
    }

    friend std::ostream& operator<<(std::ostream& output, const BigIntFibonacci& num) {
        for (int i = static_cast<int>(num.digits.size()) - 1; i >= 0; --i) {
            output << num.digits[i];
        }
        return output;
    }
};

bool isValidFibonacciString(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (c != '0' && c != '1') return false;
    }
    for (size_t i = 0; i + 1 < s.size(); ++i) {
        if (s[i] == '1' && s[i + 1] == '1') return false;
    }
    return true;
}

int main() {
    std::string a_str, b_str;
    while (true) {
        std::cout << "Введите число A в фибоначчиевой системе: ";
        std::cin >> a_str;
        std::cout << "Введите число B в фибоначчиевой системе: ";
        std::cin >> b_str;

        if (isValidFibonacciString(a_str) && isValidFibonacciString(b_str)) {
            break;
        }
        std::cout << "Некорректный ввод (допускаются только 0 и 1, без подряд идущих 1). Попробуйте снова.\n";
    }

    BigIntFibonacci a(a_str);
    BigIntFibonacci b(b_str);

    if (a < b) {
        std::cout << "Число A должно быть больше B для вычитания. Меняем местами...\n";
        std::swap(a, b);
    }

    std::cout << "A > B: " << (a > b ? "Да" : "Нет") << "\n";
    std::cout << "A == B: " << (a == b ? "Да" : "Нет") << "\n";
    std::cout << "A - B = " << (a - b) << "\n";
    std::cout << "A + B = " << (a + b) << "\n";
    std::cout << "A * B = " << (a * b) << "\n";
    std::cout << "A % 3 = " << (a % 3) << "\n";

    return 0;
}
