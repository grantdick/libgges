AR:=ar
CC:=clang
SRCDIR:=src
DEMDIR:=demo
OBJDIR:=build
INCDIR:=include
BINDIR:=dist
LIBDIR:=$(BINDIR)

INCS:=$(wildcard $(SRCDIR)/*.h)
OBJS:=$(subst $(SRCDIR)/,$(OBJDIR)/,$(patsubst %.c,%.o,$(wildcard $(SRCDIR)/*.c)))
DEMO_OBJS=$(OBJDIR)/mt19937ar.o $(OBJDIR)/parameters.o $(OBJDIR)/readline.o

CFLAGS:=-std=c99 -Wall -Wextra -pedantic -march=native -O3 -g
IFLAGS:=-I$(INCDIR)
LFLAGS:=-L$(LIBDIR) -lgges -lm

INC:=$(SRCDIR)/gges.h $(SRCDIR)/individual.h \
	$(SRCDIR)/grammar.h $(SRCDIR)/mapping.h $(SRCDIR)/derivation.h \
	$(SRCDIR)/cfggp.h $(SRCDIR)/ge.h $(SRCDIR)/sge.h

LIB:=$(LIBDIR)/libgges.a
BIN:=$(BINDIR)/ant $(BINDIR)/multiplexer $(BINDIR)/parity $(BINDIR)/regression $(BINDIR)/packing \
	$(BINDIR)/template

all: $(LIB) $(BIN)

lib: $(LIB)

$(LIBDIR)/libgges.a: $(OBJS) $(INCS)
	@echo creating library $@ from $^
	@mkdir -p $(BINDIR)
	@$(AR) -r $@ $(OBJS)
	@echo copying headers to $(INCDIR)
	@mkdir -p $(INCDIR)
	@cp $(INC) $(INCDIR)

$(BINDIR)/ant: $(DEMO_OBJS) $(OBJDIR)/antmain.o $(LIB)
	@echo linking $@ from $^
	@$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

$(BINDIR)/multiplexer: $(DEMO_OBJS) $(OBJDIR)/muxmain.o $(LIB)
	@echo linking $@ from $^
	@$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

$(BINDIR)/parity: $(DEMO_OBJS) $(OBJDIR)/parmain.o $(LIB)
	@echo linking $@ from $^
	@$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

$(BINDIR)/regression: $(DEMO_OBJS) $(OBJDIR)/data.o $(OBJDIR)/regmain.o $(LIB)
	@echo linking $@ from $^
	@$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

$(BINDIR)/packing: $(DEMO_OBJS) $(OBJDIR)/pack.o $(OBJDIR)/packmain.o $(LIB)
	@echo linking $@ from $^
	@$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

$(BINDIR)/template: $(DEMO_OBJS) $(OBJDIR)/templatemain.o $(LIB)
	@echo linking $@ from $^
	@$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

$(OBJDIR)/%.o : $(SRCDIR)/%.c $(INCS)
	@echo compiling $< into $@
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJDIR)/%.o : $(DEMDIR)/%.c $(wildcard $(DEMDIR)/*.h) $(INC)
	@echo compiling $< into $@
	@$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJDIR)

nuke: clean
	@rm -rf $(INCDIR) $(BINDIR) $(LIBDIR)

strip: all
	@echo running strip on $(BIN)
	@strip $(BIN)
