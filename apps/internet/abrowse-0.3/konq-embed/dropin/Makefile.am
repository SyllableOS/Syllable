INCLUDES = -I$(srcdir)/../kdesrc/kdecore -I$(srcdir)/../kdesrc $(QT_INCLUDES)

SUBDIRS = . kio kparts khtml atheos

noinst_LTLIBRARIES = libdropin.la

libdropin_la_SOURCES = kdebug.cpp kglobal.cpp kconfig.cpp kapp.cpp dcopobject.cpp \
			kcmdlineargs.cpp dcopclient.cpp kdialogbase.cpp kuniqueapp.cpp \
			kdialog.cpp kinstance.cpp klibloader.cpp \
			kaction.cpp kstdaction.cpp kglobalsettings.cpp \
			kiconloader.cpp kmessagebox.cpp
libdropin_la_LIBADD = $(LIB_QT)
libdropin_la_LDFLAGS = $(all_libraries)
libdropin_la_METASOURCES = AUTO


KDE_OPTIONS = qtonly
