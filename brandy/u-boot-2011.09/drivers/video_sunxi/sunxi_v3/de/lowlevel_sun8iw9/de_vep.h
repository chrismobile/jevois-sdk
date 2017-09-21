
#ifndef __DE_VEP_H__
#define __DE_VEP_H__

int de_fcc_init(unsigned int sel, unsigned int reg_base);
int de_fcc_update_regs(unsigned int sel);
int de_fcc_set_reg_base(unsigned int sel, unsigned int chno, void *base);
int de_fcc_csc_set(unsigned int sel, unsigned int chno, unsigned int en, unsigned int mode);

#endif
