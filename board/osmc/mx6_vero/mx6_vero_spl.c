/*
 * Author: Tungyi Lin <tungyilin1127@gmail.com>
 *
 * Derived from EDM_CF_IMX6 code by TechNexion,Inc
 *
 * Adapted for Vero by Sam Nazarko <email@samnazarko.co.uk>
 * 
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#ifdef CONFIG_SPL
#include <spl.h>
#include <libfdt.h>
#endif
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/gpio.h>

#define CONFIG_SPL_STACK	0x0091FFB8

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_SPL_BUILD)

#define MX6QDL_SET_PAD(p, q) \
        if (is_cpu_type(MXC_CPU_MX6Q)) \
                imx_iomux_v3_setup_pad(MX6Q_##p | q);\
        else \
                imx_iomux_v3_setup_pad(MX6DL_##p | q)

#define UART_PAD_CTRL  (PAD_CTL_PUS_100K_UP |                   \
        PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |                 \
        PAD_CTL_SRE_FAST  | PAD_CTL_HYS)


static enum boot_device boot_dev;
static enum boot_device get_boot_device(void);
static u32 spl_get_imx_type(void);

static inline void setup_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;
	uint bt_mem_mmc = (soc_sbmr & 0x00001000) >> 12;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = MX6_ONE_NAND_BOOT;
		else
			boot_dev = MX6_WEIM_NOR_BOOT;
		break;
	case 0x2:
			boot_dev = MX6_SATA_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = MX6_I2C_BOOT;
		else
			boot_dev = MX6_SPI_NOR_BOOT;
		break;
	case 0x4:
	case 0x5:
		if (bt_mem_mmc)
			boot_dev = MX6_SD0_BOOT;
		else
			boot_dev = MX6_SD1_BOOT;
		break;
	case 0x6:
	case 0x7:
		boot_dev = MX6_MMC_BOOT;
		break;
	case 0x8 ... 0xf:
		boot_dev = MX6_NAND_BOOT;
		break;
	default:
		boot_dev = MX6_SD1_BOOT;
		break;
	}
}

enum boot_device get_boot_device(void) {
	return boot_dev;
}

#include "asm/arch/mx6_ddr_regs.h"

static void spl_dram_init_vero1(void);
static u32 spl_dram_init(u32 imxtype);

static void spl_vero_dram_setup_iomux(void)
{
	volatile struct mx6sdl_iomux_ddr_regs *mx6dl_ddr_iomux;
	volatile struct mx6sdl_iomux_grp_regs *mx6dl_grp_iomux;

	mx6dl_ddr_iomux = (struct mx6sdl_iomux_ddr_regs *) MX6SDL_IOM_DDR_BASE;
	mx6dl_grp_iomux = (struct mx6sdl_iomux_grp_regs *) MX6SDL_IOM_GRP_BASE;

	mx6dl_grp_iomux->grp_ddr_type = (u32)0x000c0000;
	mx6dl_grp_iomux->grp_ddrpke = (u32)0x00000000;
	mx6dl_ddr_iomux->dram_sdclk_0 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_sdclk_1 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_cas = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_ras = (u32)0x00000028;
	mx6dl_grp_iomux->grp_addds = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_reset = (u32)0x000c0028;
	mx6dl_ddr_iomux->dram_sdcke0 = (u32)0x00003000;
	mx6dl_ddr_iomux->dram_sdcke1 = (u32)0x00003000;
	mx6dl_ddr_iomux->dram_sdba2 = (u32)0x00000000;
	mx6dl_ddr_iomux->dram_sdodt0 = (u32)0x00003030;
	mx6dl_ddr_iomux->dram_sdodt1 = (u32)0x00003030;
	mx6dl_grp_iomux->grp_ctlds = (u32)0x00000028;
	mx6dl_grp_iomux->grp_ddrmode_ctl = (u32)0x00020000;
	mx6dl_ddr_iomux->dram_sdqs0 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_sdqs1 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_sdqs2 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_sdqs3 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_sdqs4 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_sdqs5 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_sdqs6 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_sdqs7 = (u32)0x00000028;
	mx6dl_grp_iomux->grp_ddrmode = (u32)0x00020000;
	mx6dl_grp_iomux->grp_b0ds = (u32)0x00000028;
	mx6dl_grp_iomux->grp_b1ds = (u32)0x00000028;
	mx6dl_grp_iomux->grp_b2ds = (u32)0x00000028;
	mx6dl_grp_iomux->grp_b3ds = (u32)0x00000028;
	mx6dl_grp_iomux->grp_b4ds = (u32)0x00000028;
	mx6dl_grp_iomux->grp_b5ds = (u32)0x00000028;
	mx6dl_grp_iomux->grp_b6ds = (u32)0x00000028;
	mx6dl_grp_iomux->grp_b7ds = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_dqm0 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_dqm1 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_dqm2 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_dqm3 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_dqm4 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_dqm5 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_dqm6 = (u32)0x00000028;
	mx6dl_ddr_iomux->dram_dqm7 = (u32)0x00000028;
}

static void spl_dram_init_vero1(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	volatile struct mmdc_p_regs *mmdc_p1;
	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	mmdc_p1 = (struct mmdc_p_regs *) MMDC_P1_BASE_ADDR;

	/* Calibrations */
	/* ZQ */
	mmdc_p0->mpzqhwctrl = (u32)0xa1390003;
	mmdc_p1->mpzqhwctrl = (u32)0xa1390003;
	/* write leveling */
	mmdc_p0->mpwldectrl0 = (u32)0x0045004D;
	mmdc_p0->mpwldectrl1 = (u32)0x003A0047;
	mmdc_p1->mpwldectrl0 = (u32)0x001F001F;
	mmdc_p1->mpwldectrl1 = (u32)0x00210035;
	/* DQS gating, read delay, write delay calibration values
	based on calibration compare of 0x00ffff00 */
	mmdc_p0->mpdgctrl0 = (u32)0x023C0224;
	mmdc_p0->mpdgctrl1 = (u32)0x02000220;
	mmdc_p1->mpdgctrl0 = (u32)0x02200220;
	mmdc_p1->mpdgctrl1 = (u32)0x02040208;
	mmdc_p0->mprddlctl = (u32)0x44444846;
	mmdc_p1->mprddlctl = (u32)0x4042463C;
	mmdc_p0->mpwrdlctl = (u32)0x32343032;
	mmdc_p1->mpwrdlctl = (u32)0x36363430;
	/* read data bit delay */
	mmdc_p0->mprddqby0dl = (u32)0x33333333;
	mmdc_p0->mprddqby1dl = (u32)0x33333333;
	mmdc_p0->mprddqby2dl = (u32)0x33333333;
	mmdc_p0->mprddqby3dl = (u32)0x33333333;
	mmdc_p1->mprddqby0dl = (u32)0x33333333;
	mmdc_p1->mprddqby1dl = (u32)0x33333333;
	mmdc_p1->mprddqby2dl = (u32)0x33333333;
	mmdc_p1->mprddqby3dl = (u32)0x33333333;
	/* Complete calibration by forced measurment */
	mmdc_p0->mpmur0 = (u32)0x00000800;
	mmdc_p1->mpmur0 = (u32)0x00000800;
	/* MMDC init:
	 in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc = (u32)0x0002002d;
	mmdc_p0->mdotc = (u32)0x00333040;

	mmdc_p0->mdcfg0 = (u32)0x3F4352F3;
	mmdc_p0->mdcfg1 = (u32)0xB66D8B63;

	mmdc_p0->mdcfg2 = (u32)0x01ff00db;
	mmdc_p0->mdmisc = (u32)0x00011740;
	mmdc_p0->mdscr = (u32)0x00008000;
	mmdc_p0->mdrwd = (u32)0x000026d2;
	mmdc_p0->mdor = (u32)0x00431023;
	mmdc_p0->mdasp = (u32)0x00000027;
	mmdc_p0->mdctl = (u32)0x831A0000;

	mmdc_p0->mdscr = (u32)0x02008032;
	mmdc_p0->mdscr = (u32)0x0200803a;
	mmdc_p0->mdscr = (u32)0x00008033;
	mmdc_p0->mdscr = (u32)0x0000803b;
	mmdc_p0->mdscr = (u32)0x04008031;
	mmdc_p0->mdscr = (u32)0x04008039;
	mmdc_p0->mdscr = (u32)0x05208030;
	mmdc_p0->mdscr = (u32)0x05208038;
	
	mmdc_p0->mdscr = (u32)0x04008040;
	mmdc_p0->mdscr = (u32)0x04008040;

	mmdc_p0->mdref = (u32)0x00007800;
	mmdc_p0->mpodtctrl = (u32)0x00000007;
	mmdc_p1->mpodtctrl = (u32)0x00000007;
	mmdc_p0->mdpdc = (u32)0x0002556d;
	mmdc_p1->mapsr = (u32)0x00011006;
	mmdc_p0->mdscr = (u32)0x00000000;
}

static u32 spl_dram_init(u32 imxtype)
{	
	u32 ddr_size;
	switch (imxtype){
	case MXC_CPU_MX6DL:
	default:
		spl_vero_dram_setup_iomux();
		spl_dram_init_vero1();
		ddr_size = 0x40000000;
		break;	
	}
	return ddr_size;
}

static u32 spl_get_imx_type(void)
{
	u32 cpurev;
	
	cpurev = get_cpu_rev();
	return (cpurev & 0xFF000) >> 12;
}

static void prefetch_enable(void)
{
#ifdef CONFIG_SYS_PL310_BASE
	u32 reg;

	writel(0x30000003, CONFIG_SYS_PL310_BASE + 0xf60);
	
	reg = readl(CONFIG_SYS_PL310_BASE + 0x104);
	reg |= (1 << 30);
	writel(reg, CONFIG_SYS_PL310_BASE + 0x104);
#endif
}

void board_init_f(ulong dummy)
{	
	u32 imx_type, ram_size;
	/* Set the stack pointer. */
	asm volatile("mov sp, %0\n" : : "r"(CONFIG_SPL_STACK));

	imx_type = spl_get_imx_type();	
	ram_size = spl_dram_init(imx_type);	
	
	arch_cpu_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* Set global data pointer. */
	gd = &gdata;
	gd->ram_size = ram_size;

	board_early_init_f();	

	timer_init();
	preloader_console_init();
	prefetch_enable();

	board_init_r(NULL, 0);
}

void spl_board_init(void)
{
	setup_boot_device();
}

static const char *get_vero_rev(void)
{
	switch (spl_get_imx_type())
	{
		case MXC_CPU_MX6DL:
		return "imx6dl-vero.dtb";
		break;
		default: /* Broken boot */
		return "";
		break;
	}
}

#ifdef CONFIG_SPL_OS_BOOT

#define MX6_REC_BOOT	(0 << 8)
#define MX6_FAST_BOOT	(1 << 8)
#define MX6_DEV_BOOT	(2 << 8)
#define SNVS_LPGPR	0x68

int spl_start_uboot(void)
{
	u32 reg = readl(SNVS_BASE_ADDR + SNVS_LPGPR);

	if (!!(reg & MX6_FAST_BOOT))
	{
		spl_image.args = get_vero_rev(); 
		writel(MX6_REC_BOOT, SNVS_BASE_ADDR + SNVS_LPGPR);
		return 0;
	}
	else
	{
			
	}
}

/*
 * Get cells len in bytes
 *     if #NNNN-cells property is 2 then len is 8
 *     otherwise len is 4
 */
static int get_cells_len(void *blob, char *nr_cells_name)
{
        const fdt32_t *cell;

        cell = fdt_getprop(blob, 0, nr_cells_name, NULL);
        if (cell && fdt32_to_cpu(*cell) == 2)
                return 8;

        return 4;
}

/*
 * Write a 4 or 8 byte big endian cell
 */
static void write_cell(u8 *addr, u32 val, int size)
{
        int shift = (size - 1) * 8;
        while (size-- > 0) {
                *addr++ = (val >> shift) & 0xff;
                shift -= 8;
        }
}

static int spl_fdt_fixup_memory_node(void *blob, u32 start, u32 size)
{
        int err, nodeoffset;
        int addr_cell_len, size_cell_len, len;
        u8 tmp[8]; /* Up to 32-bit address + 32-bit size */
        int bank;

	printf("setup memory node at 0x%x, size: 0x%x\n", start, size);
        err = fdt_check_header(blob);
        if (err < 0) {
                printf("%s: %s\n", __FUNCTION__, fdt_strerror(err));
                return err;
        }

        /* update, or add and update /memory node */
        nodeoffset = fdt_path_offset(blob, "/memory");
        if (nodeoffset < 0) {
                nodeoffset = fdt_add_subnode(blob, 0, "memory");
                if (nodeoffset < 0)
                        printf("WARNING: could not create /memory: %s.\n",
                                        fdt_strerror(nodeoffset));
                return nodeoffset;
        }
        err = fdt_setprop(blob, nodeoffset, "device_type", "memory",
                        sizeof("memory"));
        if (err < 0) {
                printf("WARNING: could not set %s %s.\n", "device_type",
                                fdt_strerror(err));
                return err;
        }

        addr_cell_len = get_cells_len(blob, "#address-cells");
        size_cell_len = get_cells_len(blob, "#size-cells");

	write_cell(tmp + len, start, addr_cell_len);
	len += addr_cell_len;

	write_cell(tmp + len, size, size_cell_len);
	len += size_cell_len;

        err = fdt_setprop(blob, nodeoffset, "reg", tmp, len);
        if (err < 0) {
                printf("WARNING: could not set %s %s.\n",
                                "reg", fdt_strerror(err));
                return err;
        }
        return 0;
}

void spl_board_prepare_for_linux(void)
{
	int err;
	if (spl_image.have_fdt) {
		err = spl_fdt_fixup_memory_node((void*)spl_image.args_addr,
						CONFIG_SYS_SDRAM_BASE, gd->ram_size);
		if (err < 0)
			printf("Failed to fixup memory node\n");
	}

	writel(MX6_REC_BOOT, SNVS_BASE_ADDR + SNVS_LPGPR);
}

#endif

u32 spl_boot_device(void)
{
	puts("Boot Device: ");
	switch (get_boot_device()) {
	case MX6_SD0_BOOT:
		printf("SD0\n");
		return BOOT_DEVICE_MMC1;
	case MX6_SD1_BOOT:
		printf("SD1\n");
		return BOOT_DEVICE_MMC2;
	case MX6_MMC_BOOT:
		printf("MMC\n");
		return BOOT_DEVICE_MMC2;
	case MX6_NAND_BOOT:
		printf("NAND\n");
		return BOOT_DEVICE_NAND;
	case MX6_SATA_BOOT:
		printf("SATA\n");
		return BOOT_DEVICE_SATA;
	case MX6_UNKNOWN_BOOT:
	default:
		printf("UNKNOWN..\n");
		return BOOT_DEVICE_MMC1;
	}
}

u32 spl_boot_mode(void)
{
	switch (spl_boot_device()) {
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		return MMCSD_MODE;
		break;
	case BOOT_DEVICE_SATA:
		return SATA_MODE;
		break;
	//case BOOT_DEVICE_NAND:
	//	return 0;
	//	break;
	default:
		puts("spl: ERROR:  unsupported device\n");
		hang();
	}
}

void reset_cpu(ulong addr)
{
	__REG16(WDOG1_BASE_ADDR) = 4;
}
#endif

