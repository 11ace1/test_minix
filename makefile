CC = cc
CFLAGS = -I/usr/X11R7/include -L/usr/X11R7/lib -lX11 -lXpm
TARGET = conductor

all: $(TARGET)

$(TARGET): conductor.c
	$(CC) -o $(TARGET) conductor.c $(CFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: $(TARGET)
	./$(TARGET) "Moscow"

.PHONY: all clean run debug
