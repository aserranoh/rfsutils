bin_PROGRAMS = ledblink buttonsd
ledblink_SOURCES = ledblink.c parsenum.c parsenum.h
ledblink_LDADD = -lrfsgpio
buttonsd_SOURCES = buttonsd.c parsenum.c parsenum.h list.h list.c dictint.h \
                   dictint.c readline.h readline.c daemon.h
buttonsd_LDADD = -lrfsgpio
