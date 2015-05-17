TARGET=Textures.xbt
XBMCTEX=/home/pbureau/Software/xbmc-14.1-Helix/tools/TexturePacker/TexturePacker

IMAGES := $(shell find . -name "*jpg")
IMAGES += $(shell find . -name "*png")
IMAGES += $(shell find . -name "*gif")

all: $(TARGET)

.PHONY: force

$(TARGET): $(XBMCTEX) $(IMAGES)
ifeq (i486-linux,arm)
	$(XBMCTEX) -use_none -input . -output $(TARGET)
else
	$(XBMCTEX) -input . -output $(TARGET)

endif
$(XBMCTEX): force
	$(MAKE) -C /home/pbureau/Software/xbmc-14.1-Helix/tools/TexturePacker

clean:
	rm -f $(TARGET)
distclean: clean

