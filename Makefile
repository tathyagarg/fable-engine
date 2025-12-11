all:
	gcc src/main.c -o bin/main third_party/glad.o -Llib -lglfw3 -Iinclude -lm -ldl -lpthread -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
	bin/main
