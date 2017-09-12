
CC=g++
CPPFLAGS=-c -Wall -Wextra -Werror -Wno-error=unused-parameter -O3 -flto -std=c++14
DEPFLAGS=-M
LDFLAGS=-lboost_system -lpthread

BUILDDIR=build
DEPDIR=$(BUILDDIR)/dep
OBJDIR=$(BUILDDIR)/obj
EXEDIR=$(BUILDDIR)

EXEC=main
SOURCES=$(wildcard *.cpp)
DEPS=$(SOURCES:.cpp=.d)
OBJS=$(SOURCES:.cpp=.o)
DEPSFP=$(patsubst %, $(DEPDIR)/%, $(DEPS))
OBJSFP=$(patsubst %, $(OBJDIR)/%, $(OBJS))

MAIN_OBJ=$(OBJDIR)/main.o
OBJSFP_NOMAIN=$(filter-out $(MAIN_OBJ), $(OBJSFP))

$(shell mkdir -p $(DEPDIR) > /dev/null)
$(shell mkdir -p $(OBJDIR) > /dev/null)
$(shell mkdir -p $(EXEDIR) > /dev/null)



$(EXEDIR)/$(EXEC): $(OBJSFP)
	$(CC) $^ -o $(EXEDIR)/$(EXEC) $(LDFLAGS)

$(DEPDIR)/%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*, $(OBJDIR)/$(@F:.d=.o) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
.PRECIOUS: $(DEPDIR)/%.d

-include $(DEPSFP)
$(OBJDIR)/%.o: %.cpp $(DEPDIR)/%.d
	$(CC) $(CPPFLAGS) $< -o $@
.PRECIOUS: $(OBJDIR)/%.o

.PHONY: exe
exe: $(EXEDIR)/$(EXEC)


.PHONY: obj
obj: $(OBJSFP)

.PHONY: dep
dep: $(DEPSFP)

.PHONY: clean
clean:
	rm -rf ./$(DEPDIR)/*.d \
	rm -rf ./$(OBJDIR)/*.o \
	rm -rf ./$(EXEDIR)/$(EXEC)
