ifeq ($(subdir),misc)
sysdep_routines += ioperm iopl atomic setfsgid setfsuid setresgid setresuid
sysdep_headers += sys/elf.h sys/perm.h sys/reg.h sys/debugreg.h
endif

ifeq ($(subdir),elf)
#sysdep-others += lddlibc4
#install-bin += lddlibc4
endif
LN=cp
ifeq ($(subdir),csu)
#CFLAGS-initfini.s += -DWEAK_GMON_START
endif
