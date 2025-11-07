#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXT_LENGTH 50
#define MAX_LABEL_LENGTH 100

typedef struct {
    Display* display;
    Window window;
    GC gc;
    int screen;
    
    char inputText[MAX_TEXT_LENGTH];
    char labelText[MAX_LABEL_LENGTH];
    char buttonText[20];
    
    int inputActive;
    int buttonPressed;
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
    app->inputText[0] = '\0';
    strncpy(app->labelText, "text:", MAX_LABEL_LENGTH - 1);
    app->labelText[MAX_LABEL_LENGTH - 1] = '\0';
    strncpy(app->buttonText, "Enter", sizeof(app->buttonText) - 1);
    app->buttonText[sizeof(app->buttonText) - 1] = '\0';
    
    app->inputActive = 0;
    app->buttonPressed = 0;
    
    app->display = XOpenDisplay(NULL);
    if (app->display == NULL) {
        fprintf(stderr, "Cannot open X display\n");
        return 0;
    }
    
    app->screen = DefaultScreen(app->display);
    
    app->window = XCreateSimpleWindow(app->display, RootWindow(app->display, app->screen),
                                    100, 100, 500, 250, 1,
                                    BlackPixel(app->display, app->screen),
                                    WhitePixel(app->display, app->screen));
    
    XStoreName(app->display, app->window, "X11 Program");
    
    XSelectInput(app->display, app->window, 
                ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
    
    app->gc = XCreateGC(app->display, app->window, 0, NULL);
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    
    XMapWindow(app->display, app->window);
    
    return 1;
}

void drawLabel(SimpleWindow* app) {
    XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
    XFillRectangle(app->display, app->window, app->gc, 70, 45, 350, 25);
    
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawString(app->display, app->window, app->gc, 80, 60, 
                app->labelText, strlen(app->labelText));
}

void drawInputField(SimpleWindow* app) {
    int inputX = 200;
    int inputY = 55;
    int inputWidth = 200;
    int inputHeight = 25;
    
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawRectangle(app->display, app->window, app->gc, 
                  inputX, inputY, inputWidth - 1, inputHeight - 1);
    
    XSetForeground(app->display, app->gc, WhitePixel(app->display, app->screen));
    XFillRectangle(app->display, app->window, app->gc, 
                  inputX + 1, inputY + 1, inputWidth - 2, inputHeight - 2);
    
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    if (strlen(app->inputText) > 0) {
        XDrawString(app->display, app->window, app->gc, 
                   inputX + 8, inputY + 16, app->inputText, strlen(app->inputText));
    }
    
    if (app->inputActive) {
        XFontStruct* font = XLoadQueryFont(app->display, "fixed");
        if (font) {
            int textWidth = XTextWidth(font, app->inputText, strlen(app->inputText));
            XDrawLine(app->display, app->window, app->gc, 
                     inputX + 8 + textWidth, inputY + 8, 
                     inputX + 8 + textWidth, inputY + 20);
            XFreeFont(app->display, font);
        }
    }
}

void drawButton(SimpleWindow* app) {
    int buttonX = 200;
    int buttonY = 100;
    int buttonWidth = 100;
    int buttonHeight = 30;
    
    if (app->buttonPressed) {
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
    
    XSetForeground(app->display, app->gc, LightGrayPixel(app->display, app->screen));
    XFillRectangle(app->display, app->window, app->gc, 
                  buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2);
    
    XFontStruct* font = XLoadQueryFont(app->display, "fixed");
    if (font) {
        int textWidth = XTextWidth(font, app->buttonText, strlen(app->buttonText));
        int x = buttonX + (buttonWidth - textWidth) / 2;
        int y = buttonY + (buttonHeight + 6) / 2;
        
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XSetFont(app->display, app->gc, font->fid);
        XDrawString(app->display, app->window, app->gc, x, y, 
                   app->buttonText, strlen(app->buttonText));
        XFreeFont(app->display, font);
    }
}

void drawWindow(SimpleWindow* app) {
    XClearWindow(app->display, app->window);
    
    XDrawString(app->display, app->window, app->gc, 180, 35, "X11 Program", 11);
    
    drawLabel(app);
    drawInputField(app);
    drawButton(app);
}

void handleButtonPress(SimpleWindow* app, XButtonEvent event) {
    int x = event.x;
    int y = event.y;
    
    int inputX = 200;
    int inputY = 55;
    int inputWidth = 200;
    int inputHeight = 25;
    
    int buttonX = 200;
    int buttonY = 100;
    int buttonWidth = 100;
    int buttonHeight = 30;
    
    if (x >= inputX && x <= inputX + inputWidth && 
        y >= inputY && y <= inputY + inputHeight) {
        app->inputActive = 1;
        drawInputField(app);
    } 
    else if (x >= buttonX && x <= buttonX + buttonWidth && 
             y >= buttonY && y <= buttonY + buttonHeight) {
        app->buttonPressed = 1;
        drawButton(app);
        
        if (strlen(app->inputText) > 0) {
            char newLabel[MAX_LABEL_LENGTH];
            const char* prefix = "text: ";
            int availableSpace = MAX_LABEL_LENGTH - strlen(prefix) - 1;
            
            strcpy(newLabel, prefix);
            strncat(newLabel, app->inputText, availableSpace);
            newLabel[MAX_LABEL_LENGTH - 1] = '\0';
            
            strcpy(app->labelText, newLabel);
            drawLabel(app);
        }
    }
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
            if (strlen(app->inputText) > 0) {
                char newLabel[MAX_LABEL_LENGTH];
                const char* prefix = "text: ";
                int availableSpace = MAX_LABEL_LENGTH - strlen(prefix) - 1;
                
                strcpy(newLabel, prefix);
                strncat(newLabel, app->inputText, availableSpace);
                newLabel[MAX_LABEL_LENGTH - 1] = '\0';
                
                strcpy(app->labelText, newLabel);
                strcpy(app->inputText, "");
                drawLabel(app);
                drawInputField(app);
            }
        }
        else if (buffer[0] >= 32 && buffer[0] <= 126) {
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
                
            case ClientMessage:
                running = 0;
                break;
        }
    }
}

void cleanup(SimpleWindow* app) {
    if (app->display) {
        XFreeGC(app->display, app->gc);
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
