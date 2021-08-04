NAME ?= Parallax-Kernel

DATE := $(shell date "+%Y%m%d-%H%M")

DEVICE := RMX1921

VERSION := v1.0

ZIP := $(NAME)-$(VERSION)-$(DEVICE)-$(DATE).zip

EXCLUDE := Makefile *.git* *.jar* *placeholder* *.md*

zip: $(ZIP)

$(ZIP):
	@echo "Creating ZIP: $(ZIP)"
	@zip -r9 "$@" . -x $(EXCLUDE)
	@echo "Generating SHA1..."
	@sha1sum "$@" | head -c 40 > "$@.sha1"
	@cat "$@.sha1"
	@echo ""
	@echo "Done."


clean:
	@rm -vf dtbo.img
	@rm -vf *.zip*
	@rm -vf zImage
	@rm -vf Image*
	@echo "Cleaned Up."