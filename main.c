#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXT_LENGTH 50
#define MAX_LABEL_LENGTH 100
#define COMBO_WIDTH 200
#define COMBO_HEIGHT 25
#define ITEM_HEIGHT 20
#define MAX_ITEMS 4

typedef struct {
    Display* display;
    Window window;
    GC gc;
    int screen;
    
    // Текст
    char selectedText[MAX_TEXT_LENGTH];
    char labelText[MAX_LABEL_LENGTH];
    char buttonText[20];
    
    // Выпадающий список
    Window comboBox;
    Window dropDown;
    int isOpen;
    int selectedIndex;
    char items[MAX_ITEMS][50];
    int itemCount;
    
    // Флаги состояния
    int buttonPressed;
} SimpleWindow;

// Прототипы функций
unsigned long LightGrayPixel(Display* display, int screen);
void drawLabel(SimpleWindow* app);
void drawComboBox(SimpleWindow* app);
void drawDropDown(SimpleWindow* app);
void drawButton(SimpleWindow* app);
void drawWindow(SimpleWindow* app);
void handleButtonPress(SimpleWindow* app, XButtonEvent event);
void handleButtonRelease(SimpleWindow* app, XButtonEvent event);
void openDropDown(SimpleWindow* app);
void closeDropDown(SimpleWindow* app);
void selectItem(SimpleWindow* app, int index);

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
    strncpy(app->selectedText, "Выберите город", MAX_TEXT_LENGTH - 1);
    app->selectedText[MAX_TEXT_LENGTH - 1] = '\0';
    strncpy(app->labelText, "Город:", MAX_LABEL_LENGTH - 1);
    app->labelText[MAX_LABEL_LENGTH - 1] = '\0';
    strncpy(app->buttonText, "Выбрать", sizeof(app->buttonText) - 1);
    app->buttonText[sizeof(app->buttonText) - 1] = '\0';
    
    // Инициализация выпадающего списка
    app->isOpen = 0;
    app->selectedIndex = -1;
    app->itemCount = 4;
    strcpy(app->items[0], "Москва");
    strcpy(app->items[1], "Рязань");
    strcpy(app->items[2], "Санкт-Петербург");
    strcpy(app->items[3], "Владивосток");
    
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
                                    100, 100, 400, 300, 1,
                                    BlackPixel(app->display, app->screen),
                                    WhitePixel(app->display, app->screen));
    
    // Создаем комбо-бокс
    app->comboBox = XCreateSimpleWindow(app->display, app->window,
                                      150, 45, COMBO_WIDTH, COMBO_HEIGHT, 1,
                                      BlackPixel(app->display, app->screen),
                                      WhitePixel(app->display, app->screen));
    
    // Создаем выпадающий список
    app->dropDown = XCreateSimpleWindow(app->display, app->window,
                                      150, 70, COMBO_WIDTH, ITEM_HEIGHT * MAX_ITEMS, 1,
                                      BlackPixel(app->display, app->screen),
                                      WhitePixel(app->display, app->screen));
    
    // Устанавливаем заголовок окна
    XStoreName(app->display, app->window, "X11 ComboBox Program");
    
    // Выбираем события для обработки
    XSelectInput(app->display, app->window, 
                ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
    XSelectInput(app->display, app->comboBox, ExposureMask | ButtonPressMask);
    XSelectInput(app->display, app->dropDown, ExposureMask | ButtonPressMask);
    
    // Создаем графический контекст
    app->gc = XCreateGC(app->display, app->window, 0, NULL);
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    
    // Показываем окно и комбо-бокс
    XMapWindow(app->display, app->window);
    XMapWindow(app->display, app->comboBox);
    XUnmapWindow(app->display, app->dropDown);
    
    return 1;
}

void drawLabel(SimpleWindow* app) {
    // Очищаем область лейбла (примерные координаты)
    XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
    XFillRectangle(app->display, app->window, app->gc, 45, 35, 300, 20);
    
    // Рисуем текст лейбла
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawString(app->display, app->window, app->gc, 50, 50, 
                app->labelText, strlen(app->labelText));
}

void drawComboBox(SimpleWindow* app) {
    XClearWindow(app->display, app->comboBox);
    
    // Рисуем рамку
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawRectangle(app->display, app->comboBox, app->gc, 0, 0, COMBO_WIDTH-1, COMBO_HEIGHT-1);
    
    // Заливаем белым
    XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
    XFillRectangle(app->display, app->comboBox, app->gc, 1, 1, COMBO_WIDTH-2, COMBO_HEIGHT-2);
    
    // Рисуем текст
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawString(app->display, app->comboBox, app->gc, 5, 15, 
                app->selectedText, strlen(app->selectedText));
    
    // Рисуем стрелку
    int arrowX = COMBO_WIDTH - 15;
    int arrowY = COMBO_HEIGHT / 2;
    
    if (app->isOpen) {
        // Стрелка вверх
        XDrawLine(app->display, app->comboBox, app->gc, arrowX, arrowY + 3, arrowX + 5, arrowY - 3);
        XDrawLine(app->display, app->comboBox, app->gc, arrowX + 5, arrowY - 3, arrowX + 10, arrowY + 3);
    } else {
        // Стрелка вниз
        XDrawLine(app->display, app->comboBox, app->gc, arrowX, arrowY - 3, arrowX + 5, arrowY + 3);
        XDrawLine(app->display, app->comboBox, app->gc, arrowX + 5, arrowY + 3, arrowX + 10, arrowY - 3);
    }
}

void drawDropDown(SimpleWindow* app) {
    if (!app->isOpen) return;
    
    XClearWindow(app->display, app->dropDown);
    
    for (int i = 0; i < app->itemCount; i++) {
        int y = i * ITEM_HEIGHT;
        
        // Подсветка выбранного элемента
        if (i == app->selectedIndex) {
            XSetForeground(app->display, app->gc, LightGrayPixel(app->display, app->screen));
            XFillRectangle(app->display, app->dropDown, app->gc, 1, y + 1, COMBO_WIDTH - 2, ITEM_HEIGHT - 2);
        } else {
            XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
            XFillRectangle(app->display, app->dropDown, app->gc, 1, y + 1, COMBO_WIDTH - 2, ITEM_HEIGHT - 2);
        }
        
        // Рисуем текст элемента
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XDrawString(app->display, app->dropDown, app->gc, 5, y + 15, 
                    app->items[i], strlen(app->items[i]));
    }
    
    // Рисуем рамку вокруг всего списка
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawRectangle(app->display, app->dropDown, app->gc, 0, 0, COMBO_WIDTH-1, ITEM_HEIGHT * app->itemCount);
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
        int y = buttonY + (buttonHeight + 8) / 2;
        
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XSetFont(app->display, app->gc, font->fid);
        XDrawString(app->display, app->window, app->gc, x, y, 
                   app->buttonText, strlen(app->buttonText));
        XFreeFont(app->display, font);
    }
}

void drawWindow(SimpleWindow* app) {
    XClearWindow(app->display, app->window);
    
    // Рисуем заголовок
    XDrawString(app->display, app->window, app->gc, 150, 30, "Выбор города", 12);
    
    // Обновляем все элементы
    drawLabel(app);
    drawComboBox(app);
    drawDropDown(app);
    drawButton(app);
}

void openDropDown(SimpleWindow* app) {
    app->isOpen = 1;
    XMapWindow(app->display, app->dropDown);
    XRaiseWindow(app->display, app->dropDown);
    drawComboBox(app);
    drawDropDown(app);
}

void closeDropDown(SimpleWindow* app) {
    app->isOpen = 0;
    XUnmapWindow(app->display, app->dropDown);
    drawComboBox(app);
}

void selectItem(SimpleWindow* app, int index) {
    if (index >= 0 && index < app->itemCount) {
        app->selectedIndex = index;
        strcpy(app->selectedText, app->items[index]);
        drawComboBox(app);
    }
}

int getDropDownItemAtPos(SimpleWindow* app, int y) {
    int relativeY = y - 70; // Y координата начала dropDown
    if (relativeY >= 0 && relativeY < ITEM_HEIGHT * app->itemCount) {
        return relativeY / ITEM_HEIGHT;
    }
    return -1;
}

void handleButtonPress(SimpleWindow* app, XButtonEvent event) {
    int x = event.x;
    int y = event.y;
    Window window = event.window;
    
    // Клик в комбо-боксе
    if (window == app->comboBox) {
        if (app->isOpen) {
            closeDropDown(app);
        } else {
            openDropDown(app);
        }
    }
    // Клик в выпадающем списке
    else if (window == app->dropDown && app->isOpen) {
        int itemIndex = getDropDownItemAtPos(app, y);
        if (itemIndex != -1) {
            selectItem(app, itemIndex);
            closeDropDown(app);
        }
    }
    // Клик в кнопке
    else if (x >= 150 && x <= 250 && y >= 85 && y <= 115) {
        app->buttonPressed = 1;
        drawButton(app);
        
        // Обработка нажатия кнопки
        if (app->selectedIndex != -1) {
            char newLabel[MAX_LABEL_LENGTH];
            snprintf(newLabel, sizeof(newLabel), "Город: %s", app->selectedText);
            strcpy(app->labelText, newLabel);
            drawLabel(app);
        }
    }
    // Клик вне элементов - закрываем выпадающий список
    else if (app->isOpen) {
        closeDropDown(app);
    }
}

void handleButtonRelease(SimpleWindow* app, XButtonEvent event) {
    if (app->buttonPressed) {
        app->buttonPressed = 0;
        drawButton(app);
    }
}

void run(SimpleWindow* app) {
    XEvent event;
    int running = 1;
    
    while (running) {
        XNextEvent(app->display, &event);
        
        switch (event.type) {
            case Expose:
                if (event.xexpose.window == app->window) {
                    drawWindow(app);
                } else if (event.xexpose.window == app->comboBox) {
                    drawComboBox(app);
                } else if (event.xexpose.window == app->dropDown) {
                    drawDropDown(app);
                }
                break;
                
            case ButtonPress:
                handleButtonPress(app, event.xbutton);
                break;
                
            case ButtonRelease:
                handleButtonRelease(app, event.xbutton);
                break;
                
            case ClientMessage:
                // Обработка закрытия окна
                running = 0;
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
        XDestroyWindow(app->display, app->dropDown);
        XDestroyWindow(app->display, app->comboBox);
        XDestroyWindow(app->display, app->window);
        XCloseDisplay(app->display);
        app->display = NULL;
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
