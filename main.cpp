#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

unsigned long LightGrayPixel(Display* display, int screen);

class SimpleWindow {
private:
    Display* display;
    Window window;
    GC gc;
    int screen;
    

    Window inputField;
    Window label;
    Window button;
    

    std::string inputText;
    std::string labelText;
    std::string buttonText;
    

    bool inputActive;
    bool buttonPressed;

public:
    SimpleWindow() : display(nullptr), inputText(""), labelText("text:"), buttonText("Enter"), 
                     inputActive(false), buttonPressed(false) {}
    
    bool initialize() {
     
        display = XOpenDisplay(NULL);
        if (display == NULL) {
            fprintf(stderr, "Cannot open X display\n");
            return false;
        }
        
        screen = DefaultScreen(display);
        
      
        window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                    100, 100, 400, 200, 1,
                                    BlackPixel(display, screen),
                                    WhitePixel(display, screen));
        

        XStoreName(display, window, "X11 Program for MINIX");
        

        XSelectInput(display, window, 
                    ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
        

        gc = XCreateGC(display, window, 0, NULL);
        XSetForeground(display, gc, BlackPixel(display, screen));
        

        createUIElements();
        
    
        XMapWindow(display, window);
        
        return true;
    }
    
    void createUIElements() {

        inputField = XCreateSimpleWindow(display, window,
                                        150, 50, 200, 25, 1,
                                        BlackPixel(display, screen),
                                        WhitePixel(display, screen));
        XSelectInput(display, inputField, ExposureMask | ButtonPressMask);
        
  
        label = XCreateSimpleWindow(display, window,
                                   50, 50, 90, 25, 0,
                                   WhitePixel(display, screen),
                                   WhitePixel(display, screen));
        XSelectInput(display, label, ExposureMask);
        

        button = XCreateSimpleWindow(display, window,
                                   150, 100, 100, 30, 1,
                                   BlackPixel(display, screen),
                                   LightGrayPixel(display, screen));
        XSelectInput(display, button, ExposureMask | ButtonPressMask);
        
    
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
        
  
        if (inputActive) {
            XSetForeground(display, gc, BlackPixel(display, screen));
            XDrawRectangle(display, inputField, gc, 0, 0, 199, 24);
        }
        
  
        XSetForeground(display, gc, BlackPixel(display, screen));
        if (!inputText.empty()) {
            XDrawString(display, inputField, gc, 5, 15, inputText.c_str(), inputText.length());
        }
        

        if (inputActive) {
            XFontStruct* font = XLoadQueryFont(display, "fixed");
            if (font) {
                int textWidth = XTextWidth(font, inputText.c_str(), inputText.length());
                XDrawLine(display, inputField, gc, 5 + textWidth, 5, 5 + textWidth, 20);
            }
        }
    }
    
    void drawButton() {
        XClearWindow(display, button);
        
   
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
        

        XDrawString(display, window, gc, 150, 30, "X11 Program", 11);
        

        drawLabel();
        drawInputField();
        drawButton();
    }
    
    void handleButtonPress(XButtonEvent event) {
        if (event.window == inputField) {
            inputActive = true;
            drawInputField();
        } 
        else if (event.window == button) {
            buttonPressed = true;
            drawButton();
            
            if (!inputText.empty()) {
                labelText = "text: " + inputText;
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
                if (!inputText.empty()) {
                    labelText = "text: " + inputText;
                    drawLabel();
                }
            }
            else if (buffer[0] >= 32 && buffer[0] <= 126) { 
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
                    
                    break;
            }
        }
    }
    
    ~SimpleWindow() {
        if (display) {
            XFreeGC(display, gc);
            if (inputField) XDestroyWindow(display, inputField);
            if (label) XDestroyWindow(display, label);
            if (button) XDestroyWindow(display, button);
            if (window) XDestroyWindow(display, window);
            XCloseDisplay(display);
        }
    }
};


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

int main() {
    SimpleWindow app;
    
    if (!app.initialize()) {
        return 1;
    }
    
    app.run();
    
    return 0;
}