TARGET	=	calc

all: $(TARGET)

$(TARGET):
	gcc -g -Wall -Wextra -o calc calculator.c -lreadline

clean:
	-rm -f $(TARGET)