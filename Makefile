SRC_DIR = PugMod
CSSDK 	= $(SRC_DIR)/include/cssdk
METAMOD = $(SRC_DIR)/include/metamod

NAME = pugmod

COMPILER = g++

OBJECTS = $(SRC_DIR)/*.cpp

LINK =

OPT_FLAGS = -O3 -msse3 -flto -funroll-loops -fomit-frame-pointer -fno-stack-protector -fPIC

INCLUDE = -I$(SRC_DIR)/. -I$(CSSDK)/common -I$(CSSDK)/dlls -I$(CSSDK)/engine -I$(CSSDK)/game_shared -I$(CSSDK)/pm_shared -I$(CSSDK)/public -I$(METAMOD)

CFLAGS = $(OPT_FLAGS) -g -DNDEBUG -Dlinux -D__linux__ -D__USE_GNU -std=gnu++17 -m32 -shared

BIN_DIR = Release

OBJ_LINUX := $(OBJECTS:%.c=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.c

	$(COMPILER) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)

	$(MAKE) $(NAME) && strip -x $(BIN_DIR)/$(NAME)_mm.so

$(NAME): $(OBJ_LINUX)

	$(COMPILER) $(INCLUDE) $(CFLAGS) $(OBJ_LINUX) $(LINK) -o$(BIN_DIR)/$(NAME)_mm.so

check:
	cppcheck $(INCLUDE) --quiet --max-configs=100 --std=c++17 -DNDEBUG -Dlinux -D__linux__ -D__USE_GNU .

default: all

clean:
	rm -rf Release/*.o
	rm -rf Release/*.so
