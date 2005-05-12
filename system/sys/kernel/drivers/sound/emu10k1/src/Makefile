OBJSDIR := ../objs/

EMU10K1_MOD := ../emu10k1

CFLAGS += -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -mpreferred-stack-boundary=2
CFLAGS += -kernel -fno-PIC -ffreestanding

LFLAGS := -kernel

MIXER := y
DEBUG := n

EMU10K1_OBJS := $(OBJSDIR)cardwo.o $(OBJSDIR)efxmgr.o $(OBJSDIR)emuadxmg.o $(OBJSDIR)hwaccess.o $(OBJSDIR)irqmgr.o $(OBJSDIR)timer.o $(OBJSDIR)voicemgr.o $(OBJSDIR)audio.o $(OBJSDIR)main.o

ifeq ($(MIXER),y)
  EMU10K1_OBJS += $(OBJSDIR)mixer.o
endif

AC97_OBJS := $(OBJSDIR)ac97_codec.o

ifeq ($(DEBUG),y)
  CFLAGS += -DEMU10K1_DEBUG
endif

OBJS := $(EMU10K1_OBJS) $(AC97_OBJS)

all: $(EMU10K1_MOD) $(AC97_MOD)
	@echo "Done"

$(EMU10K1_MOD): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

clean:
	@rm -rf $(OBJSDIR)*
	@rm -f $(EMU10K1_MOD)
	@rm -f $(AC97_MOD)

# Rules
$(OBJSDIR)cardwo.o: cardwo.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)efxmgr.o: efxmgr.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)emuadxmg.o: emuadxmg.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)hwaccess.o: hwaccess.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)irqmgr.o: irqmgr.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)timer.o: timer.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)voicemgr.o: voicemgr.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)audio.o: audio.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)main.o: main.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)mixer.o: mixer.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(OBJSDIR)ac97_codec.o: ac97_codec.c
	$(CC) $(CFLAGS) $^ -c -o $@

