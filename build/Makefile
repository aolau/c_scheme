PROG = scheme
TEST = test_$(PROG)

PROG_SRC = main.c
PROG_OBJ = $(patsubst %.c,%.o,$(PROG_SRC))

TEST_SRC = test.c
TEST_OBJ = $(patsubst %.c,%.o,$(TEST_SRC))

SRC_DIR = ../src
INC_DIR = ../include

vpath %.c $(SRC_DIR)
vpath %.h $(INC_DIR)

CPPFLAGS = -I$(INC_DIR)
CFLAGS = -ggdb -std=c99

.PHONY: all
all: $(PROG) $(TEST)

$(PROG): $(PROG_OBJ)
	gcc -o $@ $^

$(TEST): $(TEST_OBJ)
	gcc -o $@ $^

%.o: %.c
	gcc $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.PHONY: run_tests
run_tests: $(TEST)
	@echo
	@echo Running tests...
	@echo	
	./$(TEST)

.PHONY: check
check: run_tests
	@echo
	@echo Checking for memory leaks...
	@echo
	valgrind --tool=memcheck ./$(TEST)

.PHONY: clean
clean:
	rm -f $(PROG) $(TEST) $(PROG_OBJ) $(TEST_OBJ)
