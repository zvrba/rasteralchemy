CFLAGS		= -Wall -O -I$(HOME)/COMPILE/include -I./include -D_REENTRANT
CXXFLAGS	= -Wall -O -I$(HOME)/COMPILE/include -I$(HOME)/COMPILE/include/OpenEXR -I./include -D_REENTRANT
LDFLAGS		=\
	-g\
	-L$(HOME)/COMPILE/lib -L/usr/X11R6/lib\
	-lIlmImf -lHalf -lImath -lIex -lFOX -llua -llualib\
	-lXext -lX11 -lGL -lGLU -lm -lreadline -ltermcap -lpthread

SOURCE	:= $(wildcard *.c) $(wildcard *.cc)
OBJS	:= $(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCE)))
DEPS	:= $(patsubst %.o,%.d,$(OBJS))
MISSING_DEPS := $(filter-out $(wildcard $(DEPS)),$(DEPS))
MISSING_DEPS_SOURCES := $(wildcard $(patsubst %.d,%.c,$(MISSING_DEPS)) \
                                   $(patsubst %.d,%.cc,$(MISSING_DEPS)))
CPPFLAGS += -MD

.PHONY : all deps objs clean 

all: rasteralchemy

deps: $(DEPS)
objs: $(OBJS) 
clean:
	rm -f *.o *.d

rasteralchemy: $(OBJS)
	g++ -o $@ $(OBJS) $(LDFLAGS)

ifneq ($(MISSING_DEPS),)
$(MISSING_DEPS) :
	rm -f $(patsubst %.d,%.o,$@)
endif

-include $(DEPS)

