#include <iostream>
using namespace std;

int main() {
    double num1, num2;
    char op;
    
    cout << "=== Простой калькулятор ===" << endl;
    cout << "Введите первое число: ";
    cin >> num1;
    cout << "Введите оператор (+, -, *, /): ";
    cin >> op;
    cout << "Введите второе число: ";
    cin >> num2;
    
    switch(op) {
        case '+':
            cout << "Результат: " << num1 + num2 << endl;
            break;
        case '-':
            cout << "Результат: " << num1 - num2 << endl;
            break;
        case '*':
            cout << "Результат: " << num1 * num2 << endl;
            break;
        case '/':
            if(num2 != 0) {
                cout << "Результат: " << num1 / num2 << endl;
            } else {
                cout << "Ошибка: деление на ноль!" << endl;
            }
            break;
        default:
            cout << "Неверный оператор!" << endl;
    }
    
    return 0;
}
