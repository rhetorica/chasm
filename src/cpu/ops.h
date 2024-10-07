
#ifndef OPS_H
#define OPS_H
#include "cpu.h"

struct opt;

#define OPTYPE dwordt __fastcall
#define OPPARAM memt

typedef OPTYPE opfunc(
    // struct opt* params
    OPPARAM p
);

struct opt {
    memt op;    opfunc* impl;   octet r0; octet r1;    octet r2; octet b;    octet attribs; octet skip; octet w; regt dummy;
};
// attribs: [6 unused][1 bit vp][1 bit ap]

extern struct opt oplist[];
//extern struct opt opspace[65536];
extern opfunc* opspace[65536];

#define GET_S ((p & 0xf0) >> 4)
#define GET_R ((p & 0xf))
#define GET_T ((p & 0xf00) >> 8)
#define GET_AS ((p & 0x38) >> 3)
#define GET_AR ((p & 0x07))
#define GET_AT ((p & 0x1c0) >> 6)

void build_opspace();
extern int oplist_last;

extern opfunc op_illegal;
extern opfunc op_null;
extern opfunc op_push;
extern opfunc op_pop;
extern opfunc op_push_a;
extern opfunc op_pop_a;
extern opfunc op_push_h;
extern opfunc op_pop_h;
extern opfunc op_rev;
extern opfunc op_load_a;
extern opfunc op_load_h;
extern opfunc op_real;
extern opfunc op_unreal;
extern opfunc op_store_a;
extern opfunc op_store_h;
extern opfunc op_cdr;
extern opfunc op_flip;
extern opfunc op_copy;
extern opfunc op_swap;
extern opfunc op_load;
extern opfunc op_set;
extern opfunc op_set_c;
extern opfunc op_setcdr;
extern opfunc op_store;
extern opfunc op_load_c;
extern opfunc op_store_c;
extern opfunc op_load_co;
extern opfunc op_store_co;
extern opfunc op_load_o;
extern opfunc op_store_o;
extern opfunc op_disable;
extern opfunc op_enable;
extern opfunc op_test;
extern opfunc op_bcopy;
extern opfunc op_bfill;
extern opfunc op_update;
extern opfunc op_jmp;
extern opfunc op_jn;
extern opfunc op_rep;
extern opfunc op_int;
extern opfunc op_syscall;
extern opfunc op_iret;
extern opfunc op_lmmt;
extern opfunc op_ret;
extern opfunc op_call;
extern opfunc op_load_ia;
extern opfunc op_store_ia;
extern opfunc op_cout;
extern opfunc op_creg;
extern opfunc op_cregx;
extern opfunc op_crega;
extern opfunc op_cregf;
extern opfunc op_cin;
extern opfunc op_cinx;
extern opfunc op_cina;
extern opfunc op_mod;
extern opfunc op_add;
extern opfunc op_lsh;
extern opfunc op_sub;
extern opfunc op_mul;
extern opfunc op_rsh;
extern opfunc op_div;
extern opfunc op_cmp;
extern opfunc op_smod;
extern opfunc op_sadd;
extern opfunc op_slsh;
extern opfunc op_ssub;
extern opfunc op_smul;
extern opfunc op_srsh;
extern opfunc op_sdiv;
extern opfunc op_scmp;
extern opfunc op_bmod;
extern opfunc op_badd;
extern opfunc op_bsub;
extern opfunc op_bmul;
extern opfunc op_bdiv;
extern opfunc op_fadd;
extern opfunc op_fpow;
extern opfunc op_fsub;
extern opfunc op_fmul;
extern opfunc op_flog;
extern opfunc op_fdiv;
extern opfunc op_fcmp;
extern opfunc op_not;
extern opfunc op_xor;
extern opfunc op_ror;
extern opfunc op_and;
extern opfunc op_or;
extern opfunc op_rol;
extern opfunc op_popcount;
extern opfunc op_cadd;
extern opfunc op_clsh;
extern opfunc op_csub;
extern opfunc op_cmul;
extern opfunc op_crsh;
extern opfunc op_cdiv;
extern opfunc op_ccmp;
extern opfunc op_ccmp_e;
extern opfunc op_strcpy;
extern opfunc op_load_bl;
extern opfunc op_load_bh;
extern opfunc op_strsize;
extern opfunc op_strrev;
extern opfunc op_store_bl;
extern opfunc op_store_bh;
extern opfunc op_strcat;
extern opfunc op_strpos;
extern opfunc op_getbyte;
extern opfunc op_setbyte;
extern opfunc op_behead;
extern opfunc op_put_bli;
extern opfunc op_put_bhi;
extern opfunc op_strcmp;

#endif
