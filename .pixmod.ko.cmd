cmd_/home/pi/sysprog/repo/pixmod.ko := ld -r  -EL -T ./scripts/module-common.lds -T ./arch/arm/kernel/module.lds  --build-id  -o /home/pi/sysprog/repo/pixmod.ko /home/pi/sysprog/repo/pixmod.o /home/pi/sysprog/repo/pixmod.mod.o ;  true
