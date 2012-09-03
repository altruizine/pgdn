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

BUILDDEPS = $(APPDIR)/AndroidManifest.xml \
  $(APPDIR)/assets/pgdn \
  $(APPDIR)/assets/dot.pgdn

all: build

init: $(APPDIR)/local.properties

$(APPDIR)/local.properties:
	android update project --name $(APP) --path $(APPDIR)

build: $(APPDIR)/bin/$(APP)-debug.apk

build-release release: $(APPDIR)/bin/$(APP).apk

$(APPDIR)/bin/$(APP)-debug.apk: $(APPDIR)/local.properties $(BUILDDEPS) app-deb-done

app-deb-done:
	cd $(APPDIR); ant debug

$(APPDIR)/bin/$(APP)-release-unsigned.apk: $(APPDIR)/local.properties $(BUILDDEPS) app-rel-done

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

$(APPDIR)/AndroidManifest.xml: $(APPDIR)/AndroidManifest.xml.in version
	version="$$(cat version)"; \
	  versionname="$$(printf '%d.%02d.%02d' \
	                         `expr $$version / 10000` \
	                         `expr $$version % 10000 / 100` \
	                         `expr $$version % 100`)"; \
	  sed -e "s/%versioncode%/$$version/g; \
	          s/%versionname%/$$versionname/g" $< > $@.out
	mv $@.out $@

push: $(APPDIR)/bin/$(APP)-debug.apk
	adb install -r $< || adb install $<

push-release: $(APPDIR)/bin/$(APP).apk
	adb install -r $< || adb install $<

push-config pgdn.zip:
	$(MAKE) -C pgdn $@

$(APP).apk.zip: $(APPDIR)/bin/$(APP).apk
	rm -f $@
	cd $(dir $<); zip "$(abspath $@)" $(notdir $<)

dist: $(APP).apk.zip
	$(MAKE) -C pgdn dist

newversion:
	newvers="$$(expr `cat version` + 1)"; echo $$newvers > version

uninstall:
	adb uninstall com.github.altruizine.Buttonmap

clean:
	cd $(APPDIR); rm -rf local.properties build.xml proguard-project.txt \
	  bin gen assets AndroidManifest.xml AndroidManifest.xml.out
	make -C pgdn clean
	rm -f $(APP).apk.zip
