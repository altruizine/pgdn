# Adapt this if the "android" binary cannot be found in your PATH, or
# to override where it's being looked for.
SDK = ${HOME}/src/android/android-sdk-linux_x86

export PATH := ${SDK}/tools:${PATH}

APP = Buttonmap
APPDIR = app

ASSETS = $(APPDIR)/assets/pgdn $(APPDIR)/assets/dot.pgdn

all: build

init: $(APPDIR)/local.properties

$(APPDIR)/local.properties:
	android update project --name $(APP) --path $(APPDIR)

build: $(APPDIR)/bin/$(APP)-debug.apk

$(APPDIR)/bin/$(APP)-debug.apk: $(APPDIR)/local.properties $(ASSETS)
	cd $(APPDIR); ant debug

$(APPDIR)/assets/dot.pgdn: dot.pgdn
	mkdir -p $(APPDIR)/assets
	cp $^ $@

$(APPDIR)/assets/pgdn: pgdn-done 
	mkdir -p $(APPDIR)/assets
	cp pgdn/pgdn $@

pgdn-done:
	make -C pgdn pgdn

clean:
	cd $(APPDIR); rm -rf local.properties build.xml proguard-project.txt \
	  bin gen assets
	make -C pgdn clean
