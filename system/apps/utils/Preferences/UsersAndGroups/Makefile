CXXFLAGS += -c -I.
OBJS = main.o users_window.o user_propertiesdlg.o changepwddlg.o group_propertiesdlg.o
EXE = Users
TARGET_EXE = Users\ \&\ Groups

CATALOGS = da de el es fr hu nl pt-br ru sv

#Define TEST if you're testing the Preferences; doesn't overwrite system files
#CXXFLAGS += -DTEST

$(EXE): catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lcrypt -o $(EXE)
	strip --strip-all $(EXE)
	rescopy $(EXE) -r *.png
	rescopy -a $(EXE) resources/UsersAndGroups.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/UsersAndGroups.catalog=resources/$$CATALOG/UsersAndGroups.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c UsersAndGroups.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t UsersAndGroups.cd; \
		cd ..; \
	done;

install: $(EXE)
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(TARGET_EXE)

dist: install

clean: catalog-clean
	-rm $(OBJS)
	-rm $(EXE)

catalog-clean:
	-rm resources/UsersAndGroups.catalog resources/UsersAndGroups.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/UsersAndGroups.catalog; \
	done;

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

