#
# 2018-2019 Daniel Shervheim
# danielshervheim@gmail.com
# github.com/danielshervheim
#

CXX = gcc
CXXFLAGS = -Wall
GUIFLAGS = `pkg-config --cflags --libs gtk+-3.0` -rdynamic

BUILD_DIR = build
SRC_DIR = src

PROJECT_NAME = tinychat

all: build_folder chat_server chat_client

build_folder:
	@mkdir -p $(BUILD_DIR)

chat_server: $(SRC_DIR)/server.c
	$(CXX) -o $(BUILD_DIR)/$(PROJECT_NAME)_server.app $(SRC_DIR)/server.c $(CXXFLAGS)

chat_client: $(SRC_DIR)/client.c
	$(CXX) -o $(BUILD_DIR)/$(PROJECT_NAME)_client.app $(SRC_DIR)/client.c $(CXXFLAGS) $(GUIFLAGS)

clean:
	@rm -rf $(BUILD_DIR)
