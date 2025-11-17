// file_manager.c
// Простой графический файловый менеджер для Minix на X11
// Компиляция: cc file_manager.c -o file_manager -lX11

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define WIN_W 800
#define WIN_H 600

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define MARGIN 10
#define HEADER_H 40
#define FOOTER_H 40
#define ITEM_H 20
#define VISIBLE_ITEMS ((WIN_H - HEADER_H - FOOTER_H - 2*MARGIN) / ITEM_H)
#define PATH_MAX_LEN 1024
#define FILE_PREVIEW_LIMIT 200000 // bytes

typedef struct {
    char name[PATH_MAX];
    int is_dir;
    off_t size;
    time_t mtime;
} FileEntry;

typedef struct {
    Display *dpy;
    int screen;
    Window win;
    GC gc;
    XFontStruct *font;
    int win_w, win_h;

    char cwd[PATH_MAX];
    FileEntry *entries;
    int entry_count;
    int capacity;

    int scroll; // index of first visible entry
    int selected; // index in entries
    int mouse_down;
    Time last_click_time;
} FMApp;

// ---------- utility ----------
static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static void ensure_capacity(FMApp *app) {
    if (!app->entries) {
        app->capacity = 256;
        app->entries = calloc(app->capacity, sizeof(FileEntry));
    } else if (app->entry_count >= app->capacity) {
        app->capacity *= 2;
        app->entries = realloc(app->entries, app->capacity * sizeof(FileEntry));
    }
}

// ---------- directory listing ----------
static int compare_entries(const void *a, const void *b) {
    const FileEntry *A = a;
    const FileEntry *B = b;
    // Directories first, then case-insensitive name
    if (A->is_dir != B->is_dir) return B->is_dir - A->is_dir;
    return strcasecmp(A->name, B->name);
}

static int load_directory(FMApp *app, const char *path) {
    DIR *d = opendir(path);
    if (!d) return -1;

    app->entry_count = 0;
    struct dirent *de;
    while ((de = readdir(d))) {
        // skip . and ..
        if (strcmp(de->d_name, ".") == 0) continue;
        ensure_capacity(app);
        FileEntry *e = &app->entries[app->entry_count];
        snprintf(e->name, sizeof(e->name), "%s", de->d_name);
        // stat
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", path, de->d_name);
        struct stat st;
        if (lstat(full, &st) == 0) {
            e->is_dir = S_ISDIR(st.st_mode);
            e->size = st.st_size;
            e->mtime = st.st_mtime;
        } else {
            e->is_dir = 0;
            e->size = 0;
            e->mtime = 0;
        }
        app->entry_count++;
    }
    closedir(d);
    qsort(app->entries, app->entry_count, sizeof(FileEntry), compare_entries);
    app->scroll = 0;
    app->selected = (app->entry_count>0)?0:-1;
    return 0;
}

// ---------- text measurement ----------
static int text_w(FMApp *app, const char *s) {
    if (!s || !app->font) return 0;
    return XTextWidth(app->font, s, strlen(s));
}

// ---------- UI drawing ----------
static void draw_header(FMApp *app) {
    int y = MARGIN + app->font->ascent;
    char title[PATH_MAX + 32];
    snprintf(title, sizeof(title), "Minix File Manager — %s", app->cwd);
    XDrawString(app->dpy, app->win, app->gc, MARGIN, y, title, strlen(title));

    // buttons: Up, Refresh, Exit
    int bx = WIN_W - MARGIN - 60;
    XDrawRectangle(app->dpy, app->win, app->gc, bx, MARGIN, 60, HEADER_H - 2*MARGIN);
    XDrawString(app->dpy, app->win, app->gc, bx + 8, y, "Exit", 4);
    bx -= 80;
    XDrawRectangle(app->dpy, app->win, app->gc, bx, MARGIN, 70, HEADER_H - 2*MARGIN);
    XDrawString(app->dpy, app->win, app->gc, bx + 8, y, "Refresh", 7);
    bx -= 80;
    XDrawRectangle(app->dpy, app->win, app->gc, bx, MARGIN, 70, HEADER_H - 2*MARGIN);
    XDrawString(app->dpy, app->win, app->gc, bx + 8, y, "Up", 2);
}

static void draw_footer(FMApp *app) {
    int y = WIN_H - FOOTER_H + MARGIN + app->font->ascent;
    char buf[256] = {0};
    if (app->selected >= 0 && app->selected < app->entry_count) {
        FileEntry *e = &app->entries[app->selected];
        char timebuf[64];
        struct tm tm;
        localtime_r(&e->mtime, &tm);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M", &tm);
        snprintf(buf, sizeof(buf), "%s  %s  %lld bytes", e->name, timebuf, (long long)e->size);
    } else {
        snprintf(buf, sizeof(buf), "Entries: %d", app->entry_count);
    }
    XDrawString(app->dpy, app->win, app->gc, MARGIN, y, buf, strlen(buf));
}

static void draw_list(FMApp *app) {
    int start_y = HEADER_H + MARGIN;
    int x = MARGIN;
    int w = WIN_W - 2*MARGIN;
    // background clear area
    XClearArea(app->dpy, app->win, x, start_y, w, WIN_H - HEADER_H - FOOTER_H - 2*MARGIN, False);

    int max_visible = VISIBLE_ITEMS;
    for (int i = 0; i < max_visible; i++) {
        int idx = app->scroll + i;
        if (idx >= app->entry_count) break;
        int y = start_y + i * ITEM_H + app->font->ascent;
        // highlight selection
        if (idx == app->selected) {
            XSetForeground(app->dpy, app->gc, WhitePixel(app->dpy, app->screen));
            XFillRectangle(app->dpy, app->win, app->gc, x, start_y + i*ITEM_H, w, ITEM_H);
            XSetForeground(app->dpy, app->gc, BlackPixel(app->dpy, app->screen));
        }
        FileEntry *e = &app->entries[idx];
        // icon as text
        const char *icon = e->is_dir ? "[DIR]" : "     ";
        XDrawString(app->dpy, app->win, app->gc, x, y, icon, strlen(icon));
        XDrawString(app->dpy, app->win, app->gc, x + 60, y, e->name, strlen(e->name));
        // size on right
        if (!e->is_dir) {
            char sizestr[32];
            snprintf(sizestr, sizeof(sizestr), "%lld", (long long)e->size);
            int tw = text_w(app, sizestr);
            XDrawString(app->dpy, app->win, app->gc, WIN_W - MARGIN - tw, y, sizestr, strlen(sizestr));
        }
    }
}

// ---------- file preview window ----------
static void view_text_file(FMApp *app, const char *fullpath) {
    // Read text file up to limit
    FILE *f = fopen(fullpath, "rb");
    if (!f) {
        // show small error box
        XClearWindow(app->dpy, app->win);
        XDrawString(app->dpy, app->win, app->gc, MARGIN, HEADER_H + MARGIN + app->font->ascent, "Cannot open file", 15);
        return;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz > FILE_PREVIEW_LIMIT) sz = FILE_PREVIEW_LIMIT;
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    fclose(f);
    buf[sz] = '\0';

    // Create simple modal window
    int ww = 600, wh = 400;
    Window view = XCreateSimpleWindow(app->dpy, RootWindow(app->dpy, app->screen),
                                      150, 100, ww, wh, 1,
                                      BlackPixel(app->dpy, app->screen),
                                      WhitePixel(app->dpy, app->screen));
    XMapWindow(app->dpy, view);
    GC gc = XCreateGC(app->dpy, view, 0, NULL);
    XSetFont(app->dpy, gc, app->font->fid);
    XSetForeground(app->dpy, gc, BlackPixel(app->dpy, app->screen));

    // Handle events for this view window
    XEvent ev;
    int running = 1;
    int line_h = ITEM_H;
    int max_lines = (wh - 20) / line_h;
    // simple wrap: print text line by line by newline
    char *p = buf;
    int offset = 0;
    int top_line = 0;
    // Build lines
    char **lines = malloc(sizeof(char*) * (max_lines*10 + 1000));
    int lines_count = 0;
    char *tok = strtok(p, "\n");
    while (tok && lines_count < 10000) {
        lines[lines_count++] = strdup(tok);
        tok = strtok(NULL, "\n");
    }

    while (running) {
        // draw contents
        XClearWindow(app->dpy, view);
        // header
        XDrawString(app->dpy, view, gc, 10, 15, fullpath, strlen(fullpath));
        for (int i = 0; i < max_lines; i++) {
            int li = top_line + i;
            if (li >= lines_count) break;
            XDrawString(app->dpy, view, gc, 10, 35 + i*line_h, lines[li], strlen(lines[li]));
        }
        // event loop with timeout
        while (XPending(app->dpy)) {
            XNextEvent(app->dpy, &ev);
            if (ev.type == KeyPress) {
                KeySym ks = XLookupKeysym(&ev.xkey, 0);
                if (ks == XK_q || ks == XK_Escape) {
                    running = 0;
                    break;
                } else if (ks == XK_Down) {
                    if (top_line + max_lines < lines_count) top_line++;
                } else if (ks == XK_Up) {
                    if (top_line > 0) top_line--;
                } else if (ks == XK_Page_Down) {
                    top_line += max_lines;
                    if (top_line + max_lines > lines_count) top_line = lines_count - max_lines;
                    if (top_line < 0) top_line = 0;
                } else if (ks == XK_Page_Up) {
                    top_line -= max_lines;
                    if (top_line < 0) top_line = 0;
                }
            } else if (ev.type == ButtonPress) {
                // close on click
                running = 0;
            } else if (ev.type == DestroyNotify) {
                running = 0;
            }
        }
        // small sleep to avoid busy loop
        usleep(10000);
    }

    // cleanup
    for (int i = 0; i < lines_count; i++) free(lines[i]);
    free(lines);
    XDestroyWindow(app->dpy, view);
    XFreeGC(app->dpy, gc);
    free(buf);
}

// ---------- interactions ----------
static void change_directory(FMApp *app, const char *newpath) {
    if (!newpath) return;
    if (chdir(newpath) != 0) return;
    getcwd(app->cwd, sizeof(app->cwd));
    load_directory(app, app->cwd);
}

static void open_selected(FMApp *app) {
    if (app->selected < 0 || app->selected >= app->entry_count) return;
    FileEntry *e = &app->entries[app->selected];
    char full[PATH_MAX];
    snprintf(full, sizeof(full), "%s/%s", app->cwd, e->name);
    if (e->is_dir) {
        change_directory(app, full);
    } else {
        // try to detect text file: read first bytes and check for NUL bytes
        FILE *f = fopen(full, "rb");
        if (!f) return;
        int is_text = 1;
        for (int i = 0; i < 512; i++) {
            int c = fgetc(f);
            if (c == EOF) break;
            if (c == 0) { is_text = 0; break; }
        }
        fclose(f);
        if (is_text) {
            view_text_file(app, full);
        } else {
            // show small modal with message (reuse footer area)
            XClearWindow(app->dpy, app->win);
            XDrawString(app->dpy, app->win, app->gc, MARGIN, HEADER_H + MARGIN + app->font->ascent, "Binary file cannot be previewed.", 31);
            XFlush(app->dpy);
            sleep(1);
        }
    }
}

// ---------- event handling ----------
static void handle_button(FMApp *app, XButtonEvent *bev) {
    // Buttons in header: compute their bounds
    int bx1 = WIN_W - MARGIN - 60;
    int bx2 = bx1 + 60;
    int by1 = MARGIN;
    int by2 = HEADER_H - MARGIN;
    if (bev->y >= by1 && bev->y <= by2) {
        // check which button
        int bx = bev->x;
        if (bx >= bx1 && bx <= bx2) {
            // Exit
            XCloseDisplay(app->dpy);
            exit(0);
        }
        bx1 -= 80; bx2 = bx1 + 70;
        if (bx >= bx1 && bx <= bx2) {
            // Refresh
            load_directory(app, app->cwd);
            return;
        }
        bx1 -= 80; bx2 = bx1 + 70;
        if (bx >= bx1 && bx <= bx2) {
            // Up
            char parent[PATH_MAX];
            snprintf(parent, sizeof(parent), "%s/..", app->cwd);
            change_directory(app, parent);
            return;
        }
    }

    // click in list area?
    int list_x = MARGIN;
    int list_y = HEADER_H + MARGIN;
    int list_w = WIN_W - 2*MARGIN;
    int list_h = WIN_H - HEADER_H - FOOTER_H - 2*MARGIN;
    if (bev->x >= list_x && bev->x <= list_x + list_w &&
        bev->y >= list_y && bev->y <= list_y + list_h) {
        int rel = bev->y - list_y;
        int idx = app->scroll + rel / ITEM_H;
        if (idx < 0) idx = 0;
        if (idx >= app->entry_count) idx = app->entry_count - 1;
        // selection
        if (app->selected == idx && (bev->time - app->last_click_time) < 400) {
            // double click -> open
            open_selected(app);
        } else {
            app->selected = idx;
        }
        app->last_click_time = bev->time;
    }
}

static void handle_key(FMApp *app, XKeyEvent *kev) {
    KeySym ks = XLookupKeysym(kev, 0);
    if (ks == XK_q || ks == XK_Escape) {
        XCloseDisplay(app->dpy);
        exit(0);
    } else if (ks == XK_Down) {
        if (app->selected + 1 < app->entry_count) app->selected++;
        int bottom = app->scroll + VISIBLE_ITEMS - 1;
        if (app->selected > bottom) app->scroll++;
    } else if (ks == XK_Up) {
        if (app->selected > 0) app->selected--;
        if (app->selected < app->scroll) app->scroll--;
    } else if (ks == XK_Return) {
        open_selected(app);
    } else if (ks == XK_r) {
        load_directory(app, app->cwd);
    } else if (ks == XK_BackSpace) {
        char parent[PATH_MAX];
        snprintf(parent, sizeof(parent), "%s/..", app->cwd);
        change_directory(app, parent);
    } else if (ks == XK_Page_Down) {
        app->scroll += VISIBLE_ITEMS;
        if (app->scroll < 0) app->scroll = 0;
        if (app->scroll >= app->entry_count) app->scroll = app->entry_count - 1;
    } else if (ks == XK_Page_Up) {
        app->scroll -= VISIBLE_ITEMS;
        if (app->scroll < 0) app->scroll = 0;
    } else if (ks == XK_Home) {
        app->scroll = 0;
        app->selected = 0;
    } else if (ks == XK_End) {
        app->selected = app->entry_count - 1;
        app->scroll = app->entry_count - VISIBLE_ITEMS;
        if (app->scroll < 0) app->scroll = 0;
    }
}

// ---------- init and main ----------
int main(int argc, char **argv) {
    FMApp app;
    memset(&app, 0, sizeof(app));
    app.win_w = WIN_W; app.win_h = WIN_H;

    app.dpy = XOpenDisplay(NULL);
    if (!app.dpy) die("XOpenDisplay");
    app.screen = DefaultScreen(app.dpy);
    app.win = XCreateSimpleWindow(app.dpy, RootWindow(app.dpy, app.screen),
                                  100, 100, WIN_W, WIN_H, 1,
                                  BlackPixel(app.dpy, app.screen), WhitePixel(app.dpy, app.screen));
    XStoreName(app.dpy, app.win, "Minix File Manager");
    XSelectInput(app.dpy, app.win, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask);
    XMapWindow(app.dpy, app.win);

    app.gc = XCreateGC(app.dpy, app.win, 0, NULL);

    // try load font
    app.font = XLoadQueryFont(app.dpy, "6x13");
    if (!app.font) app.font = XLoadQueryFont(app.dpy, "fixed");
    if (!app.font) die("No font");

    XSetFont(app.dpy, app.gc, app.font->fid);
    XSetForeground(app.dpy, app.gc, BlackPixel(app.dpy, app.screen));

    if (!getcwd(app.cwd, sizeof(app.cwd))) strcpy(app.cwd, "/");
    if (argc > 1) {
        if (chdir(argv[1]) == 0) getcwd(app.cwd, sizeof(app.cwd));
    }

    load_directory(&app, app.cwd);

    // main loop
    XEvent ev;
    while (1) {
        // draw
        draw_header(&app);
        draw_list(&app);
        draw_footer(&app);
        XFlush(app.dpy);

        XNextEvent(app.dpy, &ev);
        if (ev.type == Expose) {
            // redraw handled at top of loop
        } else if (ev.type == ButtonPress) {
            handle_button(&app, &ev.xbutton);
        } else if (ev.type == KeyPress) {
            handle_key(&app, &ev.xkey);
        }
    }

    // cleanup (never reached)
    if (app.entries) free(app.entries);
    if (app.font) XFreeFont(app.dpy, app.font);
    if (app.gc) XFreeGC(app.dpy, app.gc);
    if (app.dpy) XCloseDisplay(app.dpy);
    return 0;
}
