# GNU make手册：http://www.gnu.org/software/make/manual/make.html
# ************ 遇到不明白的地方请google以及阅读手册 *************

# 编译器设定和编译选项
CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99 -g3
L_FLAG = 
B_FLAG = -d -v 
DEBUG :=
SANITIZE := 

ifeq ($(DEBUG), YES)
    CFLAGS += -DD_MOD
	# L_FLAG += -d
	B_FLAG += -t
endif

ifeq ($(SANITIZE), YES)
	LEAK_CK := -fsanitize=address
endif

# 编译目标：src目录下的所有.c文件
CFILES = $(shell find ./ -name "*.c")
OBJS = $(CFILES:.c=.o)
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")
LFC = $(shell find ./ -name "*.l" | sed s/[^/]*\\.l/lex.yy.c/)
YFC = $(shell find ./ -name "*.y" | sed s/[^/]*\\.y/syntax.tab.c/)
LFO = $(LFC:.c=.o)
YFO = $(YFC:.c=.o)

parser: syntax $(filter-out $(LFO),$(OBJS))
	$(CC) -o parser $(filter-out $(LFO),$(OBJS)) -lfl -ly $(LEAK_CK) -g3

syntax: lexical syntax-c
	$(CC) -g3 -c $(YFC) -o $(YFO)

lexical: $(LFILE)
	$(FLEX)  -o $(LFC) $(L_FLAG) $(LFILE) 

syntax-c: $(YFILE)
	$(BISON) -o $(YFC) $(B_FLAG) $(YFILE)

-include $(patsubst %.o, %.d, $(OBJS))

# 定义的一些伪目标
.PHONY: clean test tmp-test ir-test
T_FILES = $(shell find ./testcase/stage* -name "*.cmm")
test: $(T_FILES)
	@for file in $^; do \
        echo ">> Parsing $$(basename $$file) ----------------------------------👇"; \
        ./parser "$$file" out1.ir; \
		echo ""; \
	done
clean:
	rm -f parser lex.yy.c syntax.tab.c syntax.tab.h syntax.output
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f $(LFC) $(YFC) $(YFC:.c=.h)
	rm -f *~

tmp-test:
	@./parser testcase/tmp-testcase.cmm out1.ir
ir-test:
	@./parser testcase/tmp-testcase.cmm out1.ir

gdb:
	@gdb -x init.gdb

.PHONY: leak-test
CMM_DIR = 
CMM_FILES = $(shell find $(CMM_DIR) -name "*.cmm")
leak-test: $(CMM_FILES) 
	@for cmm in $^; do\
		echo ">> Parsing $$cmm ----------------------------------👇"; \
		./parser "$$cmm"; \
		echo ""; \
	done	