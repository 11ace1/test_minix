#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <cstdlib>
#include <string>

using namespace std;

int main() {
    cout << "Запуск X11 тестового приложения..." << endl;

    // Открываем соединение с X сервером
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        cerr << "Ошибка: не удалось подключиться к X серверу!" << endl;
        cerr << "Убедитесь, что X сервер запущен" << endl;
        return 1;
    }

    // Получаем экран по умолчанию
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    // Создаем окно
    Window window = XCreateSimpleWindow(
        display, root,
        100, 100, 400, 300,  // x, y, width, height
        2,                    // border width
        BlackPixel(display, screen),
        WhitePixel(display, screen)
    );

    // Выбираем события для обработки
    XSelectInput(display, window, 
        ExposureMask | KeyPressMask | ButtonPressMask);

    // Показываем окно
    XMapWindow(display, window);

    // Создаем графический контекст
    GC gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, BlackPixel(display, screen));

    // Основной цикл событий
    XEvent event;
    string message = "Hello, Minix X11!";
    bool running = true;

    cout << "close window" << endl;

    while (running) {
        XNextEvent(display, &event);

        switch (event.type) {
            case Expose:
                // Рисуем текст при перерисовке окна
                XDrawString(display, window, gc, 50, 50, 
                           message.c_str(), message.length());
                XDrawString(display, window, gc, 50, 80, 
                           "press any key", 41);
                XDrawString(display, window, gc, 50, 110, 
                           "exit", 10);
                break;

            case KeyPress:
            case ButtonPress:
                running = false;
                break;
        }
    }

    // Очистка ресурсов
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    cout << "finish" << endl;
    return 0;
}