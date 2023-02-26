COMPILER=gcc
RM=rm
SRC_EXTENSION=c
OBJ_EXTENSION=o
RUNNER=

INCDIR=.
SRCDIR=interpreter
OBJDIR=obj
BUILDDIR=.
BINDIR=.
SRCS = $(wildcard $(SRCDIR)/*.$(SRC_EXTENSION)) $(wildcard ./*.$(SRC_EXTENSION))
OBJS = $(patsubst $(SRCDIR)/%.$(SRC_EXTENSION), $(OBJDIR)/%.$(OBJ_EXTENSION), $(SRCS))
DEPENDS = $(patsubst $(SRCDIR)/%.$(SRC_EXTENSION),%.d,$(SRCS))
HEADERS = $(wildcard $(INCDIR)/*.h)
GPERFFILES = $(wildcard $(SRCDIR)/*.gperf)
METHODFILES = $(patsubst $(SRCDIR)/%.gperf, $(SRCDIR)/%.methods, $(GPERFFILES))

CFLAGS=-fsanitize=address -fsanitize=leak -I"./$(INCDIR)" -Wno-unused-parameter -Wno-ignored-attributes -fopenmp -flto -mavx2 -std=c99 -pedantic -Wall -Wextra -g
LDFLAGS=$(CFLAGS) -fPIC -lm -O3 -fopenmp

EXECNAME=main
ARGS=

run: $(METHODFILES) build
	@echo EX $(RUNNER) $(BINDIR)/$(EXECNAME) $(ARGS)
	@echo ================
	@$(RUNNER) ./$(BINDIR)/$(EXECNAME) $(ARGS)
	@echo
.PHONY: run

build: $(OBJS)
	@$(COMPILER) $(filter-out %.h,$^) -o $(BINDIR)/$(EXECNAME) $(LDFLAGS)
	@echo LD $<
.PHONY: 

symbols: $(EXECNAME)
	@nm $(EXECNAME)


clean:
	@$(RM) -f $(OBJS) $(METHODFILES)
	@echo RM $(OBJS) $(METHODFILES)
.PHONY: clean

-include $(DEPENDS)

$(SRCDIR)/%.methods: $(SRCDIR)/%.gperf
	@echo GPERF $<
	@gperf $< --output-file $@

$(OBJDIR)/%.$(OBJ_EXTENSION): $(SRCDIR)/%.$(SRC_EXTENSION)
	@$(COMPILER) $(CFLAGS) -MMD -MP -MF $(patsubst %.$(SRC_EXTENSION),%.d,$<) -c $< -o $@
	@echo CC $<

init:
	@echo Initializing Folders
	@mkdir $(INCDIR)
	@mkdir $(SRCDIR)
	@mkdir $(OBJDIR)
	@mkdir $(BUILDDIR)
	@mkdir $(BINDIR)
	@echo Initialization Completed
.PHONY: init

remove: check_remove
	@$(RM) -rf $(INCDIR)
	@$(RM) -rf $(SRCDIR)
	@$(RM) -rf $(OBJDIR)
	@$(RM) -rf $(BUILDDIR)	
	@$(RM) -rf $(BINDIR)
.PHONY: remove


check_remove:
	@echo Removing folders
	@echo -n "Are you sure? [y/N] " && read ans && [ $${ans:-N} = y ]

.PHONY: remove check_remove
