CXX = g++
CXXFLAGS =  -std=c++20 -g

SRCS = ECEditorTest.cpp ECTextViewImp.cpp ECController.cpp ECModel.cpp ECCommand.cpp
OBJS = $(SRCS:.cpp=.o)

ec: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o editor
	rm -f $(OBJS)

clean:
	rm -f $(OBJS) editor
