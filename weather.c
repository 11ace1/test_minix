#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define WINDOW_WIDTH  600
#define WINDOW_HEIGHT 600
#define BUFFER_SIZE 4096
#define MAX_CITY_LENGTH 50
/* ===== Cloud background XPM ===== */
static const char *cloud_xpm[] = {
"96 15 3 1",
" 	c None",
".	c #87CEEB",
"+	c #FFFFFF",
"........................................................................................................",
"........................................................................................................",
"........+++++++++++++++.................................................................................",
"......++++++++++++++++++................................................................................",
".....++++++++++++++++++++...............................................................................",
"....+++++++......++++++++...............................................................................",
"...++++++.........+++++++...............................................................................",
"...+++++...........++++++..............................................................................",
"...+++++...........+++++++.............................................................................",
"....+++++.........++++++++..............................................................................",
".....++++++.....+++++++++...............................................................................",
".......+++++++++++++++++...............................................................................",
".........++++++++++++...................................................................................",
"........................................................................................................",
"........................................................................................................"
};


#define OPENWEATHER_API_KEY "68682c4ce7b5e11bfcefb6a4af50e437"
#define DEFAULT_CITY "Ryazan"

typedef struct {
    Display* display;
    Window window;
    GC gc;
    int screen;
    Pixmap cloud_pixmap;
    Pixmap cloud_mask;
    int cloud_w;
    int cloud_h;
    XFontStruct* main_font;
    
    char temperature[20];
    char feels_like[20];
    char humidity[20];
    char description[50];
    char error[100];
    char city[MAX_CITY_LENGTH];
    char input_city[MAX_CITY_LENGTH];
    int input_active;
} WeatherApp;

int get_weather_mock(WeatherApp* app, const char* city);
int http_get(const char* host, const char* path, char* response, int response_size);
int parse_weather_json(const char* json, WeatherApp* app);
int get_weather(WeatherApp* app, const char* api_key, const char* city);
void draw_weather(WeatherApp* app);
void initialize_app(WeatherApp* app);
void cleanup_app(WeatherApp* app);
void handle_key_press(WeatherApp* app, XKeyEvent event, const char** current_city);

XFontStruct* load_large_font(Display* display) {
    printf("Searching for LARGE fonts...\n");
    
    const char* large_fonts[] = {
        "6x13", "6x12", "6x10", "8x16bold",
        "9x15bold", "10x20bold", "fixed", "variable", NULL
    };
    
    for (int i = 0; large_fonts[i] != NULL; i++) {
        XFontStruct* font = XLoadQueryFont(display, large_fonts[i]);
        if (font) {
            int font_height = font->ascent + font->descent;
            printf("✓ Loaded font: %s (height: %d)\n", large_fonts[i], font_height);
            
            if (font_height >= 15) {
                return font;
            }
            XFreeFont(display, font);
        }
    }
    
    // Fallback
    XFontStruct* font = XLoadQueryFont(display, "fixed");
    if (font) return font;
    
    printf("ERROR: No fonts available!\n");
    return NULL;
}

// Функция для записи HTTP ответа
size_t write_callback(void* ptr, size_t size, size_t nmemb, char* response) {
    size_t total_size = size * nmemb;
    static size_t total_received = 0;
    
    if (total_received + total_size >= BUFFER_SIZE - 1) {
        return 0;
    }
    
    memcpy(response + total_received, ptr, total_size);
    total_received += total_size;
    response[total_received] = '\0';
    
    return total_size;
}

// функция для HTTP GET запроса
int http_get(const char* host, const char* path, char* response, int response_size) {
    int sockfd;
    struct hostent* server;
    struct sockaddr_in serv_addr;
    
    printf("Connecting to %s...\n", host);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //Создание TCP-сокета
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    server = gethostbyname(host); //Преобразование доменного имени в IP-адрес
    if (server == NULL) {
        fprintf(stderr, "Error: no such host %s\n", host);
        close(sockfd);
        return -1;
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(80); // HTTP
    
    printf("Attempting connection to %s...\n", host);
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    // Формируем HTTP запрос
    char request[1024];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: Minix3-Weather/1.0\r\n"
             "Connection: close\r\n\r\n",
             path, host);
    
    printf("Sending request...\nPath: %s\n", path);
    if (write(sockfd, request, strlen(request)) < 0) {
        perror("write");
        close(sockfd);
        return -1;
    }
    
    // Читаем ответ
    int total_read = 0;
    int n;
    char buffer[1024];
    int header_ended = 0;
    
    printf("Reading response...\n");
    while ((n = read(sockfd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        
        // Пропускаем HTTP заголовки
        if (!header_ended) {
            char* header_end = strstr(buffer, "\r\n\r\n");
            if (header_end) {
                header_ended = 1;
                int body_start = header_end - buffer + 4;
                int body_size = n - body_start;
                if (body_size > 0 && total_read + body_size < response_size) {
                    memcpy(response + total_read, buffer + body_start, body_size);
                    total_read += body_size;
                }
            }
        } else {
            if (total_read + n < response_size) {
                memcpy(response + total_read, buffer, n);
                total_read += n;
            }
        }
    }
    
    if (n < 0) {
        perror("read");
        close(sockfd);
        return -1;
    }
    
    response[total_read] = '\0';
    close(sockfd);
    
    printf("Received %d bytes of data\n", total_read);
    return total_read;
}

// Парсим JSON ответ от OpenWeatherMap
int parse_weather_json(const char* json, WeatherApp* app) {
    // парсинг JSON 
    printf("Parsing JSON response...\n");
    
    // Ищем температуру
    char* temp_ptr = strstr(json, "\"temp\":");
    if (temp_ptr) {
        temp_ptr += 7; // Пропускаем "\"temp\":"
        char* end_ptr = strchr(temp_ptr, ',');
        if (end_ptr) {
            int len = end_ptr - temp_ptr;
            if (len < sizeof(app->temperature) - 1) {
                strncpy(app->temperature, temp_ptr, len);
                app->temperature[len] = '\0';
                // Конвертируем из Кельвина в Цельсий
                float temp_k = atof(app->temperature);
                float temp_c = temp_k - 273.15;
                snprintf(app->temperature, sizeof(app->temperature), "%.1f", temp_c);
            }
        }
    }
    
    // Ищем ощущаемую температуру
    char* feels_ptr = strstr(json, "\"feels_like\":");
    if (feels_ptr) {
        feels_ptr += 13; 
        char* end_ptr = strchr(feels_ptr, ',');
        if (end_ptr) {
            int len = end_ptr - feels_ptr;
            if (len < sizeof(app->feels_like) - 1) {
                strncpy(app->feels_like, feels_ptr, len);
                app->feels_like[len] = '\0';
                // Конвертируем из Кельвина в Цельсий
                float feels_k = atof(app->feels_like);
                float feels_c = feels_k - 273.15;
                snprintf(app->feels_like, sizeof(app->feels_like), "%.1f", feels_c);
            }
        }
    }
    
    // Ищем влажность
    char* humidity_ptr = strstr(json, "\"humidity\":");
    if (humidity_ptr) {
        humidity_ptr += 11; 
        char* end_ptr = strchr(humidity_ptr, ',');
        if (end_ptr) {
            int len = end_ptr - humidity_ptr;
            if (len < sizeof(app->humidity) - 1) {
                strncpy(app->humidity, humidity_ptr, len);
                app->humidity[len] = '\0';
            }
        }
    }
    
    // Ищем описание погоды
    char* desc_ptr = strstr(json, "\"description\":\"");
    if (desc_ptr) {
        desc_ptr += 15; 
        char* end_ptr = strchr(desc_ptr, '"');
        if (end_ptr) {
            int len = end_ptr - desc_ptr;
            if (len < sizeof(app->description) - 1) {
                strncpy(app->description, desc_ptr, len);
                app->description[len] = '\0';
            }
        }
    }
    
    // Ищем название города
    char* city_ptr = strstr(json, "\"name\":\"");
    if (city_ptr) {
        city_ptr += 8; // Пропускаем "\"name\":\""
        char* end_ptr = strchr(city_ptr, '"');
        if (end_ptr) {
            int len = end_ptr - city_ptr;
            if (len < sizeof(app->city) - 1) {
                strncpy(app->city, city_ptr, len);
                app->city[len] = '\0';
            }
        }
    }
    
    return 1;
}

// Получение данных о погоде
int get_weather(WeatherApp* app, const char* api_key, const char* city) {
    printf("Getting weather for %s from OpenWeatherMap...\n", city);
    
    char response[BUFFER_SIZE];
    char url[512];
    
    // Формируем URL для OpenWeatherMap API
    snprintf(url, sizeof(url), 
             "/data/2.5/weather?q=%s&appid=%s",
             city, api_key);
    
    printf("API URL: %s\n", url);
    
    int result = http_get("api.openweathermap.org", url, response, BUFFER_SIZE);
    
    if (result > 0) {
        printf("Successfully received weather data\n");
        
        if (strstr(response, "\"cod\":200")) {
            // Успешный ответ
            return parse_weather_json(response, app);
        } else {
            // Ошибка API
            char* message_ptr = strstr(response, "\"message\":\"");
            if (message_ptr) {
                message_ptr += 11;
                char* end_ptr = strchr(message_ptr, '"');
                if (end_ptr) {
                    int len = end_ptr - message_ptr;
                    snprintf(app->error, sizeof(app->error), "API Error: %.*s", len, message_ptr);
                }
            } else {
                strcpy(app->error, "API returned error");
            }
            return 0;
        }
    } else {
        // Сетевая ошибка - используем фиктивные данные
        printf("Network error, using mock data\n");
        return get_weather_mock(app, city);
    }
}

// Фиктивные данные для демонстрации (если сеть не работает)
int get_weather_mock(WeatherApp* app, const char* city) {
    const char* descriptions[] = {
        "clear sky", "few clouds", "scattered clouds", "broken clouds",
        "shower rain", "rain", "thunderstorm", "snow", "mist"
    };
    
    // Генерируем "случайные" данные на основе названия города
    int seed = 0;
    for (int i = 0; city[i] != '\0'; i++) {
        seed += city[i];
    }
    
    int desc_index = seed % 9;
    int temp_base = 10 + (seed % 20); // 10-30°C
    int humidity_base = 40 + (seed % 50); // 40-90%
    
    float temp_variation = (seed % 10) - 5; // -5 to +5 variation
    float temperature = temp_base + temp_variation;
    float feels_like = temperature - (seed % 3); // feels 0-2 degrees colder
    
    snprintf(app->temperature, sizeof(app->temperature), "%.1f", temperature);
    snprintf(app->feels_like, sizeof(app->feels_like), "%.1f", feels_like);
    snprintf(app->humidity, sizeof(app->humidity), "%d", humidity_base);
    strcpy(app->description, descriptions[desc_index]);
    strcpy(app->city, city);
    strcpy(app->error, "Using mock data (no network)");
    
    printf("Mock weather for %s: %s°C, %s\n", city, app->temperature, app->description);
    return 1;
}

// Отрисовка интерфейса
void draw_weather(WeatherApp* app) {

    XFontStruct* current_font = XQueryFont(app->display,XGContextFromGC(app->gc));
    if (current_font) {
        printf("Current font: ascent=%d, descent=%d\n", current_font->ascent, current_font->descent);
    }
    XClearWindow(app->display, app->window);
    
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    
    // Заголовок
    XDrawString(app->display, app->window, app->gc, 300, 30, "Weather App", 11);
    
    // Разделительная линия
    XDrawLine(app->display, app->window, app->gc, 20, 45, 580, 45);

    // поле для ввода города
    XDrawString(app->display, app->window, app->gc, 50, 70, "Enter city:", 11);

    if (app->input_active){
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XDrawRectangle(app->display, app->window, app->gc, 150, 55, 200, 25);
    }else{
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XDrawRectangle(app->display, app->window, app->gc, 150, 55, 200, 25);
    }
    
    if (strlen(app->input_city) > 0){
        XDrawString(app->display, app->window, app->gc, 155, 72, app->input_city, strlen(app->input_city));

    }

    if (app->input_active){
        XFontStruct* font = XLoadQueryFont(app->display, "variable"); 
        if (font){
            int text_width = XTextWidth(font, app->input_city, strlen(app->input_city));
            XDrawLine(app->display, app->window, app->gc, 155 + text_width, 60, 155 + text_width, 75);
        }
    }

    XDrawRectangle(app->display, app->window, app->gc, 360, 55, 30, 25);
    XDrawString(app->display, app->window, app->gc, 365, 72, "OK", 2);

    // Город
    XDrawString(app->display, app->window, app->gc, 50, 110, "Current City:", 13);
    XDrawString(app->display, app->window, app->gc, 170, 110, app->city, strlen(app->city));
    
    // Температура
    XDrawString(app->display, app->window, app->gc, 50, 140, "Temperature:", 12);
    if (strlen(app->temperature) > 0) {
        char temp_str[50];
        snprintf(temp_str, sizeof(temp_str), "%s °C", app->temperature);
        XDrawString(app->display, app->window, app->gc, 170, 140, temp_str, strlen(temp_str));
    }
    
    // Ощущаемая температура
    XDrawString(app->display, app->window, app->gc, 50, 165, "Feels like:", 11);
    if (strlen(app->feels_like) > 0) {
        char feels_str[50];
        snprintf(feels_str, sizeof(feels_str), "%s °C", app->feels_like);
        XDrawString(app->display, app->window, app->gc, 170, 165, feels_str, strlen(feels_str));
    }
    
    // Влажность 
    XDrawString(app->display, app->window, app->gc, 50, 190, "Humidity:", 9);
    if (strlen(app->humidity) > 0) {
        char humidity_str[50];
        snprintf(humidity_str, sizeof(humidity_str), "%s %%", app->humidity);
        XDrawString(app->display, app->window, app->gc, 170, 190, humidity_str, strlen(humidity_str)); 
    }
    
    // Описание
    XDrawString(app->display, app->window, app->gc, 50, 215, "Condition:", 10);
    XDrawString(app->display, app->window, app->gc, 170, 215, app->description, strlen(app->description));
    
    // Кнопка обновления
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawRectangle(app->display, app->window, app->gc, 150, 250, 100, 30);
    XDrawString(app->display, app->window, app->gc, 170, 270, "Refresh", 7);
    
    // Кнопка выхода
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    XDrawRectangle(app->display, app->window, app->gc, 270, 250, 100, 30);
    XDrawString(app->display, app->window, app->gc, 290, 270, "Exit", 4);
    
    // Сообщение об ошибке 
    if (strlen(app->error) > 0) {
        XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
        XDrawString(app->display, app->window, app->gc, 50, 310, "Note:", 5);
        XDrawString(app->display, app->window, app->gc, 100, 310, app->error, strlen(app->error)); // ИСПРАВЛЕНО: 310
    }
    
    // Инструкция 
    XDrawString(app->display, app->window, app->gc, 50, 350, "Click city field to type | Enter to apply", 38);
    XDrawString(app->display, app->window, app->gc, 50, 370, "Press Q to quit", 15);
    if (app->cloud_pixmap != None) {
    int y = WINDOW_HEIGHT - app->cloud_h - 10;
    int x = (WINDOW_WIDTH - app->cloud_w) / 2;  // центрируем по горизонтали
    XCopyArea(app->display, app->cloud_pixmap, app->window, app->gc,
                0, 0, app->cloud_w, app->cloud_h,
                x, y);}
}

// Обработка нажатий клавиш
void handle_key_press(WeatherApp* app, XKeyEvent event, const char** current_city) {
    if (!app->input_active) return;
    
    char buffer[10];
    KeySym keysym;
    int count = XLookupString(&event, buffer, sizeof(buffer) - 1, &keysym, NULL);
    
    if (count > 0) {
        buffer[count] = '\0';
        
        if (keysym == XK_BackSpace) {
            // Удаление последнего символа
            if (strlen(app->input_city) > 0) {
                app->input_city[strlen(app->input_city) - 1] = '\0';
            }
        }
        else if (keysym == XK_Return || keysym == XK_KP_Enter) {
            // Применение введенного города
            if (strlen(app->input_city) > 0) {
                strcpy(app->city, app->input_city);
                *current_city = app->city;
                app->input_city[0] = '\0';
                app->input_active = 0;
                
                printf("City changed to: %s\n", app->city);
                
                // Получаем погоду для нового города
                if (!get_weather(app, OPENWEATHER_API_KEY, app->city)) {
                    get_weather_mock(app, app->city);
                }
            }
        }
        else if (keysym == XK_Escape) {
            // Отмена ввода
            app->input_city[0] = '\0';
            app->input_active = 0;
        }
        else if (buffer[0] >= 32 && buffer[0] <= 126) {
            // Добавление символа
            if (strlen(app->input_city) < MAX_CITY_LENGTH - 1) {
                strncat(app->input_city, buffer, 1);
            }
        }
        
        draw_weather(app);
    }
}

// Инициализация приложения
void initialize_app(WeatherApp* app) {
    memset(app, 0, sizeof(WeatherApp));
    
    app->display = XOpenDisplay(NULL);
    if (!app->display) {
        fprintf(stderr, "Cannot open X display\n");
        exit(1);
    }
    
    app->main_font = load_large_font(app->display);
    if (!app->main_font) {
        fprintf(stderr, "Failed to load any font!\n");
        exit(1);
    }

    app->screen = DefaultScreen(app->display);
    app->window = XCreateSimpleWindow(app->display, 
                                     RootWindow(app->display, app->screen),
                                     100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 1,
                                     BlackPixel(app->display, app->screen),
                                     WhitePixel(app->display, app->screen));
    
    XStoreName(app->display, app->window, "MINIX3 Weather");
    XSelectInput(app->display, app->window, 
                ExposureMask | KeyPressMask | ButtonPressMask);
    
    app->gc = XCreateGC(app->display, app->window, 0, NULL);
    XSetForeground(app->display, app->gc, BlackPixel(app->display, app->screen));
    
    XSetFont(app->display, app->gc, app->main_font->fid);
    XMapWindow(app->display, app->window);

    XpmAttributes xa;
    memset(&xa, 0, sizeof(xa));

    xa.valuemask = XpmSize | XpmDepth;
    xa.depth = DefaultDepth(app->display, app->screen);

    int xpm_result = XpmCreatePixmapFromData(
    app->display,
    app->window,
    (char **)cloud_xpm,
    &app->cloud_pixmap,
    &app->cloud_mask,
    &xa);

    if (xpm_result == XpmSuccess) {
        app->cloud_w = xa.width;
        app->cloud_h = xa.height;
        printf("Cloud XPM loaded: %dx%d\n", xa.width, xa.height);
    } else {
        printf("Failed to load cloud XPM (code %d)\n", xpm_result);
        app->cloud_pixmap = None;}
    
}

// Очистка ресурсов
void cleanup_app(WeatherApp* app) {
    if (app->display) {
        if (app->main_font){
            XFreeFont(app->display, app->main_font);
        }
        XFreeGC(app->display, app->gc);
        XDestroyWindow(app->display, app->window);
        XCloseDisplay(app->display);
    }
    if (app->cloud_pixmap != None) {
        XFreePixmap(app->display, app->cloud_pixmap);
    }

}

int main(int argc, char* argv[]) {
    WeatherApp app;
    const char* current_city = DEFAULT_CITY;
    
    // Инициализация
    memset(&app, 0, sizeof(WeatherApp));
    strcpy(app.city, DEFAULT_CITY);
    app.input_active = 0;
    
    if (argc > 1) {
        current_city = argv[1];
        strcpy(app.city, current_city);
    }
    
    printf("Starting Weather App for city: %s\n", current_city);
    printf("Using OpenWeatherMap API\n");
    
    initialize_app(&app);
    
    // Первоначальная загрузка погоды
    if (!get_weather(&app, OPENWEATHER_API_KEY, current_city)) {
        get_weather_mock(&app, current_city);
    }
    
    // Главный цикл
    XEvent event;
    int running = 1;

    while (running) {
        XNextEvent(app.display, &event);
        
        switch (event.type) {
            case Expose:
                draw_weather(&app);
                break;
                
            case ButtonPress:
                // Проверяем клик по полю ввода города
                if (event.xbutton.x >= 150 && event.xbutton.x <= 350 &&
                    event.xbutton.y >= 55 && event.xbutton.y <= 80) {
                    
                    printf("City input field clicked\n");
                    app.input_active = 1;
                    draw_weather(&app);
                }
                // Проверяем клик по кнопке OK
                else if (event.xbutton.x >= 360 && event.xbutton.x <= 390 &&
                         event.xbutton.y >= 55 && event.xbutton.y <= 80) {
                    
                    printf("OK button clicked\n");
                    if (strlen(app.input_city) > 0) {
                        strcpy(app.city, app.input_city);
                        current_city = app.city;
                        app.input_city[0] = '\0';
                        app.input_active = 0;
                        
                        printf("City changed to: %s\n", app.city);
                        
                        if (!get_weather(&app, OPENWEATHER_API_KEY, app.city)) {
                            get_weather_mock(&app, app.city);
                        }
                        draw_weather(&app);
                    }
                }
                // Проверяем клик по кнопке Refresh
                else if (event.xbutton.x >= 150 && event.xbutton.x <= 250 &&
                         event.xbutton.y >= 250 && event.xbutton.y <= 280) {
                    
                    printf("Refresh button clicked\n");
                    
                    if (!get_weather(&app, OPENWEATHER_API_KEY, current_city)) {
                        get_weather_mock(&app, current_city);
                    }
                    draw_weather(&app);
                }
                // Проверяем клик по кнопке Exit
                else if (event.xbutton.x >= 270 && event.xbutton.x <= 370 &&
                         event.xbutton.y >= 250 && event.xbutton.y <= 280) {
                    
                    printf("Exit button clicked\n");
                    running = 0;
                }
                // Клик вне элементов - деактивируем ввод
                else {
                    app.input_active = 0;
                    draw_weather(&app);
                }
                break;
                
            case KeyPress:
                // Обработка ввода с клавиатуры 
                handle_key_press(&app, event.xkey, &current_city);
                
                // Выход по нажатию Q
                if (event.xkey.keycode == XKeysymToKeycode(app.display, XK_q) ||
                    event.xkey.keycode == XKeysymToKeycode(app.display, XK_Q)) {
                    printf("Quitting...\n");
                    running = 0;
                }
                break;
        }
    }
    
    cleanup_app(&app);
    printf("Weather App closed\n");
    return 0;
}