HDF5 := h5c++
HDF5FLAGS += -I./include

INCLUDES := $(shell find ./include -name "*.h")
SRCS := $(shell find ./src -name "*.cpp")

all: exchange trader

-include $(OBJS:.o=.d)

EXCHANGE_DEP := $(SRCS) exchange.cpp
TRADER_DEP := $(SRCS) trader.cpp

%.cpp: $(INCLUDES)

exchange: $(EXCHANGE_DEP)
	@echo + HDF5 $@
	@$(HDF5) $(HDF5FLAGS) $^ -o $@

trader: $(TRADER_DEP)
	@echo + HDF5 $@
	@$(HDF5) $(HDF5FLAGS) $^ -o $@

ubi-read-test: ubi-read-test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ 

clean:
	rm -rf *.o exchange trader
