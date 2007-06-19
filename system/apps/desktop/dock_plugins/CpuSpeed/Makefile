CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c
APPBIN = /system/extensions/dock
VPATH = ./
OBJDIR	:= objs
OBJS = CpuSpeed.o 

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/CpuSpeed.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/CpuSpeed.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=CpuSpeed.so" $(OBJS) -lsyllable -o $(OBJDIR)/CpuSpeed.so

install:  $(OBJDIR)/CpuSpeed.so
	install -s $(OBJDIR)/CpuSpeed.so $(APPBIN)/CpuSpeed.so

dist:  $(OBJDIR)/CpuSpeed.so
	install -s $(OBJDIR)/CpuSpeed.so $(IMAGE)/$(APPBIN)/CpuSpeed.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/CpuSpeed.so

-include $(OBJDIR)/*.d

