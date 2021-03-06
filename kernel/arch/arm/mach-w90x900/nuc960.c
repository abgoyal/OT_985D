

#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <mach/hardware.h>
#include "cpu.h"

/* define specific CPU platform device */

static struct platform_device *nuc960_dev[] __initdata = {
	&nuc900_device_kpi,
	&nuc900_device_fmi,
};

/* define specific CPU platform io map */

static struct map_desc nuc960evb_iodesc[] __initdata = {
};

/*Init NUC960 evb io*/

void __init nuc960_map_io(void)
{
	nuc900_map_io(nuc960evb_iodesc, ARRAY_SIZE(nuc960evb_iodesc));
}

/*Init NUC960 clock*/

void __init nuc960_init_clocks(void)
{
	nuc900_init_clocks();
}

/*Init NUC960 board info*/

void __init nuc960_board_init(void)
{
	nuc900_board_init(nuc960_dev, ARRAY_SIZE(nuc960_dev));
}
