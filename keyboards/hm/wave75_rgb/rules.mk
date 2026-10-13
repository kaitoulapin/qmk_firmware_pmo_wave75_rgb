# Build Options
#   change yes to no to disable
#
# Board: it should exist either in <chibios>/os/hal/boards/
#  or <this_dir>/boards
BOARD = FS026
MCU = cortex-m0
MCU_ARCH = cortex-m0
ARMV = 6
MCU_FAMILY = ES32
MCU_SERIES = FS026
MCU_PORT_NAME = ES32
MCU_STARTUP = FS026
MCU_LDSCRIPT = FS026
EEPROM_DRIVER = custom
NO_USB_STARTUP_CHECK = yes
BLUETOOTH_CUSTOM = yes

ENCODER_MAP_ENABLE = yes    # Enable ENCODER MAP
ENCODER_DRIVER = custom
DEBOUNCE_TYPE = asym_eager_defer_pk
RGB_MATRIX_CUSTOM_KB = yes

SRC += keyboards/hm/wave75_rgb/indicator_lighting.c
LDFLAGS += -Wl,--wrap=rgb_matrix_set_color

# The indicator whitelist is specific to the shipped binary archive.
WAVE75_INDICATOR_LIBRARY_CHECK := $(shell python3 scripts/check-wave75-indicator-library.py lib/rdr_lib/librdrcommon.a)
ifneq ($(WAVE75_INDICATOR_LIBRARY_CHECK),verified)
    $(error Vendor library verification failed; re-audit indicator_lighting.c)
endif
