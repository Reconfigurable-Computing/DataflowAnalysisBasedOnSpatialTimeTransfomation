INCLUDE := -I $(shell pwd) -I /usr/include -g
main:main.o workload.o arch.o mapping.o eigenUtil.o debug.o singleLevelAnalysis.o multiLevelAnalysis.o timeline.o
	g++ main.o workload.o arch.o mapping.o eigenUtil.o debug.o singleLevelAnalysis.o multiLevelAnalysis.o timeline.o -o main ${INCLUDE} 
	rm -r ./*.o
workload.o:src/datastruct/workload.cpp
	g++ -c src/datastruct/workload.cpp ${INCLUDE}
arch.o:src/datastruct/arch.cpp 
	g++ -c src/datastruct/arch.cpp ${INCLUDE}
mapping.o:src/datastruct/mapping.cpp
	g++ -c src/datastruct/mapping.cpp  ${INCLUDE}
eigenUtil.o:src/util/eigenUtil.cpp
	g++ -c src/util/eigenUtil.cpp ${INCLUDE}
debug.o:src/util/debug.cpp
	g++ -c src/util/debug.cpp ${INCLUDE}
singleLevelAnalysis.o: src/analysis/singleLevelAnalysis.cpp
	g++ -c src/analysis/singleLevelAnalysis.cpp ${INCLUDE}
multiLevelAnalysis.o: src/analysis/multiLevelAnalysis.cpp
	g++ -c src/analysis/multiLevelAnalysis.cpp ${INCLUDE}
timeline.o:src/util/timeline.cpp
	g++ -c src/util/timeline.cpp ${INCLUDE}
main.o:main.cpp
	g++ -c main.cpp ${INCLUDE}
.PHONY:clean

clean:
	rm -r ./*.o
