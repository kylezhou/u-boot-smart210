/*
 * (C) Copyright 2006 OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#include <nand.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <asm/arch/nand.h>
#include <asm/io.h>

#define S5PC110_NFCONF_ECCTYPE0(x) ((x)<<23)
#define S5PC110_NFCONF_TACLS(x)    ((x)<<12)
#define S5PC110_NFCONF_TWRPH0(x)   ((x)<<8)
#define S5PC110_NFCONF_TWRPH1(x)   ((x)<<4)
#define S5PC110_NFCONF_MLCFLASH    (1<<3)
#define S5PC110_NFCONF_PAGESIZE    (1<<2)
#define S5PC110_NFCONF_ADDRCYCLE   (1<<1)

#define S5PC110_NFCONT_LOCKTIGHT   (1<<17)
#define S5PC110_NFCONT_LOCK        (1<<16)
#define S5PC110_NFCONT_MECCLOCK    (1<<7)
#define S5PC110_NFCONT_SECCLOCK    (1<<6)
#define S5PC110_NFCONT_INITMECC    (1<<5)
#define S5PC110_NFCONT_INITSECC    (1<<4)
#define S5PC110_NFCONT_nFCE        (1<<1)
#define S5PC110_NFCONT_EN          (1<<0)

#define S5PC110_NFSTAT_ILLEGALACCESS   (1<<5)
#define S5PC110_NFSTAT_RnB_TRANSDETECT (1<<4)
#define S5PC110_NFSTAT_FLASH_RnB       (1)

#define S5PC110_NFECCCONF_MSGLEN(x) ((x)<<16)
#define S5PC110_NFECCCONF_ECCTYPE(x) (x)

#define S5PC110_NFECCCONT_ENCODE   (1<<16)
#define S5PC110_NFECCCONT_INITMECC (1<<2)
#define S5PC110_NFECCCONT_RESETECC (1<<0)

#define S5PC110_NFECCSTAT_ECCBUSY      (1<<31)
#define S5PC110_NFECCSTAT_ENCODEDONE   (1<<25)
#define S5PC110_NFECCSTAT_DECODEDONE   (1<<24)
#define S5PC110_NFECCSTAT_FREEPAGESTAT (1<<8)

static int cur_ecc_mode;

#ifdef CONFIG_NAND_SPL

/* in the early stage of NAND flash booting, printf() is not available */
#define printf(fmt, args...)

static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd_to_nand(mtd);

	for (i = 0; i < len; i++)
		buf[i] = readb(chip->IO_ADDR_R);
}
#endif

static void s5pc110_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct s5pc110_nand *nand = (struct s5pc110_nand *)samsung_get_base_nand();

	debug("hwcontrol(): 0x%02x 0x%02x\n", cmd, ctrl);

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_CLE)
			chip->IO_ADDR_W = &nand->nfcmmd;
		else if (ctrl & NAND_ALE)
			chip->IO_ADDR_W = &nand->nfaddr;
		else
			chip->IO_ADDR_W = &nand->nfdata;

		if (ctrl & NAND_NCE)
			writel(readl(&nand->nfcont) & ~S5PC110_NFCONT_nFCE,
			       &nand->nfcont);
		else
			writel(readl(&nand->nfcont) | S5PC110_NFCONT_nFCE,
			       &nand->nfcont);
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, chip->IO_ADDR_W);
	else
		chip->IO_ADDR_W = &nand->nfdata;
}

static int s5pc110_dev_ready(struct mtd_info *mtd)
{
	struct s5pc110_nand *nand = (struct s5pc110_nand *)samsung_get_base_nand();
	debug("dev_ready\n");
	return readl(&nand->nfstat) & S5PC110_NFSTAT_FLASH_RnB;
}

#ifdef CONFIG_S5PC110_NAND_HWECC
void s5pc110_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct s5pc110_nand *nand = (struct s5pc110_nand *)samsung_get_base_nand();
	u_long tmp;

	debug("s5pc110_nand_enable_hwecc(%p, %d)\n", mtd, mode);
	cur_ecc_mode = mode;

	/* disble 1-bit and 4-bit ECC */
	writel(readl(&nand->nfconf) | S5PC110_NFCONF_ECCTYPE0(3), &nand->nfconf);

	/* set mode to encode or decode */
	tmp = readl(&nand->nfecccont);
	if (mode == NAND_ECC_READ)
	{
	    tmp &= ~S5PC110_NFECCCONT_ENCODE;
	}
	else if (mode == NAND_ECC_WRITE)
	{
	    tmp |= S5PC110_NFECCCONT_ENCODE;
	}
	writel(tmp, &nand->nfecccont);

	/* clear encode/decode done status */
	writel(readl(&nand->nfeccstat) | S5PC110_NFECCSTAT_ENCODEDONE | S5PC110_NFECCSTAT_DECODEDONE, &nand->nfeccstat);

	/* clear illegal access & RnB trans detect */
	writel(readl(&nand->nfstat) | S5PC110_NFSTAT_ILLEGALACCESS | S5PC110_NFSTAT_RnB_TRANSDETECT, &nand->nfstat);

	/* Initialize main area ECC decoder/encoder in NFCONT */
	writel(readl(&nand->nfcont) | S5PC110_NFCONT_INITMECC, &nand->nfcont);
	
	/* The ECC message size of 512 and 16-bit ECC/512B */
	writel(S5PC110_NFECCCONF_MSGLEN(CONFIG_SYS_NAND_ECCSIZE-1) | S5PC110_NFECCCONF_ECCTYPE(5), &nand->nfeccconf);

	/* Initialize main area ECC decoder/encoder in NFECCCONT */
	writel(readl(&nand->nfecccont) | S5PC110_NFECCCONT_INITMECC, &nand->nfecccont);
	
	/* Unlock Main area ECC   */
	writel(readl(&nand->nfcont) & ~S5PC110_NFCONT_MECCLOCK, &nand->nfcont);
}

/* wait until encode/decode is done */
static void s5pc110_nand_wait_ecc_status(const struct s5pc110_nand *nand)
{
	u_long flag = cur_ecc_mode == NAND_ECC_READ ?
	    S5PC110_NFECCSTAT_DECODEDONE : S5PC110_NFECCSTAT_ENCODEDONE;
	while(!(readl(&nand->nfeccstat) & flag));
}

static int s5pc110_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
				      u_char *ecc_code)
{
	struct s5pc110_nand *nand = (struct s5pc110_nand *)samsung_get_base_nand();
	u32 nfeccprgecc0 = 0, nfeccprgecc1 = 0, nfeccprgecc2 = 0,
	    nfeccprgecc3 = 0, nfeccprgecc4 = 0, nfeccprgecc5 = 0,
	    nfeccprgecc6 = 0;

	/* do not need to calculate ecc in read mode */
	if(cur_ecc_mode == NAND_ECC_READ) {
		debug("skipped ecc calculation for nand read mode\n");
		return 0;
	}

	/* Lock Main area ECC */
	writel(readl(&nand->nfcont) | S5PC110_NFCONT_MECCLOCK, &nand->nfcont);

	/* wait until ecc encode/decode is done */
	s5pc110_nand_wait_ecc_status(nand);

	/* read 26 bytes of ECC parity */
	nfeccprgecc0 = readl(&nand->nfeccprgecc0);
	nfeccprgecc1 = readl(&nand->nfeccprgecc1);
	nfeccprgecc2 = readl(&nand->nfeccprgecc2);
	nfeccprgecc3 = readl(&nand->nfeccprgecc3);
	nfeccprgecc4 = readl(&nand->nfeccprgecc4);
	nfeccprgecc5 = readl(&nand->nfeccprgecc5);
	nfeccprgecc6 = readl(&nand->nfeccprgecc6);

	ecc_code[0] = nfeccprgecc0 & 0xff;
	ecc_code[1] = (nfeccprgecc0 >> 8) & 0xff;
	ecc_code[2] = (nfeccprgecc0 >> 16) & 0xff;
	ecc_code[3] = (nfeccprgecc0 >> 24) & 0xff;
	ecc_code[4] = nfeccprgecc1 & 0xff;
	ecc_code[5] = (nfeccprgecc1 >> 8) & 0xff;
	ecc_code[6] = (nfeccprgecc1 >> 16) & 0xff;
	ecc_code[7] = (nfeccprgecc1 >> 24) & 0xff;
	ecc_code[8] = nfeccprgecc2 & 0xff;
	ecc_code[9] = (nfeccprgecc2 >> 8) & 0xff;
	ecc_code[10] = (nfeccprgecc2 >> 16) & 0xff;
	ecc_code[11] = (nfeccprgecc2 >> 24) & 0xff;
	ecc_code[12] = nfeccprgecc3 & 0xff;
	ecc_code[13] = (nfeccprgecc3 >> 8) & 0xff;
	ecc_code[14] = (nfeccprgecc3 >> 16) & 0xff;
	ecc_code[15] = (nfeccprgecc3 >> 24) & 0xff;
	ecc_code[16] = nfeccprgecc4 & 0xff;
	ecc_code[17] = (nfeccprgecc4 >> 8) & 0xff;
	ecc_code[18] = (nfeccprgecc4 >> 16) & 0xff;
	ecc_code[19] = (nfeccprgecc4 >> 24) & 0xff;
	ecc_code[20] = nfeccprgecc5 & 0xff;
	ecc_code[21] = (nfeccprgecc5 >> 8) & 0xff;
	ecc_code[22] = (nfeccprgecc5 >> 16) & 0xff;
	ecc_code[23] = (nfeccprgecc5 >> 24) & 0xff;
	ecc_code[24] = nfeccprgecc6 & 0xff;
	ecc_code[25] = (nfeccprgecc6 >> 8) & 0xff;
	
	debug("s5pc110_nand_calculate_hwecc(%p,):\n"
	      "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
	      "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
	      "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
	      "0x%02x 0x%02x\n", mtd,
	      ecc_code[0], ecc_code[1], ecc_code[2], ecc_code[3],
	      ecc_code[4], ecc_code[5], ecc_code[6], ecc_code[7],
	      ecc_code[8], ecc_code[9], ecc_code[10], ecc_code[11],
	      ecc_code[12], ecc_code[13], ecc_code[14], ecc_code[15],
	      ecc_code[16], ecc_code[17], ecc_code[18], ecc_code[19],
	      ecc_code[20], ecc_code[21], ecc_code[22], ecc_code[23],
	      ecc_code[24], ecc_code[25]);

	return 0;
}

static int s5pc110_nand_correct_data(struct mtd_info *mtd, u_char *dat,
				     u_char *read_ecc, u_char *calc_ecc)
{
	u_long nfeccsecstat;
	u_long nfeccerl0, nfeccerl1, nfeccerl2, nfeccerl3, nfeccerl4, nfeccerl5, nfeccerl6, nfeccerl7;
	u_long nfeccerp0, nfeccerp1, nfeccerp2, nfeccerp3;
	u_char eccerrono;
	u_short errbyteloc[16];
	u_char errpattern[16];
	u_char i;
    	struct s5pc110_nand *nand = (struct s5pc110_nand *)samsung_get_base_nand();

	struct nand_chip *chip = mtd_to_nand(mtd);

	/* write read_ecc to nand controller */
	chip->write_buf(mtd, read_ecc, chip->ecc.bytes);

	/* wait until ecc decoding is done */
	s5pc110_nand_wait_ecc_status(nand);

	/* wait for ecc idle */
	while(readl(&nand->nfeccstat) & S5PC110_NFECCSTAT_ECCBUSY);

	nfeccsecstat = readl(&nand->nfeccsecstat);
	if(nfeccsecstat & 0x20000000) { /* undocumented bit */
		printf("s5pc110_nand_correct_data: NFECCSECSTAT = %08lx\n", nfeccsecstat);
		return 0;
	}
	eccerrono = nfeccsecstat & 0x1f;
	if(eccerrono == 0) {
		printf("s5pc110_nand_correct_data: no errors!\n");
		return 0;
	}
	if(eccerrono > 16) {
		printf("s5pc110_nand_correct_data: %d uncorrectable errors detected\n", eccerrono);
		return -1;
	}
	printf("s5pc110_nand_correct_data: correcting %d errors\n", eccerrono);

	/* get error locations */
	nfeccerl0 = readl(&nand->nfeccerl0);
	nfeccerl1 = readl(&nand->nfeccerl1);
	nfeccerl2 = readl(&nand->nfeccerl2);
	nfeccerl3 = readl(&nand->nfeccerl3);
	nfeccerl4 = readl(&nand->nfeccerl4);
	nfeccerl5 = readl(&nand->nfeccerl5);
	nfeccerl6 = readl(&nand->nfeccerl6);
	nfeccerl7 = readl(&nand->nfeccerl7);

	errbyteloc[0] = nfeccerl0 & 0x3ff;
	errbyteloc[1] = (nfeccerl0 >> 16) & 0x3ff;
	errbyteloc[2] = nfeccerl1 & 0x3ff;
	errbyteloc[3] = (nfeccerl1 >> 16) & 0x3ff;
	errbyteloc[4] = nfeccerl2 & 0x3ff;
	errbyteloc[5] = (nfeccerl2 >> 16) & 0x3ff;
	errbyteloc[6] = nfeccerl3 & 0x3ff;
	errbyteloc[7] = (nfeccerl3 >> 16) & 0x3ff;
	errbyteloc[8] = nfeccerl4 & 0x3ff;
	errbyteloc[9] = (nfeccerl4 >> 16) & 0x3ff;
	errbyteloc[10] = nfeccerl5 & 0x3ff;
	errbyteloc[11] = (nfeccerl5 >> 16) & 0x3ff;
	errbyteloc[12] = nfeccerl6 & 0x3ff;
	errbyteloc[13] = (nfeccerl6 >> 16) & 0x3ff;
	errbyteloc[14] = nfeccerl7 & 0x3ff;
	errbyteloc[15] = (nfeccerl7 >> 16) & 0x3ff;

	/* get error patterns */
	nfeccerp0 = readl(&nand->nfeccerp0);
	nfeccerp1 = readl(&nand->nfeccerp1);
	nfeccerp2 = readl(&nand->nfeccerp2);
	nfeccerp3 = readl(&nand->nfeccerp3);

	errpattern[0] = nfeccerp0 & 0xff;
	errpattern[1] = (nfeccerp0 >> 8) & 0xff;
	errpattern[2] = (nfeccerp0 >> 16) & 0xff;
	errpattern[3] = (nfeccerp0 >> 24) & 0xff;
	errpattern[4] = nfeccerp1 & 0xff;
	errpattern[5] = (nfeccerp1 >> 8) & 0xff;
	errpattern[6] = (nfeccerp1 >> 16) & 0xff;
	errpattern[7] = (nfeccerp1 >> 24) & 0xff;
	errpattern[8] = nfeccerp2 & 0xff;
	errpattern[9] = (nfeccerp2 >> 8) & 0xff;
	errpattern[10] = (nfeccerp2 >> 16) & 0xff;
	errpattern[11] = (nfeccerp2 >> 24) & 0xff;
	errpattern[12] = nfeccerp3 & 0xff;
	errpattern[13] = (nfeccerp3 >> 8) & 0xff;
	errpattern[14] = (nfeccerp3 >> 16) & 0xff;
	errpattern[15] = (nfeccerp3 >> 24) & 0xff;


	/* correct errors */
	for(i=0; i < eccerrono; i++)
		dat[errbyteloc[i]] ^= errpattern[i];

	return eccerrono;
}
#endif

static void s5pc110_nand_select_chip(struct mtd_info *mtd, int ctl)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	switch(ctl) {
	case -1:
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
		break;
	case 0:
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		break;
	}
}

static struct nand_ecclayout nand_oob_512_16bit = {
	.eccbytes = 416, /* 8192 / 512 * 26 */
	.eccpos = {
                   36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 
                   64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 
                   92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 
                   120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 
                   148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 
                   176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 
                   204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 
                   232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 
                   260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 
                   288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 
                   316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 
                   344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 
                   372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 
                   400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 
                   428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 438, 439, 440, 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 
                   456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471, 472, 473, 474, 475, 476, 477, 478, 479, 480, 481},
	/* 0 and 1 for bad block flags, 2~33 free (os use),
	   36~481 for main ecc, 484~509 for spare ecc */
	.oobfree = { { .offset = 2, .length = 32 } }
};

int board_nand_init(struct nand_chip *nand)
{
	u_int32_t cfg;
	u_int8_t tacls, twrph0, twrph1;
	struct s5pc110_nand *nand_reg = (struct s5pc110_nand *)samsung_get_base_nand();

	debug("board_nand_init()\n");

	/* initialize hardware */
#if defined(CONFIG_S5PC110_CUSTOM_NAND_TIMING)
	tacls  = CONFIG_S5PC110_TACLS;
	twrph0 = CONFIG_S5PC110_TWRPH0;
	twrph1 =  CONFIG_S5PC110_TWRPH1;
#else
	tacls = 3; /* HCLK=5ns tACLS=15ns */
	twrph0 = 5; /* tWRPH0=25ns */
	twrph1 = 3; /* tWRPH1=15ns */
#endif

	cfg = S5PC110_NFCONF_ECCTYPE0(3);
	cfg |= S5PC110_NFCONF_TACLS(tacls);
	cfg |= S5PC110_NFCONF_TWRPH0(twrph0 - 1);
	cfg |= S5PC110_NFCONF_TWRPH1(twrph1 - 1);
	cfg |= S5PC110_NFCONF_MLCFLASH;
	cfg |= S5PC110_NFCONF_ADDRCYCLE;

	writel(cfg, &nand_reg->nfconf);

	cfg = S5PC110_NFCONT_EN | S5PC110_NFCONT_nFCE;
	writel(cfg, &nand_reg->nfcont);

	/* Config GPIO */
	/* MP0_1CON */
	gpio_cfg_pin(S5PC110_GPIO_MP012, S5P_GPIO_FUNC(0x3));
	gpio_cfg_pin(S5PC110_GPIO_MP013, S5P_GPIO_FUNC(0x3));
	gpio_cfg_pin(S5PC110_GPIO_MP014, S5P_GPIO_FUNC(0x3));
	gpio_cfg_pin(S5PC110_GPIO_MP015, S5P_GPIO_FUNC(0x3));
	/* MP0_3CON */
	gpio_cfg_pin(S5PC110_GPIO_MP030, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP031, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP032, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP033, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP034, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP035, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP036, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP037, S5P_GPIO_FUNC(0x2));
	/* MP0_6CON */
	gpio_cfg_pin(S5PC110_GPIO_MP060, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP061, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP062, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP063, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP064, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP065, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP066, S5P_GPIO_FUNC(0x2));
	gpio_cfg_pin(S5PC110_GPIO_MP067, S5P_GPIO_FUNC(0x2));

	/* initialize nand_chip data structure */
	nand->IO_ADDR_R = (void *)&nand_reg->nfdata;
	nand->IO_ADDR_W = (void *)&nand_reg->nfdata;

	nand->select_chip = s5pc110_nand_select_chip;

#ifdef CONFIG_NAND_SPL
	nand->read_buf = nand_read_buf;
#endif

	/* hwcontrol must always be implemented */
	nand->cmd_ctrl = s5pc110_hwcontrol;

	nand->dev_ready = s5pc110_dev_ready;

#ifdef CONFIG_S5PC110_NAND_HWECC
	nand->ecc.hwctl = s5pc110_nand_enable_hwecc;
	nand->ecc.calculate = s5pc110_nand_calculate_ecc;
	nand->ecc.correct = s5pc110_nand_correct_data;
	nand->ecc.mode = NAND_ECC_HW_OOB_FIRST;
	nand->ecc.size = CONFIG_SYS_NAND_ECCSIZE;
	nand->ecc.bytes = CONFIG_SYS_NAND_ECCBYTES;
	nand->ecc.strength = 1;
#else
	nand->ecc.mode = NAND_ECC_SOFT;
#endif
	nand->ecc.layout = &nand_oob_512_16bit;

#ifdef CONFIG_S5PC110_NAND_BBT
	nand->bbt_options |= NAND_BBT_USE_FLASH;
#endif

	debug("end of nand_init\n");

	return 0;
}
