NAME ?= Parallax-Kernel

DATE := $(shell date "+%Y%m%d-%H%M")

DEVICE := RMX1921

VERSION := r2

ZIP := $(NAME)-$(DEVICE)-$(COMPILER)-$(DATE)-$(VERSION).zip

EXCLUDE := Makefile *.git* *.jar* *placeholder* *.md*

zip: $(ZIP)

$(ZIP):
	@echo "Creating ZIP: $(ZIP)"
	@zip -r9 "$@" . -x $(EXCLUDE)
	@echo "Generating SHA1..."
	@sha1sum "$@" > "$@.sha1"
	@cat "$@.sha1"
	@echo "Done."


clean:
	@rm -vf dtbo.img
	@rm -vf *.zip*
	@rm -vf zImage
	@rm -vf Image*
	@echo "Cleaned Up."