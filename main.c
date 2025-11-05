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
    
    char selectedText[MAX_TEXT_LENGTH];
    char labelText[MAX_LABEL_LENGTH];
    
    Window comboBox;
    Window dropDown;
    int isOpen;
    int selectedIndex;
    char items[MAX_ITEMS][50];
    int itemCount;
} SimpleWindow;

unsigned long LightGrayPixel(Display* display, int screen) {
    XColor color;
    Colormap colormap = DefaultColormap(display, screen);
    
    color.red = 30000;
    color.green = 30000;
    color.blue = 30000;
    color.flags = DoRed | DoGreen | DoBlue;
    
    if (XAllocColor(display, colormap, &color)) {
        return color.pixel;
    }
    
    return WhitePixel(display, screen);
}

int initialize(SimpleWindow* app) {
    strncpy(app->selectedText, "Select city", MAX_TEXT_LENGTH - 1);
    app->selectedText[MAX_TEXT_LENGTH - 1] = '\0';
    strncpy(app->labelText, "City:", MAX_LABEL_LENGTH - 1);
    app->labelText[MAX_LABEL_LENGTH - 1] = '\0';
    
    app->isOpen = 0;
    app->selectedIndex = -1;
    app->itemCount = 4;
    strcpy(app->items[0], "Moscow");
    strcpy(app->items[1], "Ryazan");
    strcpy(app->items[2], "Saint Petersburg");
    strcpy(app->items[3], "Vladivostok");
    
    app->display = XOpenDisplay(NULL);
    if (app->display == NULL) {
        fprintf(stderr, "Cannot open X display\n");
        return 0;
    }
    
    app->screen = DefaultScreen(app->display);
    
    app->window = XCreateSimpleWindow(app->display, RootWindow(app->display, app->screen),
                                    100, 100, 400, 300, 1,
                                    BlackPixel(app->display, app->screen),
                                    WhitePixel(app->display, app->screen));
    
    app->comboBox = XCreateSimpleWindow(app->display, app->window,
                                      150, 45, COMBO_WIDTH, COMBO_HEIGHT, 1,
                                      BlackPixel(app->display, app->screen),
                                      WhitePixel(app->display, app->screen));
    
    app->dropDown = XCreateSimpleWindow(app->display, app->window,
                                      150, 70, COMBO_WIDTH, ITEM_HEIGHT * MAX_ITEMS, 1,
                                      BlackPixel(app->display, app->screen),
                                      WhitePixel(app->display, app->screen));
    
    XStoreName(app->display, app->window, "City Selector");
    
    XSelectInput(app->display, app->window, 
                ExposureMask | ButtonPressMask | StructureNotifyMask);
    XSelectInput(app->display, app->comboBox, ExposureMask | ButtonPressMask);
    XSelectInput(app->display, app->dropDown, ExposureMask | ButtonPressMask);
    
    app->gc = XCreateGC(app->display, app->window, 0, NULL);
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    
    XMapWindow(app->display, app->window);
    XMapWindow(app->display, app->comboBox);
    XUnmapWindow(app->display, app->dropDown);
    
    return 1;
}

void drawLabel(SimpleWindow* app) {
    XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
    XFillRectangle(app->display, app->window, app->gc, 45, 35, 300, 20);
    
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawString(app->display, app->window, app->gc, 50, 50, 
                app->labelText, strlen(app->labelText));
}

void drawComboBox(SimpleWindow* app) {
    XClearWindow(app->display, app->comboBox);
    
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawRectangle(app->display, app->comboBox, app->gc, 0, 0, COMBO_WIDTH-1, COMBO_HEIGHT-1);
    
    XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
    XFillRectangle(app->display, app->comboBox, app->gc, 1, 1, COMBO_WIDTH-2, COMBO_HEIGHT-2);
    
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawString(app->display, app->comboBox, app->gc, 5, 15, 
                app->selectedText, strlen(app->selectedText));
    
    int arrowX = COMBO_WIDTH - 15;
    int arrowY = COMBO_HEIGHT / 2;
    
    if (app->isOpen) {
        XDrawLine(app->display, app->comboBox, app->gc, arrowX, arrowY + 3, arrowX + 5, arrowY - 3);
        XDrawLine(app->display, app->comboBox, app->gc, arrowX + 5, arrowY - 3, arrowX + 10, arrowY + 3);
    } else {
        XDrawLine(app->display, app->comboBox, app->gc, arrowX, arrowY - 3, arrowX + 5, arrowY + 3);
        XDrawLine(app->display, app->comboBox, app->gc, arrowX + 5, arrowY + 3, arrowX + 10, arrowY - 3);
    }
}

void drawDropDown(SimpleWindow* app) {
    if (!app->isOpen) return;
    
    XClearWindow(app->display, app->dropDown);
    
    for (int i = 0; i < app->itemCount; i++) {
        int y = i * ITEM_HEIGHT;
        
        if (i == app->selectedIndex) {
            XSetForeground(app->display, app->gc, LightGrayPixel(app->display, app->screen));
            XFillRectangle(app->display, app->dropDown, app->gc, 1, y + 1, COMBO_WIDTH - 2, ITEM_HEIGHT - 2);
        } else {
            XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
            XFillRectangle(app->display, app->dropDown, app->gc, 1, y + 1, COMBO_WIDTH - 2, ITEM_HEIGHT - 2);
        }
        
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XDrawString(app->display, app->dropDown, app->gc, 5, y + 15, 
                    app->items[i], strlen(app->items[i]));
    }
    
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawRectangle(app->display, app->dropDown, app->gc, 0, 0, COMBO_WIDTH-1, ITEM_HEIGHT * app->itemCount);
}

void drawWindow(SimpleWindow* app) {
    XClearWindow(app->display, app->window);
    
    XDrawString(app->display, app->window, app->gc, 150, 30, "Select City", 11);
    
    drawLabel(app);
    drawComboBox(app);
    drawDropDown(app);
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
        
        char newLabel[MAX_LABEL_LENGTH];
        snprintf(newLabel, sizeof(newLabel), "City: %s", app->selectedText);
        strcpy(app->labelText, newLabel);
        drawLabel(app);
        
        drawComboBox(app);
    }
}

void handleButtonPress(SimpleWindow* app, XButtonEvent event) {
    int x = event.x;
    int y = event.y;
    Window window = event.window;
    
    if (window == app->comboBox) {
        if (app->isOpen) {
            closeDropDown(app);
        } else {
            openDropDown(app);
        }
    }
    else if (window == app->dropDown && app->isOpen) {
        int itemIndex = y / ITEM_HEIGHT;
        
        if (itemIndex >= 0 && itemIndex < app->itemCount) {
            selectItem(app, itemIndex);
            closeDropDown(app);
        }
    }
    else if (app->isOpen) {
        closeDropDown(app);
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
                
            case ClientMessage:
                running = 0;
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
