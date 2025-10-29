#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXT_LENGTH 100

typedef struct {
    Display* display;
    Window window;
    GC gc;
    int screen;
    
    // Текст
    char inputText[MAX_TEXT_LENGTH];
    char labelText[MAX_TEXT_LENGTH];
    char buttonText[MAX_TEXT_LENGTH];
    
    // Флаги состояния
    int inputActive;
    int buttonPressed;
} SimpleWindow;

// Прототипы функций
unsigned long LightGrayPixel(Display* display, int screen);
void drawLabel(SimpleWindow* app);
void drawInputField(SimpleWindow* app);
void drawButton(SimpleWindow* app);
void drawWindow(SimpleWindow* app);
void handleButtonPress(SimpleWindow* app, XButtonEvent event);
void handleButtonRelease(SimpleWindow* app, XButtonEvent event);
void handleKeyPress(SimpleWindow* app, XKeyEvent event);

unsigned long LightGrayPixel(Display* display, int screen) {
    XColor color;
    Colormap colormap = DefaultColormap(display, screen);
    
    // Создаем светлосерый цвет
    color.red = 30000;
    color.green = 30000;
    color.blue = 30000;
    color.flags = DoRed | DoGreen | DoBlue;
    
    if (XAllocColor(display, colormap, &color)) {
        return color.pixel;
    }
    
    // Запасной вариант
    return WhitePixel(display, screen);
}

int initialize(SimpleWindow* app) {
    // Инициализация полей
    strcpy(app->inputText, "");
    strcpy(app->labelText, "text:");
    strcpy(app->buttonText, "Enter");
    app->inputActive = 0;
    app->buttonPressed = 0;
    
    // Открываем соединение с X сервером
    app->display = XOpenDisplay(NULL);
    if (app->display == NULL) {
        fprintf(stderr, "Cannot open X display\n");
        return 0;
    }
    
    app->screen = DefaultScreen(app->display);
    
    // Создаем главное окно
    app->window = XCreateSimpleWindow(app->display, RootWindow(app->display, app->screen),
                                    100, 100, 400, 200, 1,
                                    BlackPixel(app->display, app->screen),
                                    WhitePixel(app->display, app->screen));
    
    // Устанавливаем заголовок окна
    XStoreName(app->display, app->window, "X11 Program for MINIX");
    
    // Выбираем события для обработки
    XSelectInput(app->display, app->window, 
                ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
    
    // Создаем графический контекст
    app->gc = XCreateGC(app->display, app->window, 0, NULL);
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    
    // Показываем окно
    XMapWindow(app->display, app->window);
    
    return 1;
}

void drawLabel(SimpleWindow* app) {
    XClearWindow(app->display, app->window);
    XDrawString(app->display, app->window, app->gc, 50, 50, 
                app->labelText, strlen(app->labelText));
}

void drawInputField(SimpleWindow* app) {
    // Рисуем поле ввода
    int inputX = 150;
    int inputY = 45;
    int inputWidth = 200;
    int inputHeight = 25;
    
    // Рисуем рамку
    if (app->inputActive) {
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XDrawRectangle(app->display, app->window, app->gc, 
                      inputX, inputY, inputWidth - 1, inputHeight - 1);
    } else {
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XDrawRectangle(app->display, app->window, app->gc, 
                      inputX, inputY, inputWidth - 1, inputHeight - 1);
    }
    
    // Заливаем белым
    XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
    XFillRectangle(app->display, app->window, app->gc, 
                  inputX + 1, inputY + 1, inputWidth - 2, inputHeight - 2);
    
    // Рисуем текст
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    if (strlen(app->inputText) > 0) {
        XDrawString(app->display, app->window, app->gc, 
                   inputX + 5, inputY + 15, app->inputText, strlen(app->inputText));
    }
    
    // Курсор, если поле активно
    if (app->inputActive) {
        XFontStruct* font = XLoadQueryFont(app->display, "fixed");
        if (font) {
            int textWidth = XTextWidth(font, app->inputText, strlen(app->inputText));
            XDrawLine(app->display, app->window, app->gc, 
                     inputX + 5 + textWidth, inputY + 5, 
                     inputX + 5 + textWidth, inputY + 20);
        }
    }
}

void drawButton(SimpleWindow* app) {
    int buttonX = 150;
    int buttonY = 85;
    int buttonWidth = 100;
    int buttonHeight = 30;
    
    // Рисуем 3D эффект для кнопки
    if (app->buttonPressed) {
        // Вдавленная кнопка
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XDrawLine(app->display, app->window, app->gc, 
                 buttonX, buttonY, buttonX + buttonWidth - 1, buttonY);
        XDrawLine(app->display, app->window, app->gc, 
                 buttonX, buttonY, buttonX, buttonY + buttonHeight - 1);
        
        XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
        XDrawLine(app->display, app->window, app->gc, 
                 buttonX + buttonWidth - 1, buttonY, 
                 buttonX + buttonWidth - 1, buttonY + buttonHeight - 1);
        XDrawLine(app->display, app->window, app->gc, 
                 buttonX, buttonY + buttonHeight - 1, 
                 buttonX + buttonWidth - 1, buttonY + buttonHeight - 1);
    } else {
        // Выпуклая кнопка
        XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
        XDrawLine(app->display, app->window, app->gc, 
                 buttonX, buttonY, buttonX + buttonWidth - 1, buttonY);
        XDrawLine(app->display, app->window, app->gc, 
                 buttonX, buttonY, buttonX, buttonY + buttonHeight - 1);
        
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XDrawLine(app->display, app->window, app->gc, 
                 buttonX + buttonWidth - 1, buttonY, 
                 buttonX + buttonWidth - 1, buttonY + buttonHeight - 1);
        XDrawLine(app->display, app->window, app->gc, 
                 buttonX, buttonY + buttonHeight - 1, 
                 buttonX + buttonWidth - 1, buttonY + buttonHeight - 1);
    }
    
    // Заливаем серым
    XSetForeground(app->display, app->gc, LightGrayPixel(app->display, app->screen));
    XFillRectangle(app->display, app->window, app->gc, 
                  buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2);
    
    // Центрируем текст на кнопке
    XFontStruct* font = XLoadQueryFont(app->display, "fixed");
    if (font) {
        int textWidth = XTextWidth(font, app->buttonText, strlen(app->buttonText));
        int x = buttonX + (buttonWidth - textWidth) / 2;
        int y = buttonY + (buttonHeight + 8) / 2; // Эмпирическая формула для вертикального центрирования
        
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XSetFont(app->display, app->gc, font->fid);
        XDrawString(app->display, app->window, app->gc, x, y, 
                   app->buttonText, strlen(app->buttonText));
    }
}

void drawWindow(SimpleWindow* app) {
    XClearWindow(app->display, app->window);
    
    // Рисуем заголовок
    XDrawString(app->display, app->window, app->gc, 150, 30, "X11 Program", 11);
    
    // Обновляем все элементы
    drawLabel(app);
    drawInputField(app);
    drawButton(app);
}

void handleButtonPress(SimpleWindow* app, XButtonEvent event) {
    int x = event.x;
    int y = event.y;
    
    // Проверяем, где был клик
    int inputX = 150;
    int inputY = 45;
    int inputWidth = 200;
    int inputHeight = 25;
    
    int buttonX = 150;
    int buttonY = 85;
    int buttonWidth = 100;
    int buttonHeight = 30;
    
    // Клик в поле ввода
    if (x >= inputX && x <= inputX + inputWidth && 
        y >= inputY && y <= inputY + inputHeight) {
        app->inputActive = 1;
        drawInputField(app);
    } 
    // Клик в кнопке
    else if (x >= buttonX && x <= buttonX + buttonWidth && 
             y >= buttonY && y <= buttonY + buttonHeight) {
        app->buttonPressed = 1;
        drawButton(app);
        
        // Обработка нажатия кнопки
        if (strlen(app->inputText) > 0) {
            char newLabel[MAX_TEXT_LENGTH];
            snprintf(newLabel, sizeof(newLabel), "text: %s", app->inputText);
            strcpy(app->labelText, newLabel);
            drawLabel(app);
        }
    }
    // Клик вне элементов
    else {
        app->inputActive = 0;
        drawInputField(app);
    }
}

void handleButtonRelease(SimpleWindow* app, XButtonEvent event) {
    if (app->buttonPressed) {
        app->buttonPressed = 0;
        drawButton(app);
    }
}

void handleKeyPress(SimpleWindow* app, XKeyEvent event) {
    if (!app->inputActive) return;
    
    char buffer[10];
    KeySym keysym;
    int count = XLookupString(&event, buffer, sizeof(buffer) - 1, &keysym, NULL);
    
    if (count > 0) {
        buffer[count] = '\0';
        
        if (keysym == XK_BackSpace) {
            if (strlen(app->inputText) > 0) {
                app->inputText[strlen(app->inputText) - 1] = '\0';
            }
        }
        else if (keysym == XK_Return) {
            // Enter - применяем текст
            if (strlen(app->inputText) > 0) {
                char newLabel[MAX_TEXT_LENGTH];
                snprintf(newLabel, sizeof(newLabel), "text: %s", app->inputText);
                strcpy(app->labelText, newLabel);
                drawLabel(app);
            }
        }
        else if (buffer[0] >= 32 && buffer[0] <= 126) { // Печатные символы
            if (strlen(app->inputText) < MAX_TEXT_LENGTH - 1) {
                strncat(app->inputText, buffer, 1);
            }
        }
        
        drawInputField(app);
    }
}

void run(SimpleWindow* app) {
    XEvent event;
    int running = 1;
    
    while (running) {
        XNextEvent(app->display, &event);
        
        switch (event.type) {
            case Expose:
                drawWindow(app);
                break;
                
            case ButtonPress:
                handleButtonPress(app, event.xbutton);
                break;
                
            case ButtonRelease:
                handleButtonRelease(app, event.xbutton);
                break;
                
            case KeyPress:
                handleKeyPress(app, event.xkey);
                break;
                
            case ConfigureNotify:
                // Обработка изменения размера окна
                break;
        }
    }
}

void cleanup(SimpleWindow* app) {
    if (app->display) {
        XFreeGC(app->display, app->gc);
        XDestroyWindow(app->display, app->window);
        XCloseDisplay(app->display);
    }
}

int main() {
    SimpleWindow app;
    
    if (!initialize(&app)) {
        return 1;
    }
    
    run(&app);
    cleanup(&app);
    
    return 0;
}