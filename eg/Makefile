OBJS := $(patsubst %.rb,%.mrb,$(wildcard *.rb))
all: $(OBJS)
clean:
	rm -f *.mrb
.SUFFIXES: .rb .mrb
.rb.mrb:
	$(MRUBY_DIR)/bin/mrbc $< -o$@
.PHONY: clean
