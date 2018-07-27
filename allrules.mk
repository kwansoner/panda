ALL_SRC += $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
ALL_OBJ += $(patsubst %.cpp, $(OBJDIR)/%.o, $(ALL_SRC))
ALL_INCLUDE += $(patsubst %, -I%, $(INCLUDE))

# make obj directory
$(foreach dir, $(SRCDIR), $(shell mkdir -p $(OBJDIR)/$(dir)))

$(TARGET): $(ALL_OBJ)
	$(CC) $(CFLAGS) $(ALL_INCLUDE) $^ -o $@

$(OBJDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) $(ALL_INCLUDE) -c $< -o $@

.PHONY: clean
clean:
	-rm $(BINDIR)/* -rf