/* */

#ifndef __ASM_ARCH_NAND_H_
#define __ASM_ARCH_NAND_H_

#ifndef __ASSEMBLY__

struct s5pc110_nand {
	unsigned int nfconf;
	unsigned int nfcont;
	unsigned int nfcmmd;
	unsigned int nfaddr;
	unsigned int nfdata;
	unsigned int nfmeccd0;
	unsigned int nfmeccd1;
	unsigned int nfseccd;
	unsigned int nfsblk;
	unsigned int nfeblk;
	unsigned int nfstat;
	unsigned int nfeccerr0;
	unsigned int nfeccerr1;
	unsigned int nfmecc0;
	unsigned int nfmecc1;
	unsigned int nfsecc;
	unsigned int nfmlcbitpt;
	unsigned char res0[0x1ffbc];
	unsigned int nfeccconf;
	unsigned char res1[0x1c];
	unsigned int nfecccont;
	unsigned char res2[0x0c];
	unsigned int nfeccstat;
	unsigned char res3[0x0c];
	unsigned int nfeccsecstat;
	unsigned char res4[0x4c];
	unsigned int nfeccprgecc0;
	unsigned int nfeccprgecc1;
	unsigned int nfeccprgecc2;
	unsigned int nfeccprgecc3;
	unsigned int nfeccprgecc4;
	unsigned int nfeccprgecc5;
	unsigned int nfeccprgecc6;
	unsigned char res5[0x14];
	unsigned int nfeccerl0;
	unsigned int nfeccerl1;
	unsigned int nfeccerl2;
	unsigned int nfeccerl3;
	unsigned int nfeccerl4;
	unsigned int nfeccerl5;
	unsigned int nfeccerl6;
	unsigned int nfeccerl7;
	unsigned char res6[0x10];
	unsigned int nfeccerp0;
	unsigned int nfeccerp1;
	unsigned int nfeccerp2;
	unsigned int nfeccerp3;
	unsigned char res7[0x10];
	unsigned int nfeccconecc0;
	unsigned int nfeccconecc1;
	unsigned int nfeccconecc2;
	unsigned int nfeccconecc3;
	unsigned int nfeccconecc4;
	unsigned int nfeccconecc5;
	unsigned int nfeccconecc6;

};

#endif	/* __ASSEMBLY__ */

#endif

