# Adapt this if the "android" binary cannot be found in your PATH, or
# to override where it's being looked for.
SDK = ${HOME}/src/android/android-sdk-linux_x86

# Release user alias and keystore.  Use Makefile.local to override.
-include Makefile.local
ALIAS ?= release
KEYSTORE ?= $${HOME}/.android/$(ALIAS).keystore

export PATH := ${SDK}/tools:${PATH}

APP = Buttonmap
APPDIR = app

ASSETS = $(APPDIR)/assets/pgdn $(APPDIR)/assets/dot.pgdn

all: build

init: $(APPDIR)/local.properties

$(APPDIR)/local.properties:
	android update project --name $(APP) --path $(APPDIR)

build: $(APPDIR)/bin/$(APP)-debug.apk

build-release release: $(APPDIR)/bin/$(APP).apk

$(APPDIR)/bin/$(APP)-debug.apk: $(APPDIR)/local.properties $(ASSETS) app-deb-done

app-deb-done:
	cd $(APPDIR); ant debug

$(APPDIR)/bin/$(APP)-release-unsigned.apk: $(APPDIR)/local.properties $(ASSETS) app-rel-done

app-rel-done:
	cd $(APPDIR); ant release

$(APPDIR)/bin/$(APP)-release-signed.apk: $(APPDIR)/bin/$(APP)-release-unsigned.apk
	cp $< $@
	jarsigner -verbose -digestalg SHA1 -keystore $(KEYSTORE) $@ $(ALIAS)

$(APPDIR)/bin/$(APP).apk: $(APPDIR)/bin/$(APP)-release-signed.apk
	rm -f $@
	zipalign 4 $< $@

$(APPDIR)/assets/dot.pgdn: pgdn/dot.pgdn
	mkdir -p $(APPDIR)/assets
	cp $< $@

$(APPDIR)/assets/pgdn: pgdn-done 
	mkdir -p $(APPDIR)/assets
	cp -p pgdn/pgdn $@

pgdn-done:
	make -C pgdn pgdn


push: $(APPDIR)/bin/$(APP)-debug.apk
	adb install -r $< || adb install $<

push-release: $(APPDIR)/bin/$(APP).apk
	adb install -r $< || adb install $<

push-config:
	adb push pgdn/dot.pgdn /sdcard/.pgdn

uninstall:
	adb uninstall com.github.altruizine.Buttonmap

clean:
	cd $(APPDIR); rm -rf local.properties build.xml proguard-project.txt \
	  bin gen assets
	make -C pgdn clean
