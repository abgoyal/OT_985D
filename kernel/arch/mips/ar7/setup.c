
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/pm.h>
#include <linux/time.h>

#include <asm/reboot.h>
#include <asm/mach-ar7/ar7.h>
#include <asm/mach-ar7/prom.h>

static void ar7_machine_restart(char *command)
{
	u32 *softres_reg = ioremap(AR7_REGS_RESET + AR7_RESET_SOFTWARE, 1);

	writel(1, softres_reg);
}

static void ar7_machine_halt(void)
{
	while (1)
		;
}

static void ar7_machine_power_off(void)
{
	u32 *power_reg = (u32 *)ioremap(AR7_REGS_POWER, 1);
	u32 power_state = readl(power_reg) | (3 << 30);

	writel(power_state, power_reg);
	ar7_machine_halt();
}

const char *get_system_type(void)
{
	u16 chip_id = ar7_chip_id();
	switch (chip_id) {
	case AR7_CHIP_7100:
		return "TI AR7 (TNETD7100)";
	case AR7_CHIP_7200:
		return "TI AR7 (TNETD7200)";
	case AR7_CHIP_7300:
		return "TI AR7 (TNETD7300)";
	default:
		return "TI AR7 (unknown)";
	}
}

static int __init ar7_init_console(void)
{
	return 0;
}
console_initcall(ar7_init_console);

void __init plat_mem_setup(void)
{
	unsigned long io_base;

	_machine_restart = ar7_machine_restart;
	_machine_halt = ar7_machine_halt;
	pm_power_off = ar7_machine_power_off;
	panic_timeout = 3;

	io_base = (unsigned long)ioremap(AR7_REGS_BASE, 0x10000);
	if (!io_base)
		panic("Can't remap IO base!\n");
	set_io_port_base(io_base);

	prom_meminit();

	printk(KERN_INFO "%s, ID: 0x%04x, Revision: 0x%02x\n",
			get_system_type(), ar7_chip_id(), ar7_chip_rev());
}
