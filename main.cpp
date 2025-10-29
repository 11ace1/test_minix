#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

class SimpleWindow {
private:
    Display* display;
    Window window;
    GC gc;
    int screen;
    
    // Элементы интерфейса
    Window inputField;
    Window label;
    Window button;
    
    // Текст
    std::string inputText;
    std::string labelText;
    std::string buttonText;
    
    // Флаги состояния
    bool inputActive;
    bool buttonPressed;

public:
    SimpleWindow() : inputText(""), labelText("Введите текст:"), buttonText("Применить"), 
                     inputActive(false), buttonPressed(false) {}
    
    bool initialize() {
        // Открываем соединение с X сервером
        display = XOpenDisplay(NULL);
        if (display == NULL) {
            fprintf(stderr, "Не удалось открыть дисплей X\n");
            return false;
        }
        
        screen = DefaultScreen(display);
        
        // Создаем главное окно
        window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                    100, 100, 400, 200, 1,
                                    BlackPixel(display, screen),
                                    WhitePixel(display, screen));
        
        // Устанавливаем заголовок окна
        XStoreName(display, window, "X11 Программа для MINIX");
        
        // Выбираем события для обработки
        XSelectInput(display, window, 
                    ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
        
        // Создаем графический контекст
        gc = XCreateGC(display, window, 0, NULL);
        XSetForeground(display, gc, BlackPixel(display, screen));
        
        // Создаем дочерние окна для элементов интерфейса
        createUIElements();
        
        // Показываем окно
        XMapWindow(display, window);
        
        return true;
    }
    
    void createUIElements() {
        // Создаем поле ввода
        inputField = XCreateSimpleWindow(display, window,
                                        150, 50, 200, 25, 1,
                                        BlackPixel(display, screen),
                                        WhitePixel(display, screen));
        XSelectInput(display, inputField, ExposureMask | ButtonPressMask);
        
        // Создаем лейбл
        label = XCreateSimpleWindow(display, window,
                                   50, 50, 90, 25, 0,
                                   WhitePixel(display, screen),
                                   WhitePixel(display, screen));
        XSelectInput(display, label, ExposureMask);
        
        // Создаем кнопку
        button = XCreateSimpleWindow(display, window,
                                   150, 100, 100, 30, 1,
                                   BlackPixel(display, screen),
                                   LightGrayPixel(display, screen));
        XSelectInput(display, button, ExposureMask | ButtonPressMask);
        
        // Показываем все элементы
        XMapWindow(display, inputField);
        XMapWindow(display, label);
        XMapWindow(display, button);
    }
    
    void drawLabel() {
        XClearWindow(display, label);
        XDrawString(display, label, gc, 5, 15, labelText.c_str(), labelText.length());
    }
    
    void drawInputField() {
        XClearWindow(display, inputField);
        
        // Рисуем рамку
        if (inputActive) {
            XSetForeground(display, gc, BlackPixel(display, screen));
            XDrawRectangle(display, inputField, gc, 0, 0, 199, 24);
        }
        
        // Рисуем текст
        XSetForeground(display, gc, BlackPixel(display, screen));
        if (!inputText.empty()) {
            XDrawString(display, inputField, gc, 5, 15, inputText.c_str(), inputText.length());
        }
        
        // Курсор, если поле активно
        if (inputActive) {
            int textWidth = XTextWidth(XLoadQueryFont(display, "fixed"), inputText.c_str(), inputText.length());
            XDrawLine(display, inputField, gc, 5 + textWidth, 5, 5 + textWidth, 20);
        }
    }
    
    void drawButton() {
        XClearWindow(display, button);
        
        // Рисуем 3D эффект для кнопки
        if (buttonPressed) {
            XSetForeground(display, gc, BlackPixel(display, screen));
            XDrawLine(display, button, gc, 0, 0, 99, 0);
            XDrawLine(display, button, gc, 0, 0, 0, 29);
            XSetForeground(display, gc, WhitePixel(display, screen));
            XDrawLine(display, button, gc, 99, 0, 99, 29);
            XDrawLine(display, button, gc, 0, 29, 99, 29);
        } else {
            XSetForeground(display, gc, WhitePixel(display, screen));
            XDrawLine(display, button, gc, 0, 0, 99, 0);
            XDrawLine(display, button, gc, 0, 0, 0, 29);
            XSetForeground(display, gc, BlackPixel(display, screen));
            XDrawLine(display, button, gc, 99, 0, 99, 29);
            XDrawLine(display, button, gc, 0, 29, 99, 29);
        }
        
        // Центрируем текст на кнопке
        XFontStruct* font = XLoadQueryFont(display, "fixed");
        if (font) {
            int textWidth = XTextWidth(font, buttonText.c_str(), buttonText.length());
            int x = (100 - textWidth) / 2;
            XSetFont(display, gc, font->fid);
            XDrawString(display, button, gc, x, 18, buttonText.c_str(), buttonText.length());
        }
    }
    
    void drawWindow() {
        XClearWindow(display, window);
        
        // Рисуем заголовок
        XDrawString(display, window, gc, 150, 30, "X11 Программа", 13);
        
        // Обновляем все элементы
        drawLabel();
        drawInputField();
        drawButton();
    }
    
    void handleButtonPress(XButtonEvent event) {
        // Проверяем, где был клик
        if (event.window == inputField) {
            inputActive = true;
            drawInputField();
        } 
        else if (event.window == button) {
            buttonPressed = true;
            drawButton();
            
            // Обработка нажатия кнопки
            if (!inputText.empty()) {
                labelText = "Вы ввели: " + inputText;
                drawLabel();
            }
        }
    }
    
    void handleButtonRelease(XButtonEvent event) {
        if (event.window == button) {
            buttonPressed = false;
            drawButton();
        }
    }
    
    void handleKeyPress(XKeyEvent event) {
        if (!inputActive) return;
        
        char buffer[10];
        KeySym keysym;
        int count = XLookupString(&event, buffer, sizeof(buffer) - 1, &keysym, NULL);
        
        if (count > 0) {
            buffer[count] = '\0';
            
            if (keysym == XK_BackSpace) {
                if (!inputText.empty()) {
                    inputText.pop_back();
                }
            }
            else if (keysym == XK_Return) {
                // Enter - применяем текст
                if (!inputText.empty()) {
                    labelText = "Вы ввели: " + inputText;
                    drawLabel();
                }
            }
            else if (buffer[0] >= 32 && buffer[0] <= 126) { // Печатные символы
                inputText += buffer[0];
            }
            
            drawInputField();
        }
    }
    
    void run() {
        XEvent event;
        bool running = true;
        
        while (running) {
            XNextEvent(display, &event);
            
            switch (event.type) {
                case Expose:
                    if (event.xexpose.window == window) {
                        drawWindow();
                    } else if (event.xexpose.window == label) {
                        drawLabel();
                    } else if (event.xexpose.window == inputField) {
                        drawInputField();
                    } else if (event.xexpose.window == button) {
                        drawButton();
                    }
                    break;
                    
                case ButtonPress:
                    handleButtonPress(event.xbutton);
                    break;
                    
                case ButtonRelease:
                    handleButtonRelease(event.xbutton);
                    break;
                    
                case KeyPress:
                    handleKeyPress(event.xkey);
                    break;
                    
                case ConfigureNotify:
                    // Обработка изменения размера окна
                    break;
                    
                case ClientMessage:
                    // Обработка закрытия окна
                    running = false;
                    break;
            }
        }
    }
    
    ~SimpleWindow() {
        if (display) {
            XFreeGC(display, gc);
            XDestroyWindow(display, inputField);
            XDestroyWindow(display, label);
            XDestroyWindow(display, button);
            XDestroyWindow(display, window);
            XCloseDisplay(display);
        }
    }
};

// Функция для получения светлосерого цвета (если не определена в системе)
unsigned long LightGrayPixel(Display* display, int screen) {
    XColor color;
    Colormap colormap = DefaultColormap(display, screen);
    
    // Пытаемся получить светлосерый цвет
    if (XAllocNamedColor(display, colormap, "light gray", &color, &color)) {
        return color.pixel;
    } else if (XAllocNamedColor(display, colormap, "gray", &color, &color)) {
        return color.pixel;
    } else {
        // Запасной вариант - создаем цвет вручную
        color.red = 60000;
        color.green = 60000;
        color.blue = 60000;
        color.flags = DoRed | DoGreen | DoBlue;
        XAllocColor(display, colormap, &color);
        return color.pixel;
    }
}

int main() {
    SimpleWindow app;
    
    if (!app.initialize()) {
        return 1;
    }
    
    app.run();
    
    return 0;
}
