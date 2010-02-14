OBJECTS = \
	hdfmonkey.o \
	diskio.o \
	image_file.o \
	ff.o \
	clock.o \
	ccsbcs.o

hdfmonkey: ${OBJECTS}
	gcc ${OBJECTS} -o hdfmonkey

clean:
	rm -f hdfmonkey *.o
