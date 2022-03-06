INC_DIR += ./include
BUILD_DIR ?= ./build

CXX := g++
CXXFLAGS := -std=c++17 -MMD -Wall -Werror -I $(INC_DIR)
ifeq ($(RELEASE), 1)
CXXFLAGS += -g -O2
else
CXXFLAGS += -ggdb -fsanitize=address,undefined
endif
LD := g++
LDFLAGS := $(CXXFLAGS)
HDF5 := /opt/anaconda3/bin/h5c++

SRCS := $(shell find ./src -name "*.cpp")
OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

$(BUILD_DIR)/%.o: %.cpp
	@echo + CXX $<
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: exchange trader

EXCHANGE_DEP := $(OBJS) $(BUILD_DIR)/exchange.o
TRADER_DEP := $(OBJS) $(BUILD_DIR)/trader.o

exchange: $(EXCHANGE_DEP)
	@echo + LD $@
	@$(LD) $(LDFLAGS) $^ -o $@

trader: $(TRADER_DEP)
	@echo + LD $@
	@$(LD) $(LDFLAGS) $^ -o $@

ubi-read-test: ubi-read-test.cpp
	$(HDF5) -o $@ $^ 

clean:
	rm -rf $(BUILD_DIR) exchange trader
