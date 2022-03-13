HDF5 := h5c++
HDF5FLAGS += -Wall -I./include -g 

INCLUDES := $(shell find ./include -name "*.h")
SRCS := $(shell find ./src -name "*.cpp")

all: exchange trader

EXCHANGE_DEP := $(SRCS) exchange.cpp
TRADER_DEP := $(SRCS) trader.cpp
TEST_DEP := $(SRCS) test.cpp

exchange: $(EXCHANGE_DEP)
	@echo + HDF5 $@
	@$(HDF5) $(HDF5FLAGS) $^ -o $@

trader: $(TRADER_DEP)
	@echo + HDF5 $@
	@$(HDF5) $(HDF5FLAGS) $^ -o $@

test: test.cpp src/ubi-read-write.cpp
	$(HDF5) $(HDF5FLAGS) -o $@ $^

clean:
	rm -rf *.o exchange trader test /package

