# contrib/data_rig/Makefile

MODULE_big = data_rig
OBJS= data_rig.o $(WIN32RES)

EXTENSION = data_rig
DATA = data_rig--1.0.sql
PGFILEDESC = "data_rig - multidimensional OLAP data type"

REGRESS = data_rig

SHLIB_LINK += $(filter -lm, $(LIBS))

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/data_rig
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
