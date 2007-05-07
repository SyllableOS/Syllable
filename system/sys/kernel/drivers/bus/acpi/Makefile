CFLAGS   += -kernel -ffreestanding -fno-PIC -fno-strict-aliasing -c -D__ENABLE_DEBUG__ -Iinclude
AFLAGS   = -c

DISPATCHER := dispatcher/dsfield.o dispatcher/dsinit.o dispatcher/dsmethod.o dispatcher/dsmthdat.o \
			dispatcher/dsobject.o dispatcher/dsopcode.o dispatcher/dsutils.o dispatcher/dswexec.o \
			dispatcher/dswload.o dispatcher/dswscope.o dispatcher/dswstate.o

EVENTS := events/evevent.o events/evgpe.o events/evgpeblk.o events/evmisc.o events/evregion.o \
			events/evrgnini.o events/evsci.o events/evxface.o events/evxfevnt.o events/evxfregn.o

EXECUTER := executer/exconfig.o executer/exconvrt.o executer/excreate.o executer/exdump.o \
			executer/exfield.o executer/exfldio.o executer/exmisc.o executer/exmutex.o \
			executer/exnames.o executer/exoparg1.o executer/exoparg2.o executer/exoparg3.o \
			executer/exoparg6.o executer/exprep.o executer/exregion.o executer/exresnte.o \
			executer/exresolv.o executer/exresop.o executer/exstore.o executer/exstoren.o \
			executer/exstorob.o executer/exsystem.o executer/exutils.o

HARDWARE := hardware/hwacpi.o hardware/hwgpe.o hardware/hwregs.o hardware/hwsleep.o hardware/hwtimer.o

NAMESPACE := namespace/nsaccess.o namespace/nsalloc.o namespace/nsdump.o namespace/nsdumpdv.o \
			namespace/nseval.o namespace/nsinit.o namespace/nsload.o namespace/nsnames.o \
			namespace/nsobject.o namespace/nsparse.o namespace/nssearch.o namespace/nsutils.o namespace/nswalk.o \
			namespace/nsxfeval.o namespace/nsxfname.o namespace/nsxfobj.o

PARSER := parser/psargs.o parser/psloop.o parser/psopcode.o parser/psparse.o parser/psscope.o parser/pstree.o \
			parser/psutils.o parser/pswalk.o parser/psxface.o

RESOURCES := resources/rsaddr.o resources/rscalc.o resources/rscreate.o resources/rsdump.o \
			resources/rsio.o resources/rsirq.o resources/rslist.o resources/rsmemory.o \
			resources/rsmisc.o resources/rsutils.o resources/rsxface.o resources/rsinfo.o

TABLES := tables/tbfadt.o tables/tbfind.o tables/tbinstal.o tables/tbutils.o tables/tbxface.o tables/tbxfroot.o

UTILITIES := utilities/utalloc.o utilities/utcache.o utilities/utcopy.o utilities/utdebug.o utilities/utdelete.o \
			utilities/uteval.o utilities/utglobal.o utilities/utinit.o utilities/utmath.o \
			utilities/utmisc.o utilities/utmutex.o utilities/utobject.o utilities/utstate.o utilities/utxface.o utilities/utresrc.o

OBJS := acpi.o bus.o button.o ec.o fan.o memset.o osl.o power.o scan.o simple_pci.o thermal.o utils.o wakeup.o video.o $(DISPATCHER) $(EVENTS) $(EXECUTER) $(HARDWARE) $(NAMESPACE) $(PARSER) $(RESOURCES) \
				$(TABLES) $(UTILITIES)


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/acpi

objs:
	mkdir objs
	-mkdir objs/dispatcher
	-mkdir objs/events
	-mkdir objs/executer
	-mkdir objs/hardware
	-mkdir objs/namespace
	-mkdir objs/parser
	-mkdir objs/resources
	-mkdir objs/tables
	-mkdir objs/utilities

$(OBJDIR)/acpi: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/acpi
	strip --strip-all $(OBJDIR)/acpi

install: all
	cp -f $(OBJDIR)/acpi $(IMAGE)/system/drivers/dev/bus/acpi

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/acpi
