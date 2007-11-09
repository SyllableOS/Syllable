CC   = gcc
LL   = gcc

OBJS = AttribEdit.o etextview.o
EXE  = AttribEdit
COPTS = -c -Wall -O2

all: $(OBJS)
	$(LL) $(OBJS) -lsyllable -lstdc++ -o $(EXE)
	rescopy -r $(EXE) ./icons/*
	strip --strip-all $(EXE)

install: all
	cp -f $(EXE) /bin/$(EXE)

clean:
	rm -f $(OBJS)
	rm -f $(EXE)

%.o: %.cpp
	$(CC) $(COPTS) $< -o $@

AttribEdit.o: AttribEdit.cpp
etextview.o: etextview.cpp
