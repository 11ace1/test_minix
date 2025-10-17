#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <cstdlib>
#include <string>

using namespace std;

int main() {
    cout << "start x11" << endl;


    Display* display = XOpenDisplay(NULL);
    if (!display) {
        cerr << "error" << endl;
        cerr << "rer" << endl;
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    Window window = XCreateSimpleWindow(
        display, root,
        100, 100, 400, 300,  
        2,                    
        BlackPixel(display, screen),
        WhitePixel(display, screen)
    );

    XSelectInput(display, window, 
        ExposureMask | KeyPressMask | ButtonPressMask);

    XMapWindow(display, window);

    GC gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, BlackPixel(display, screen));

    XEvent event;
    string message = "Hello, Minix X11!";
    bool running = true;

    cout << "close window" << endl;

    while (running) {
        XNextEvent(display, &event);

        switch (event.type) {
            case Expose:
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

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    cout << "finish" << endl;
    return 0;
}