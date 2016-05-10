all: libs tydemux tytranscode tyeditor typrocess tymplex

clean:
	rm -f *.o ; cd libs && make clean && cd - ; cd tydemux && make clean && cd -; cd tytranscode && make clean && cd -  ; cd tyeditor && make clean && cd - ; cd tymplex && make clean && cd - ; cd liba52 && make clean && cd - ; cd typrocess && make clean && cd -



SUBDIRS = libs liba52 tydemux tytranscode typrocess tyeditor tymplex tychopper

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

QMAKESPEC=linux-g++
export QMAKESPEC

tymplex: libs
 
tydemux: libs

tytranscode: libs liba52

typrocess: libs tydemux tymplex tytranscode

tyeditor: libs tydemux typrocess
	cd tyeditor && qmake -o Makefile tyeditor.pro
	cd tyeditor && make tyeditor

