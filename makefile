#
# To compile, type "make" or make "all"
# To remove files, type "make clean"
#
all: GradeServer GradeClient

GradeServer: GradeServer.cpp
	g++ GradeServer.cpp -lpthread -o GradeServer

GradeClient: GradeClient.cpp
	g++ GradeClient.cpp -lpthread -o GradeClient

clean:
	-rm -f GradeServer.o GradeClient.o GradeServer GradeClient