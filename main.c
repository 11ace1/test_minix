#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 200
#define INPUT_WIDTH 200
#define INPUT_HEIGHT 25
#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 30

typedef struct {
    Display *display;
    Window window;
    GC gc;
    int screen;
    
    // Состояние приложения
    char input_text[256];
    char label_text[256];
    int input_active;
    int button_pressed;
    
    // Координаты элементов
    int input_x, input_y;
    int label_x, label_y;
    int button_x, button_y;
} AppState;

// Функция инициализации приложения
int initialize_app(AppState *app) {
    // Открываем соединение с X сервером
    app->display = XOpenDisplay(NULL);
    if (app->display == NULL) {
        fprintf(stderr, "Не удалось открыть дисплей X\n");
        return 0;
    }
    
    app->screen = DefaultScreen(app->display);
    
    // Создаем главное окно
    app->window = XCreateSimpleWindow(app->display, 
                                     RootWindow(app->display, app->screen),
                                     100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 2,
                                     BlackPixel(app->display, app->screen),
                                     WhitePixel(app->display, app->screen));
    
    // Устанавливаем заголовок окна
    XStoreName(app->display, app->window, "MINIX X11 Program");
    
    // Выбираем события для обработки
    XSelectInput(app->display, app->window, 
                ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask);
    
    // Создаем графический контекст
    XGCValues values;
    values.foreground = BlackPixel(app->display, app->screen);
    values.background = WhitePixel(app->display, app->screen);
    app->gc = XCreateGC(app->display, app->window, GCForeground | GCBackground, &values);
    
    // Инициализируем состояние
    strcpy(app->input_text, "");
    strcpy(app->label_text, "Введите текст:");
    app->input_active = 0;
    app->button_pressed = 0;
    
    // Устанавливаем координаты элементов
    app->label_x = 50;
    app->label_y = 50;
    app->input_x = 150;
    app->input_y = 50;
    app->button_x = 150;
    app->button_y = 100;
    
    // Показываем окно
    XMapWindow(app->display, app->window);
    XFlush(app->display);
    
    return 1;
}

// Функция отрисовки лейбла
void draw_label(AppState *app) {
    XDrawString(app->display, app->window, app->gc, 
                app->label_x, app->label_y + 15, 
                app->label_text, strlen(app->label_text));
}

// Функция отрисовки поля ввода
void draw_input_field(AppState *app) {
    // Рисуем рамку
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawRectangle(app->display, app->window, app->gc, 
                   app->input_x, app->input_y, INPUT_WIDTH, INPUT_HEIGHT);
    
    // Заливаем белым фон
    XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
    XFillRectangle(app->display, app->window, app->gc, 
                   app->input_x + 1, app->input_y + 1, INPUT_WIDTH - 2, INPUT_HEIGHT - 2);
    
    // Рисуем текст
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    if (strlen(app->input_text) > 0) {
        XDrawString(app->display, app->window, app->gc, 
                    app->input_x + 5, app->input_y + 15, 
                    app->input_text, strlen(app->input_text));
    }
    
    // Рисуем курсор если поле активно
    if (app->input_active) {
        int text_width = 8 * strlen(app->input_text); // Приблизительная ширина текста
        XDrawLine(app->display, app->window, app->gc, 
                  app->input_x + 5 + text_width, app->input_y + 5,
                  app->input_x + 5 + text_width, app->input_y + 20);
    }
}

// Функция отрисовки кнопки
void draw_button(AppState *app) {
    // Рисуем рамку кнопки
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawRectangle(app->display, app->window, app->gc, 
                   app->button_x, app->button_y, BUTTON_WIDTH, BUTTON_HEIGHT);
    
    // Заливаем фон кнопки
    if (app->button_pressed) {
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    } else {
        XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
    }
    XFillRectangle(app->display, app->window, app->gc, 
                   app->button_x + 1, app->button_y + 1, BUTTON_WIDTH - 2, BUTTON_HEIGHT - 2);
    
    // Рисуем текст кнопки
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    char *button_text = "Применить";
    int text_x = app->button_x + (BUTTON_WIDTH - 8 * strlen(button_text)) / 2;
    int text_y = app->button_y + BUTTON_HEIGHT / 2 + 4;
    XDrawString(app->display, app->window, app->gc, text_x, text_y, button_text, strlen(button_text));
}

// Функция отрисовки всего окна
void draw_window(AppState *app) {
    // Очищаем окно
    XClearWindow(app->display, app->window);
    
    // Рисуем заголовок
    XDrawString(app->display, app->window, app->gc, 150, 30, "MINIX X11 Program", 17);
    
    // Рисуем все элементы
    draw_label(app);
    draw_input_field(app);
    draw_button(app);
    
    XFlush(app->display);
}

// Проверка попадания точки в поле ввода
int is_in_input_field(AppState *app, int x, int y) {
    return (x >= app->input_x && x <= app->input_x + INPUT_WIDTH && 
            y >= app->input_y && y <= app->input_y + INPUT_HEIGHT);
}

// Проверка попадания точки в кнопку
int is_in_button(AppState *app, int x, int y) {
    return (x >= app->button_x && x <= app->button_x + BUTTON_WIDTH && 
            y >= app->button_y && y <= app->button_y + BUTTON_HEIGHT);
}

// Обработка нажатия кнопки мыши
void handle_button_press(AppState *app, int x, int y) {
    if (is_in_input_field(app, x, y)) {
        app->input_active = 1;
        draw_input_field(app);
    } 
    else if (is_in_button(app, x, y)) {
        app->button_pressed = 1;
        draw_button(app);
    }
    else {
        app->input_active = 0;
        draw_input_field(app);
    }
    XFlush(app->display);
}

// Обработка отпускания кнопки мыши
void handle_button_release(AppState *app, int x, int y) {
    if (app->button_pressed && is_in_button(app, x, y)) {
        // Обработка нажатия кнопки - обновляем лейбл
        if (strlen(app->input_text) > 0) {
            strcpy(app->label_text, "Вы ввели: ");
            strcat(app->label_text, app->input_text);
            draw_label(app);
        }
    }
    app->button_pressed = 0;
    draw_button(app);
    XFlush(app->display);
}

// Обработка нажатия клавиши
void handle_key_press(AppState *app, XKeyEvent event) {
    if (!app->input_active) return;
    
    char buffer[10];
    KeySym keysym;
    int count = XLookupString(&event, buffer, sizeof(buffer) - 1, &keysym, NULL);
    
    if (count > 0) {
        buffer[count] = '\0';
        
        if (keysym == XK_BackSpace) {
            // Удаляем последний символ
            int len = strlen(app->input_text);
            if (len > 0) {
                app->input_text[len - 1] = '\0';
            }
        }
        else if (keysym == XK_Return) {
            // Enter - применяем текст
            if (strlen(app->input_text) > 0) {
                strcpy(app->label_text, "Вы ввели: ");
                strcat(app->label_text, app->input_text);
                draw_label(app);
            }
        }
        else if (buffer[0] >= 32 && buffer[0] <= 126) {
            // Добавляем символ если есть место
            int len = strlen(app->input_text);
            if (len < sizeof(app->input_text) - 1) {
                app->input_text[len] = buffer[0];
                app->input_text[len + 1] = '\0';
            }
        }
        
        draw_input_field(app);
        XFlush(app->display);
    }
}

// Главный цикл приложения
void run_app(AppState *app) {
    XEvent event;
    int running = 1;
    
    while (running) {
        XNextEvent(app->display, &event);
        
        switch (event.type) {
            case Expose:
                draw_window(app);
                break;
                
            case ButtonPress:
                handle_button_press(app, event.xbutton.x, event.xbutton.y);
                break;
                
            case ButtonRelease:
                handle_button_release(app, event.xbutton.x, event.xbutton.y);
                break;
                
            case KeyPress:
                handle_key_press(app, event.xkey);
                break;
        }
    }
}

// Освобождение ресурсов
void cleanup_app(AppState *app) {
    if (app->display) {
        XFreeGC(app->display, app->gc);
        XDestroyWindow(app->display, app->window);
        XCloseDisplay(app->display);
    }
}

int main() {
    AppState app;
    
    printf("Запуск X11 приложения для MINIX...\n");
    
    if (!initialize_app(&app)) {
        fprintf(stderr, "Ошибка инициализации приложения\n");
        return 1;
    }
    
    printf("Приложение успешно инициализировано\n");
    printf("Используйте:\n");
    printf(" - Кликните на поле ввода для активации\n");
    printf(" - Введите текст с клавиатуры\n");
    printf(" - Нажмите кнопку 'Применить' или Enter\n");
    
    run_app(&app);
    cleanup_app(&app);
    
    return 0;
}
