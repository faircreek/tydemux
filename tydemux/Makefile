# Generated automatically from Makefile.in by configure.
#------------------------------------------------------------------
# stuff modified by configure
#------------------------------------------------------------------

DEFS=
EXTRA_LIBS= -lm -L../libs/ -lthreadlib -lpthread 

#-------------------------------
# Uncomment the following line if Pentium MMX support does not exist,
#------------------------------

#MMX= -DNO_MMX

#-------------------
# Shouldn't have to change anything else for Linux.
# If not using Linux, then change as appropriate for your system
#-------------------

#DEFS+= $(MMX) -DMPEG2DEC -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_ISOC99_SOURCE -D_GNU_SOURCE -DDEBUGTHREAD 
DEFS+= $(MMX) -DMPEG2DEC -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_ISOC99_SOURCE -D_GNU_SOURCE


#-------------------------------------------------------------------
# to add a new file to a package :
# - add the file into the variables PACKAGE_OBJS
#
# to add a new package do the following 4 steps :
#
# 1. add these 3 variables :
# PACKAGE_DIR=$(PACKAGES_DIR)/basic
# PACKAGE_OBJ_DIR=$(PACKAGE_DIR)/obj
# PACKAGE_OBJS = \
#	$(PACKAGE_OBJ_DIR)/foo.o\
#	$(PACKAGE_OBJ_DIR)/bar.o
#
# 2. add you package to the clean target
#
# 3. add the rule :
# $(AUDIO_OBJ_DIR)/%.o : $(AUDIO_DIR)/%.c
# 	$(CC) -c $(CFLAGS) $< -o $@
#
# 4. add the variables PACKAGE_OBJS to the variable OBJS
#
#--------------------------------------------------------------------

.SUFFIXES: .c .o
CC=gcc -g
AR=ar -rsc
LD=ld -G -z text
MKDIR=mkdir -p
MV=mv

PACKAGES_DIR=./
DVM_LIB_DIR=./

INCLUDE=-I../include -I../libs/mpeg2dec/include/

#Linux
#CFLAGS= -O2 -Kpic $(INCLUDE)
CFLAGS= -Wall -W -O2 -fpic  $(DEFS) $(INCLUDE)
LIBS= $(EXTRA_LIBS) ../libs/mpeg2dec/libvo/libvo.a ../libs/mpeg2dec/libmpeg2/.libs/libmpeg2.a


LIBDIR=-L$(DVM_LIB_DIR)


#---------------------------------------------------------------
# TYDEMUX
#---------------------------------------------------------------

BASIC_DIR=$(PACKAGES_DIR)/
BASIC_LIB=$(DVM_LIB_DIR)/libtydemux.a
BASIC_OBJ_DIR=$(BASIC_DIR)/obj
BASIC_OBJS=\
	$(BASIC_OBJ_DIR)/bit.o\
	$(BASIC_OBJ_DIR)/buffer.o\
	$(BASIC_OBJ_DIR)/chunk.o\
	$(BASIC_OBJ_DIR)/chunk_check.o\
	$(BASIC_OBJ_DIR)/chunk_check_junk.o\
	$(BASIC_OBJ_DIR)/chunk_help_functions.o\
	$(BASIC_OBJ_DIR)/chunk_parse.o\
	$(BASIC_OBJ_DIR)/chunk_read.o\
	$(BASIC_OBJ_DIR)/chunk_verify.o\
	$(BASIC_OBJ_DIR)/debug.o\
	$(BASIC_OBJ_DIR)/frame_rate.o\
	$(BASIC_OBJ_DIR)/gop.o\
	$(BASIC_OBJ_DIR)/misc.o\
	$(BASIC_OBJ_DIR)/pes_holder.o\
	$(BASIC_OBJ_DIR)/pes_holder_insert.o\
	$(BASIC_OBJ_DIR)/pes_holder_video_check.o\
	$(BASIC_OBJ_DIR)/pes_holder_video_check_help.o\
	$(BASIC_OBJ_DIR)/picture.o\
	$(BASIC_OBJ_DIR)/probe.o\
	$(BASIC_OBJ_DIR)/seq_frame_correction.o\
	$(BASIC_OBJ_DIR)/seq_header_fix.o\
	$(BASIC_OBJ_DIR)/sync_drift.o\
	$(BASIC_OBJ_DIR)/time.o\
	$(BASIC_OBJ_DIR)/tmp_ref.o\
	$(BASIC_OBJ_DIR)/tmp_ref_check.o\
	$(BASIC_OBJ_DIR)/tmp_ref_fix.o\
	$(BASIC_OBJ_DIR)/tmp_ref_help.o\
	$(BASIC_OBJ_DIR)/tydemux.o\
	$(BASIC_OBJ_DIR)/tyrecord.o\
	$(BASIC_OBJ_DIR)/tystream.o\
	$(BASIC_OBJ_DIR)/tystream_init.o\
	$(BASIC_OBJ_DIR)/tystream_repair.o\
	$(BASIC_OBJ_DIR)/cutpoints.o\
	$(BASIC_OBJ_DIR)/index_edit.o\
	$(BASIC_OBJ_DIR)/vstream.o \
	$(BASIC_OBJ_DIR)/index_edit_help.o \
	$(BASIC_OBJ_DIR)/dir_index.o \
	$(BASIC_OBJ_DIR)/fsid.o \
	$(BASIC_OBJ_DIR)/tydemux_remote.o

#-----------------------------------------------------------------------------------
#
#-----------------------------------------------------------------------------------

DVM_LIBS=\
	$(BASIC_LIB)



#-----------------------------------------------------------------------------------
#
#-----------------------------------------------------------------------------------
all : clibs tydemux tyls tyget

clibs : $(DVM_LIB_DIR) $(DVM_LIBS)

tydemux : clibs
	$(CC) main.c $(CFLAGS) -o $@ $(DVM_LIBS) $(LIBS)

remote : clibs
	$(CC) remote.c $(CFLAGS) -o $@ $(DVM_LIBS) $(LIBS)

tyls : clibs
	$(CC) tyls.c $(CFLAGS) -o $@ $(DVM_LIBS) $(LIBS)

tyget : clibs
	$(CC) tyget.c $(CFLAGS) -o $@ $(DVM_LIBS) $(LIBS)



clean :
	\rm -f $(DVM_LIBS)
	\rm -rf $(BASIC_DIR)/obj
	\rm -f tydemux
	\rm -f remote

$(DVM_LIB_DIR) :
	$(MKDIR) $(DVM_LIB_DIR)

$(LIB_INSTALL_DIR) :
	$(MKDIR) $(LIB_INSTALL_DIR)

$(BIN_INSTALL_DIR) :
	$(MKDIR) $(BIN_INSTALL_DIR)

$(OBJ_DIR) :
	$(MKDIR) $(OBJ_DIR)

#-----------------------------------------------------------------------------------
# General rules
#-----------------------------------------------------------------------------------


$(BASIC_OBJ_DIR) :
	$(MKDIR) $@
$(BASIC_OBJ_DIR)/%.o : $(BASIC_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@
$(BASIC_OBJ_DIR)/%.o : $(BASIC_DIR)/tcl/%.c
	$(CC) -I$(BASIC_DIR) -c $(CFLAGS) $< -o $@
$(BASIC_LIB) : $(BASIC_OBJ_DIR) $(BASIC_OBJS)
	$(AR)  $(BASIC_LIB) $(BASIC_OBJS)


