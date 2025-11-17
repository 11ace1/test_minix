CC = cc
CFLAGS = -I/usr/X11R7/include -L/usr/X11R7/lib -lX11
TARGET = weather

all: $(TARGET)

$(TARGET): new_icon.c
	$(CC) -o $(TARGET) weather.c $(CFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: $(TARGET)
	./$(TARGET) "Moscow"

.PHONY: all clean run debug
