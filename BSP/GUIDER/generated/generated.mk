# images
include $(PRJ_DIR)/generated/images/images.mk

# GUI Guider fonts
include $(PRJ_DIR)/generated/guider_fonts/guider_fonts.mk

# GUI Guider customer fonts
include $(PRJ_DIR)/generated/guider_customer_fonts/guider_customer_fonts.mk

# Screen files
GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/generated/screens/*.c))
DEPPATH += --dep-path $(PRJ_DIR)/generated/screens
VPATH += :$(PRJ_DIR)/generated/screens
CFLAGS += "-I$(PRJ_DIR)/generated/screens"

GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/generated/*.c))

DEPPATH += --dep-path $(PRJ_DIR)/generated
VPATH += :$(PRJ_DIR)/generated

CFLAGS += "-I$(PRJ_DIR)/generated"