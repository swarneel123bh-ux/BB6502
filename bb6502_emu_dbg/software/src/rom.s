; ------
; ROM — top-level file for the ROM binary
; Includes bootloader and bios, linked together by rom.cfg
; ------

.include "vars.s"
.include "../boot/bootloader.s"
.include "bios.s"
