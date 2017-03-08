/*
 * (C) Copyright 2006 OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#include <nand.h>
#include <asm/arch/cpu.h>
#include <asm/arch/nand.h>
#include <asm/io.h>

#define S5PC110_NFCONF_ECCTYPE0    (1<<23)
#define S5PC110_NFCONF_MLCFLASH    (1<<3)
#define S5PC110_NFCONF_ADDRCYCLE   (1<<1)
#define S5PC110_NFCONF_TACLS(x)    ((x)<<12)
#define S5PC110_NFCONF_TWRPH0(x)   ((x)<<8)
#define S5PC110_NFCONF_TWRPH1(x)   ((x)<<4)

#define S5PC110_NFCONT_MECCLOCK    (1<<7)
#define S5PC110_NFCONT_SECCLOCK    (1<<6)
#define S5PC110_NFCONT_INITMECC    (1<<5)
#define S5PC110_NFCONT_INITSECC    (1<<4)
#define S5PC110_NFCONT_nFCE        (1<<1)
#define S5PC110_NFCONT_EN          (1<<0)

#define S5PC110_ADDR_NCLE 0x08
#define S5PC110_ADDR_NALE 0x0C

#ifdef CONFIG_NAND_SPL

/* in the early stage of NAND flash booting, printf() is not available */
#define printf(fmt, args...)

static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd_to_nand(mtd);

	for (i = 0; i < len; i++)
		buf[i] = readb(this->IO_ADDR_R);
}
#endif

static void s5pc110_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct s5pc110_nand *nand = (struct s5pc110_nand *)samsung_get_base_nand();

	debug("hwcontrol(): 0x%02x 0x%02x\n", cmd, ctrl);

	if (ctrl & NAND_CTRL_CHANGE) {
		ulong IO_ADDR_W = (ulong)nand;

		if (ctrl & NAND_CLE)
			IO_ADDR_W |= S5PC110_ADDR_NCLE;
		if (ctrl & NAND_ALE)
			IO_ADDR_W |= S5PC110_ADDR_NALE;

		chip->IO_ADDR_W = (void *)IO_ADDR_W;

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
	return readl(&nand->nfstat) & 0x01;
}

#ifdef CONFIG_S5PC110_NAND_HWECC
void s5pc110_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct s5pc110_nand *nand = (struct s5pc110_nand *)samsung_get_base_nand();
	debug("s5pc110_nand_enable_hwecc(%p, %d)\n", mtd, mode);
	//writel(readl(&nand->nfconf) | S5PC110_NFCONF_INITECC, &nand->nfconf);
	if (mode == NAND_ECC_WRITE) {
	}
}

static int s5pc110_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
				      u_char *ecc_code)
{
	struct s5pc110_nand *nand = (struct s5pc110_nand *)samsung_get_base_nand();
	ecc_code[0] = readb(&nand->nfecc);
	ecc_code[1] = readb(&nand->nfecc + 1);
	ecc_code[2] = readb(&nand->nfecc + 2);
	debug("s5pc110_nand_calculate_hwecc(%p,): 0x%02x 0x%02x 0x%02x\n",
	      mtd , ecc_code[0], ecc_code[1], ecc_code[2]);

	return 0;
}

static int s5pc110_nand_correct_data(struct mtd_info *mtd, u_char *dat,
				     u_char *read_ecc, u_char *calc_ecc)
{
	if (read_ecc[0] == calc_ecc[0] &&
	    read_ecc[1] == calc_ecc[1] &&
	    read_ecc[2] == calc_ecc[2])
		return 0;

	printf("s5pc110_nand_correct_data: not implemented\n");
	return -EBADMSG;
}
#endif

static void s5pc110_nand_select_chip(struct mtd_info *mtd, int ctl)
{
    struct nand_chip *this = mtd_to_nand(mtd);
    switch(ctl) {
    case -1:
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
	break;
    case 0:
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
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
	/* 0 and 1 for bad block flags, 2~12 free, 36~481 for ecc */
	.oobfree = {
			{.offset = 2,
			.length = 22}
		}
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

	cfg = S5PC110_NFCONF_ECCTYPE0;
	cfg |= S5PC110_NFCONF_TACLS(tacls);
	cfg |= S5PC110_NFCONF_TWRPH0(twrph0 - 1);
	cfg |= S5PC110_NFCONF_TWRPH1(twrph1 - 1);
	cfg |= S5PC110_NFCONF_MLCFLASH;
	cfg |= S5PC110_NFCONF_ADDRCYCLE;

	writel(cfg, &nand_reg->nfconf);

	cfg = S5PC110_NFCONT_EN | S5PC110_NFCONT_nFCE;
	writel(cfg, &nand_reg->nfcont);

	/* initialize nand_chip data structure */
	nand->IO_ADDR_R = (void *)&nand_reg->nfdata;
	nand->IO_ADDR_W = (void *)&nand_reg->nfdata;

	nand->select_chip = s5pc110_nand_select_chip;

	/* read_buf and write_buf are default */
	/* read_byte and write_byte are default */
#ifdef CONFIG_NAND_SPL
	nand->read_buf = nand_read_buf;
#endif

	/* hwcontrol always must be implemented */
	nand->cmd_ctrl = s5pc110_hwcontrol;

	nand->dev_ready = s5pc110_dev_ready;

#ifdef CONFIG_S5PC110_NAND_HWECC
	nand->ecc.hwctl = s5pc110_nand_enable_hwecc;
	nand->ecc.calculate = s5pc110_nand_calculate_ecc;
	nand->ecc.correct = s5pc110_nand_correct_data;
	nand->ecc.mode = NAND_ECC_HW;
	nand->ecc.size = CONFIG_SYS_NAND_ECCSIZE;
	nand->ecc.bytes = CONFIG_SYS_NAND_ECCBYTES;
	nand->ecc.strength = 1;
	nand->ecc.layout = nand_oob_512_16bit;
	nand->ecc.read_page = s5pc110_nand_read_page_hwecc;
#else
	nand->ecc.mode = NAND_ECC_SOFT;
#endif

#ifdef CONFIG_S5PC110_NAND_BBT
	nand->bbt_options |= NAND_BBT_USE_FLASH;
#endif

	debug("end of nand_init\n");

	return 0;
}
