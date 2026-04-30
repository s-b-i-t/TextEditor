CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -g

BUILD_DIR := build
BIN_DIR := bin
DIST_DIR := dist
TARGET := $(BIN_DIR)/text-editor
ARCHIVE := $(DIST_DIR)/text-editor.tar.gz

SRCS := \
	main.cpp \
	TextView.cpp \
	Controller.cpp \
	Command.cpp \
	Model.cpp

OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean dist

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

dist: $(TARGET)
	mkdir -p $(DIST_DIR)
	rm -rf $(DIST_DIR)/text-editor
	mkdir -p $(DIST_DIR)/text-editor/bin $(DIST_DIR)/text-editor/dependencies
	cp $(TARGET) $(DIST_DIR)/text-editor/bin/
	cp README.md $(DIST_DIR)/text-editor/
	cp -R dependencies/. $(DIST_DIR)/text-editor/dependencies/
	tar -czf $(ARCHIVE) -C $(DIST_DIR) text-editor
	@printf "Created %s\n" "$(ARCHIVE)"

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(DIST_DIR)

-include $(DEPS)
