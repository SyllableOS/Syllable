ifeq ($(subdir),math)
libm-sysdep_routines += libm_atan2_reg s_matherrf s_matherrl libm_reduce \
			libm_tan libm_error \
			libm_frexp4 libm_frexp4f libm_frexp4l

sysdep_routines += libm_frexp4 libm_frexp4f libm_frexp4l libc_libm_error
sysdep-CPPFLAGS += -DSIZE_INT_32
endif
