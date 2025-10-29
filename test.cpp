#include <X11/Xlib.h>
#include <X11/Xutil.h>
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
    
    int inputX, inputY, inputWidth, inputHeight;
    int labelX, labelY, labelWidth, labelHeight;
    int buttonX, buttonY, buttonWidth, buttonHeight;
    
    std::string inputText;
    std::string labelText;
    std::string buttonText;
    
    bool inputActive;
    bool buttonPressed;

public:
    SimpleWindow() : 
        inputX(150), inputY(50), inputWidth(200), inputHeight(25),
        labelX(50), labelY(50), labelWidth(90), labelHeight(25),
        buttonX(150), buttonY(100), buttonWidth(100), buttonHeight(30),
        inputText(""), labelText("Введите текст:"), buttonText("Применить"), 
        inputActive(false), buttonPressed(false) {}
    
    bool initialize() {
        display = XOpenDisplay(NULL);
        if (display == NULL) {
            fprintf(stderr, "Не удалось открыть дисплей X\n");
            return false;
        }
        
        screen = DefaultScreen(display);
        
        window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                    100, 100, 400, 200, 2,
                                    BlackPixel(display, screen),
                                    WhitePixel(display, screen));
        
        XStoreName(display, window, "X11 MINIX Program");
        
        XSelectInput(display, window, 
                    ExposureMask | KeyPressMask | ButtonPressMask | KeyReleaseMask | ButtonReleaseMask);
        
        XGCValues values;
        values.foreground = BlackPixel(display, screen);
        values.background = WhitePixel(display, screen);
        gc = XCreateGC(display, window, GCForeground | GCBackground, &values);
        
        XMapWindow(display, window);
        XFlush(display);
        
        return true;
    }
    
    void drawLabel() {
        XSetForeground(display, gc, BlackPixel(display, screen));
        XDrawString(display, window, gc, labelX + 5, labelY + 15, labelText.c_str(), labelText.length());
    }
    
    void drawInputField() {
        XSetForeground(display, gc, BlackPixel(display, screen));
        XDrawRectangle(display, window, gc, inputX, inputY, inputWidth, inputHeight);
        
        XSetForeground(display, gc, WhitePixel(display, screen));
        XFillRectangle(display, window, gc, inputX + 1, inputY + 1, inputWidth - 2, inputHeight - 2);
        
        XSetForeground(display, gc, BlackPixel(display, screen));
        if (!inputText.empty()) {
            XDrawString(display, window, gc, inputX + 5, inputY + 15, inputText.c_str(), inputText.length());
        }
        
        if (inputActive) {
            int textWidth = 8 * inputText.length(); 
            XDrawLine(display, window, gc, inputX + 5 + textWidth, inputY + 5, 
                     inputX + 5 + textWidth, inputY + 20);
        }
    }
    
    void drawButton() {
        XSetForeground(display, gc, BlackPixel(display, screen));
        XDrawRectangle(display, window, gc, buttonX, buttonY, buttonWidth, buttonHeight);
        
        if (buttonPressed) {
            XSetForeground(display, gc, BlackPixel(display, screen));
        } else {
            XSetForeground(display, gc, WhitePixel(display, screen));
        }
        XFillRectangle(display, window, gc, buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2);
        
        XSetForeground(display, gc, BlackPixel(display, screen));
        int textX = buttonX + (buttonWidth - 8 * buttonText.length()) / 2;
        int textY = buttonY + buttonHeight / 2 + 4;
        XDrawString(display, window, gc, textX, textY, buttonText.c_str(), buttonText.length());
    }
    
    void drawWindow() {
        XClearWindow(display, window);
        
        XDrawString(display, window, gc, 150, 30, "MINIX X11 Program", 17);
        
        drawLabel();
        drawInputField();
        drawButton();
        
        XFlush(display);
    }
    
    bool isInInputField(int x, int y) {
        return (x >= inputX && x <= inputX + inputWidth && 
                y >= inputY && y <= inputY + inputHeight);
    }
    
    bool isInButton(int x, int y) {
        return (x >= buttonX && x <= buttonX + buttonWidth && 
                y >= buttonY && y <= buttonY + buttonHeight);
    }
    
    void handleButtonPress(int x, int y) {
        if (isInInputField(x, y)) {
            inputActive = true;
            drawInputField();
        } 
        else if (isInButton(x, y)) {
            buttonPressed = true;
            drawButton();
        }
        else {
            inputActive = false;
            drawInputField();
        }
        XFlush(display);
    }
    
    void handleButtonRelease(int x, int y) {
        if (buttonPressed && isInButton(x, y)) {
            if (!inputText.empty()) {
                labelText = "Text: " + inputText;
                drawLabel();
            }
        }
        buttonPressed = false;
        drawButton();
        XFlush(display);
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
                if (!inputText.empty()) {
                    labelText = "Text: " + inputText;
                    drawLabel();
                }
            }
            else if (buffer[0] >= 32 && buffer[0] <= 126) { 
                inputText += buffer[0];
            }
            
            drawInputField();
            XFlush(display);
        }
    }
    
    void run() {
        XEvent event;
        bool running = true;
        
        while (running) {
            XNextEvent(display, &event);
            
            switch (event.type) {
                case Expose:
                    drawWindow();
                    break;
                    
                case ButtonPress:
                    handleButtonPress(event.xbutton.x, event.xbutton.y);
                    break;
                    
                case ButtonRelease:
                    handleButtonRelease(event.xbutton.x, event.xbutton.y);
                    break;
                    
                case KeyPress:
                    handleKeyPress(event.xkey);
                    break;
                    
                case KeyRelease:
                    break;
            }
        }
    }
    
    ~SimpleWindow() {
        if (display) {
            XFreeGC(display, gc);
            XDestroyWindow(display, window);
            XCloseDisplay(display);
        }
    }
};

int main() {
    SimpleWindow app;
    
    if (!app.initialize()) {
        fprintf(stderr, "Ошибка инициализации приложения\n");
        return 1;
    }
    
    app.run();
    
    return 0;
}
