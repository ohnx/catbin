CFLAGS=-Wall -Werror -Iinclude/ -luv
OUTPUT=catbin
OBJS=objs/main.o objs/log.o objs/common.o objs/read.o objs/write.o

.PHONY: default
default: $(OUTPUT)

objs/%.o: src/%.c
	@mkdir -p objs/
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUTPUT): $(OBJS)
	$(CC) $^ -o $(OUTPUT) $(CFLAGS)

.PHONY: clean
clean:
	rm -rf objs/ rm $(OUTPUT)

.PHONY: debug
debug: CFLAGS += -D__DEBUG -g -O0
debug: default
