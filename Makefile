# Unity Facilitator Makefile
# -------------------------------------

CC      = g++47
DEFINES = -DUNITY_FACILITATOR
CFLAGS  = -Wall -lpthread $(DEFINES)
DEBUG   = -ggdb
INCLUDE = .
PROGRAMNAME = Facilitator
PROGRAMSOURCES = Facilitator.cpp

# -------------------------------------

COMMON_INCLUDE = Common
RAKNET_INCLUDE = /usr/local/include/raknet

COMMON_SOURCES = \
$(COMMON_INCLUDE)/Log.cpp\
$(COMMON_INCLUDE)/Utility.cpp

COMMON_OBJECTS = $(COMMON_SOURCES:.cpp=.o)

all: $(COMMON_SOURCES) $(PROGRAMNAME)

$(PROGRAMNAME): $(COMMON_OBJECTS)
	$(CC) $(DEBUG) -I$(INCLUDE) -I$(COMMON_INCLUDE) -I$(RAKNET_INCLUDE) -L/usr/local/lib -lraknet $(CFLAGS) $(COMMON_OBJECTS) $(RAKNET_OBJECTS) $(PROGRAMSOURCES) -o $(PROGRAMNAME) 
	chmod +x $(PROGRAMNAME)

clean:
	rm -f $(PROGRAMNAME)
	
cleanall:
	rm -f $(PROGRAMNAME)
	rm -f $(RAKNET_OBJECTS)
	rm -f $(COMMON_OBJECTS)
	
# Compile all RakNet cpp source files
.cpp.o:
	$(CC) -c -Wall -I$(INCLUDE) -I$(COMMON_INCLUDE) $(DEFINES) $< -o $@
