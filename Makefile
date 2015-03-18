all: mac

remake: clean all

mac:
	g++ Main.cpp GLSL_helper.cpp MStackHelp.cpp mesh.cpp -DGL_GLEXT_PROTOTYPES -framework OpenGL -framework GLUT -o AsteroidRunner

linux:
	g++ Main.cpp GLSL_helper.cpp MStackHelp.cpp mesh.cpp -DGL_GLEXT_PROTOTYPES -lGL -lGLU -lglut -o AsteroidRunner

clean:
	rm -f *~
	rm -f AsteroidRunner
	rm -f *.o

