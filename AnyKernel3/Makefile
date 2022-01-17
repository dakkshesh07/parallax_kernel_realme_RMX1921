#Kernel Name
NAME := Parallax-Kernel

#Build Date
DATE := $(shell date "+%Y%m%d-%H%M")

#Device Name
DEVICE := RMX1921

#Zip Name format, $RELEASE_BUILD, $COMPILER and $ZIP_VERSION handled by build script
ifneq ($(RELEASE_BUILD),)
ZIP := $(NAME)-$(ZIP_VERSION)-$(DEVICE)-$(DATE).zip
else
ZIP := $(NAME)-$(DEVICE)-$(COMPILER)-$(DATE)-$(ZIP_VERSION).zip
endif

#Files to be excluded while zipping
EXCLUDE := Makefile *.git* *.jar* *placeholder* *.md*

#Create build zip
zip: $(ZIP)

$(ZIP):
	@echo "Creating ZIP: $(ZIP)"
	@zip -r9 "$@" . -x $(EXCLUDE)
	@echo "Generating SHA1..."
	@sha1sum "$@" > "$@.sha1"
	@cat "$@.sha1"
	@echo "Done."

#Clean previous build leftovers
clean:
	@rm -vf dtbo.img
	@rm -vf *.zip*
	@rm -vf zImage
	@rm -vf Image*
	@echo "Cleaned Up."