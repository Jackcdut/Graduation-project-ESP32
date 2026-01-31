
GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/custom/*.c))

DEPPATH += --dep-path $(PRJ_DIR)/custom
VPATH += :$(PRJ_DIR)/custom

CFLAGS += "-I$(PRJ_DIR)/custom"

# Network module
GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/custom/modules/network/*.c))
DEPPATH += --dep-path $(PRJ_DIR)/custom/modules/network
VPATH += :$(PRJ_DIR)/custom/modules/network
CFLAGS += "-I$(PRJ_DIR)/custom/modules/network"

# Power supply module
GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/custom/modules/power_supply/*.c))
DEPPATH += --dep-path $(PRJ_DIR)/custom/modules/power_supply
VPATH += :$(PRJ_DIR)/custom/modules/power_supply
CFLAGS += "-I$(PRJ_DIR)/custom/modules/power_supply"

# STM32 communication module
GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/custom/modules/stm32_comm/*.c))
DEPPATH += --dep-path $(PRJ_DIR)/custom/modules/stm32_comm
VPATH += :$(PRJ_DIR)/custom/modules/stm32_comm
CFLAGS += "-I$(PRJ_DIR)/custom/modules/stm32_comm"
