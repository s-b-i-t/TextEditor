CXX = g++
CXXFLAGS =  -std=c++11 -g

SRCS = ECEditorTest.cpp ECTextViewImp.cpp ECController.cpp ECModel.cpp
OBJS = $(SRCS:.cpp=.o)

ec: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o myeditor
	rm -f $(OBJS)

clean:
	rm -f $(OBJS) myeditor
