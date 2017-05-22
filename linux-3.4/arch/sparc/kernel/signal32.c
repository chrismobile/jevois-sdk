/*  arch/sparc64/kernel/signal32.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 *  Copyright (C) 1996 Miguel de Icaza (miguel@nuclecu.unam.mx)
 *  Copyright (C) 1997 Eddie C. Dost   (ecd@skynet.be)
 *  Copyright (C) 1997,1998 Jakub Jelinek   (jj@sunsite.mff.cuni.cz)
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/binfmts.h>
#include <linux/compat.h>
#include <linux/bitops.h>
#include <linux/tracehook.h>

#include <asm/uaccess.h>
#include <asm/ptrace.h>
#include <asm/pgtable.h>
#include <asm/psrcompat.h>
#include <asm/fpumacro.h>
#include <asm/visasm.h>
#include <asm/compat_signal.h>
#include <asm/switch_to.h>

#include "sigutil.h"

#define _BLOCKABLE (~(sigmask(SIGKILL) | sigmask(SIGSTOP)))

/* This magic should be in g_upper[0] for all upper parts
 * to be valid.
 */
#define SIGINFO_EXTRA_V8PLUS_MAGIC  0x130e269
typedef struct {
  unsigned int g_upper[8];
  unsigned int o_upper[8];
  unsigned int asi;
} siginfo_extra_v8plus_t;

struct signal_frame32 {
  struct sparc_stackf32 ss;
  __siginfo32_t   info;
  /* __siginfo_fpu_t * */ u32 fpu_save;
  unsigned int    insns[2];
  unsigned int    extramask[_COMPAT_NSIG_WORDS - 1];
  unsigned int    extra_size; /* Should be sizeof(siginfo_extra_v8plus_t) */
  /* Only valid if (info.si_regs.psr & (PSR_VERS|PSR_IMPL)) == PSR_V8PLUS */
  siginfo_extra_v8plus_t  v8plus;
  /* __siginfo_rwin_t * */u32 rwin_save;
} __attribute__ ( (aligned (8) ) );

typedef struct compat_siginfo {
  int si_signo;
  int si_errno;
  int si_code;
  
  union {
    int _pad[SI_PAD_SIZE32];
    
    /* kill() */
    struct {
      compat_pid_t _pid;    /* sender's pid */
      unsigned int _uid;    /* sender's uid */
    } _kill;
    
    /* POSIX.1b timers */
    struct {
      compat_timer_t _tid;      /* timer id */
      int _overrun;     /* overrun count */
      compat_sigval_t _sigval;    /* same as below */
      int _sys_private;   /* not to be passed to user */
    } _timer;
    
    /* POSIX.1b signals */
    struct {
      compat_pid_t _pid;    /* sender's pid */
      unsigned int _uid;    /* sender's uid */
      compat_sigval_t _sigval;
    } _rt;
    
    /* SIGCHLD */
    struct {
      compat_pid_t _pid;    /* which child */
      unsigned int _uid;    /* sender's uid */
      int _status;      /* exit code */
      compat_clock_t _utime;
      compat_clock_t _stime;
    } _sigchld;
    
    /* SIGILL, SIGFPE, SIGSEGV, SIGBUS, SIGEMT */
    struct {
      u32 _addr; /* faulting insn/memory ref. */
      int _trapno;
    } _sigfault;
    
    /* SIGPOLL */
    struct {
      int _band;  /* POLL_IN, POLL_OUT, POLL_MSG */
      int _fd;
    } _sigpoll;
  } _sifields;
} compat_siginfo_t;

struct rt_signal_frame32 {
  struct sparc_stackf32 ss;
  compat_siginfo_t  info;
  struct pt_regs32  regs;
  compat_sigset_t   mask;
  /* __siginfo_fpu_t * */ u32 fpu_save;
  unsigned int    insns[2];
  stack_t32   stack;
  unsigned int    extra_size; /* Should be sizeof(siginfo_extra_v8plus_t) */
  /* Only valid if (regs.psr & (PSR_VERS|PSR_IMPL)) == PSR_V8PLUS */
  siginfo_extra_v8plus_t  v8plus;
  /* __siginfo_rwin_t * */u32 rwin_save;
} __attribute__ ( (aligned (8) ) );

int copy_siginfo_to_user32 (compat_siginfo_t __user * to, siginfo_t * from)
{
  int err;
  
  if (!access_ok (VERIFY_WRITE, to, sizeof (compat_siginfo_t) ) )
  { return -EFAULT; }
  
  /* If you change siginfo_t structure, please be sure
     this code is fixed accordingly.
     It should never copy any pad contained in the structure
     to avoid security leaks, but must copy the generic
     3 ints plus the relevant union member.
     This routine must convert siginfo from 64bit to 32bit as well
     at the same time.  */
  err = __put_user (from->si_signo, &to->si_signo);
  err |= __put_user (from->si_errno, &to->si_errno);
  err |= __put_user ( (short) from->si_code, &to->si_code);
  if (from->si_code < 0)
  { err |= __copy_to_user (&to->_sifields._pad, &from->_sifields._pad, SI_PAD_SIZE); }
  else {
    switch (from->si_code >> 16) {
    case __SI_TIMER >> 16:
      err |= __put_user (from->si_tid, &to->si_tid);
      err |= __put_user (from->si_overrun, &to->si_overrun);
      err |= __put_user (from->si_int, &to->si_int);
      break;
    case __SI_CHLD >> 16:
      err |= __put_user (from->si_utime, &to->si_utime);
      err |= __put_user (from->si_stime, &to->si_stime);
      err |= __put_user (from->si_status, &to->si_status);
    default:
      err |= __put_user (from->si_pid, &to->si_pid);
      err |= __put_user (from->si_uid, &to->si_uid);
      break;
    case __SI_FAULT >> 16:
      err |= __put_user (from->si_trapno, &to->si_trapno);
      err |= __put_user ( (unsigned long) from->si_addr, &to->si_addr);
      break;
    case __SI_POLL >> 16:
      err |= __put_user (from->si_band, &to->si_band);
      err |= __put_user (from->si_fd, &to->si_fd);
      break;
    case __SI_RT >> 16: /* This is not generated by the kernel as of now.  */
    case __SI_MESGQ >> 16:
      err |= __put_user (from->si_pid, &to->si_pid);
      err |= __put_user (from->si_uid, &to->si_uid);
      err |= __put_user (from->si_int, &to->si_int);
      break;
    }
  }
  return err;
}

/* CAUTION: This is just a very minimalist implementation for the
 *          sake of compat_sys_rt_sigqueueinfo()
 */
int copy_siginfo_from_user32 (siginfo_t * to, compat_siginfo_t __user * from)
{
  if (!access_ok (VERIFY_WRITE, from, sizeof (compat_siginfo_t) ) )
  { return -EFAULT; }
  
  if (copy_from_user (to, from, 3 * sizeof (int) ) ||
      copy_from_user (to->_sifields._pad, from->_sifields._pad,
                      SI_PAD_SIZE) )
  { return -EFAULT; }
  
  return 0;
}

void do_sigreturn32 (struct pt_regs * regs)
{
  struct signal_frame32 __user * sf;
  compat_uptr_t fpu_save;
  compat_uptr_t rwin_save;
  unsigned int psr;
  unsigned pc, npc;
  sigset_t set;
  unsigned seta[_COMPAT_NSIG_WORDS];
  int err, i;
  
  /* Always make any pending restarted system calls return -EINTR */
  current_thread_info()->restart_block.fn = do_no_restart_syscall;
  
  synchronize_user_stack();
  
  regs->u_regs[UREG_FP] &= 0x00000000ffffffffUL;
  sf = (struct signal_frame32 __user *) regs->u_regs[UREG_FP];
  
  /* 1. Make sure we are not getting garbage from the user */
  if (!access_ok (VERIFY_READ, sf, sizeof (*sf) ) ||
      ( ( (unsigned long) sf) & 3) )
  { goto segv; }
  
  get_user (pc, &sf->info.si_regs.pc);
  __get_user (npc, &sf->info.si_regs.npc);
  
  if ( (pc | npc) & 3)
  { goto segv; }
  
  if (test_thread_flag (TIF_32BIT) ) {
    pc &= 0xffffffff;
    npc &= 0xffffffff;
  }
  regs->tpc = pc;
  regs->tnpc = npc;
  
  /* 2. Restore the state */
  err = __get_user (regs->y, &sf->info.si_regs.y);
  err |= __get_user (psr, &sf->info.si_regs.psr);
  
  for (i = UREG_G1; i <= UREG_I7; i++)
  { err |= __get_user (regs->u_regs[i], &sf->info.si_regs.u_regs[i]); }
  if ( (psr & (PSR_VERS | PSR_IMPL) ) == PSR_V8PLUS) {
    err |= __get_user (i, &sf->v8plus.g_upper[0]);
    if (i == SIGINFO_EXTRA_V8PLUS_MAGIC) {
      unsigned long asi;
      
      for (i = UREG_G1; i <= UREG_I7; i++)
      { err |= __get_user ( ( (u32 *) regs->u_regs) [2 * i], &sf->v8plus.g_upper[i]); }
      err |= __get_user (asi, &sf->v8plus.asi);
      regs->tstate &= ~TSTATE_ASI;
      regs->tstate |= ( (asi & 0xffUL) << 24UL);
    }
  }
  
  /* User can only change condition codes in %tstate. */
  regs->tstate &= ~ (TSTATE_ICC | TSTATE_XCC);
  regs->tstate |= psr_to_tstate_icc (psr);
  
  /* Prevent syscall restart.  */
  pt_regs_clear_syscall (regs);
  
  err |= __get_user (fpu_save, &sf->fpu_save);
  if (!err && fpu_save)
  { err |= restore_fpu_state (regs, compat_ptr (fpu_save) ); }
  err |= __get_user (rwin_save, &sf->rwin_save);
  if (!err && rwin_save) {
    if (restore_rwin_state (compat_ptr (rwin_save) ) )
    { goto segv; }
  }
  err |= __get_user (seta[0], &sf->info.si_mask);
  err |= copy_from_user (seta + 1, &sf->extramask,
                         (_COMPAT_NSIG_WORDS - 1) * sizeof (unsigned int) );
  if (err)
  { goto segv; }
  switch (_NSIG_WORDS) {
  case 4: set.sig[3] = seta[6] + ( ( (long) seta[7]) << 32);
  case 3: set.sig[2] = seta[4] + ( ( (long) seta[5]) << 32);
  case 2: set.sig[1] = seta[2] + ( ( (long) seta[3]) << 32);
  case 1: set.sig[0] = seta[0] + ( ( (long) seta[1]) << 32);
  }
  sigdelsetmask (&set, ~_BLOCKABLE);
  set_current_blocked (&set);
  return;
  
segv:
  force_sig (SIGSEGV, current);
}

asmlinkage void do_rt_sigreturn32 (struct pt_regs * regs)
{
  struct rt_signal_frame32 __user * sf;
  unsigned int psr, pc, npc, u_ss_sp;
  compat_uptr_t fpu_save;
  compat_uptr_t rwin_save;
  mm_segment_t old_fs;
  sigset_t set;
  compat_sigset_t seta;
  stack_t st;
  int err, i;
  
  /* Always make any pending restarted system calls return -EINTR */
  current_thread_info()->restart_block.fn = do_no_restart_syscall;
  
  synchronize_user_stack();
  regs->u_regs[UREG_FP] &= 0x00000000ffffffffUL;
  sf = (struct rt_signal_frame32 __user *) regs->u_regs[UREG_FP];
  
  /* 1. Make sure we are not getting garbage from the user */
  if (!access_ok (VERIFY_READ, sf, sizeof (*sf) ) ||
      ( ( (unsigned long) sf) & 3) )
  { goto segv; }
  
  get_user (pc, &sf->regs.pc);
  __get_user (npc, &sf->regs.npc);
  
  if ( (pc | npc) & 3)
  { goto segv; }
  
  if (test_thread_flag (TIF_32BIT) ) {
    pc &= 0xffffffff;
    npc &= 0xffffffff;
  }
  regs->tpc = pc;
  regs->tnpc = npc;
  
  /* 2. Restore the state */
  err = __get_user (regs->y, &sf->regs.y);
  err |= __get_user (psr, &sf->regs.psr);
  
  for (i = UREG_G1; i <= UREG_I7; i++)
  { err |= __get_user (regs->u_regs[i], &sf->regs.u_regs[i]); }
  if ( (psr & (PSR_VERS | PSR_IMPL) ) == PSR_V8PLUS) {
    err |= __get_user (i, &sf->v8plus.g_upper[0]);
    if (i == SIGINFO_EXTRA_V8PLUS_MAGIC) {
      unsigned long asi;
      
      for (i = UREG_G1; i <= UREG_I7; i++)
      { err |= __get_user ( ( (u32 *) regs->u_regs) [2 * i], &sf->v8plus.g_upper[i]); }
      err |= __get_user (asi, &sf->v8plus.asi);
      regs->tstate &= ~TSTATE_ASI;
      regs->tstate |= ( (asi & 0xffUL) << 24UL);
    }
  }
  
  /* User can only change condition codes in %tstate. */
  regs->tstate &= ~ (TSTATE_ICC | TSTATE_XCC);
  regs->tstate |= psr_to_tstate_icc (psr);
  
  /* Prevent syscall restart.  */
  pt_regs_clear_syscall (regs);
  
  err |= __get_user (fpu_save, &sf->fpu_save);
  if (!err && fpu_save)
  { err |= restore_fpu_state (regs, compat_ptr (fpu_save) ); }
  err |= copy_from_user (&seta, &sf->mask, sizeof (compat_sigset_t) );
  err |= __get_user (u_ss_sp, &sf->stack.ss_sp);
  st.ss_sp = compat_ptr (u_ss_sp);
  err |= __get_user (st.ss_flags, &sf->stack.ss_flags);
  err |= __get_user (st.ss_size, &sf->stack.ss_size);
  if (err)
  { goto segv; }
  
  /* It is more difficult to avoid calling this function than to
     call it and ignore errors.  */
  old_fs = get_fs();
  set_fs (KERNEL_DS);
  do_sigaltstack ( (stack_t __user *) &st, NULL, (unsigned long) sf);
  set_fs (old_fs);
  
  err |= __get_user (rwin_save, &sf->rwin_save);
  if (!err && rwin_save) {
    if (restore_rwin_state (compat_ptr (rwin_save) ) )
    { goto segv; }
  }
  
  switch (_NSIG_WORDS) {
  case 4: set.sig[3] = seta.sig[6] + ( ( (long) seta.sig[7]) << 32);
  case 3: set.sig[2] = seta.sig[4] + ( ( (long) seta.sig[5]) << 32);
  case 2: set.sig[1] = seta.sig[2] + ( ( (long) seta.sig[3]) << 32);
  case 1: set.sig[0] = seta.sig[0] + ( ( (long) seta.sig[1]) << 32);
  }
  sigdelsetmask (&set, ~_BLOCKABLE);
  set_current_blocked (&set);
  return;
segv:
  force_sig (SIGSEGV, current);
}

/* Checks if the fp is valid */
static int invalid_frame_pointer (void __user * fp, int fplen)
{
  if ( ( ( (unsigned long) fp) & 7) || ( (unsigned long) fp) > 0x100000000ULL - fplen)
  { return 1; }
  return 0;
}

static void __user * get_sigframe (struct sigaction * sa, struct pt_regs * regs, unsigned long framesize)
{
  unsigned long sp;
  
  regs->u_regs[UREG_FP] &= 0x00000000ffffffffUL;
  sp = regs->u_regs[UREG_FP];
  
  /*
   * If we are on the alternate signal stack and would overflow it, don't.
   * Return an always-bogus address instead so we will die with SIGSEGV.
   */
  if (on_sig_stack (sp) && !likely (on_sig_stack (sp - framesize) ) )
  { return (void __user *) - 1L; }
  
  /* This is the X/Open sanctioned signal stack switching.  */
  if (sa->sa_flags & SA_ONSTACK) {
    if (sas_ss_flags (sp) == 0)
    { sp = current->sas_ss_sp + current->sas_ss_size; }
  }
  
  sp -= framesize;
  
  /* Always align the stack frame.  This handles two cases.  First,
   * sigaltstack need not be mindful of platform specific stack
   * alignment.  Second, if we took this signal because the stack
   * is not aligned properly, we'd like to take the signal cleanly
   * and report that.
   */
  sp &= ~15UL;
  
  return (void __user *) sp;
}

/* The I-cache flush instruction only works in the primary ASI, which
 * right now is the nucleus, aka. kernel space.
 *
 * Therefore we have to kick the instructions out using the kernel
 * side linear mapping of the physical address backing the user
 * instructions.
 */
static void flush_signal_insns (unsigned long address)
{
  unsigned long pstate, paddr;
  pte_t * ptep, pte;
  pgd_t * pgdp;
  pud_t * pudp;
  pmd_t * pmdp;
  
  /* Commit all stores of the instructions we are about to flush.  */
  wmb();
  
  /* Disable cross-call reception.  In this way even a very wide
   * munmap() on another cpu can't tear down the page table
   * hierarchy from underneath us, since that can't complete
   * until the IPI tlb flush returns.
   */
  
  __asm__ __volatile__ ("rdpr %%pstate, %0" : "=r" (pstate) );
  __asm__ __volatile__ ("wrpr %0, %1, %%pstate"
                        : : "r" (pstate), "i" (PSTATE_IE) );
                        
  pgdp = pgd_offset (current->mm, address);
  if (pgd_none (*pgdp) )
  { goto out_irqs_on; }
  pudp = pud_offset (pgdp, address);
  if (pud_none (*pudp) )
  { goto out_irqs_on; }
  pmdp = pmd_offset (pudp, address);
  if (pmd_none (*pmdp) )
  { goto out_irqs_on; }
  
  ptep = pte_offset_map (pmdp, address);
  pte = *ptep;
  if (!pte_present (pte) )
  { goto out_unmap; }
  
  paddr = (unsigned long) page_address (pte_page (pte) );
  
  __asm__ __volatile__ ("flush	%0 + %1"
                        : /* no outputs */
                        : "r" (paddr),
                        "r" (address & (PAGE_SIZE - 1) )
                        : "memory");
                        
out_unmap:
  pte_unmap (ptep);
out_irqs_on:
  __asm__ __volatile__ ("wrpr %0, 0x0, %%pstate" : : "r" (pstate) );
  
}

static int setup_frame32 (struct k_sigaction * ka, struct pt_regs * regs,
                          int signo, sigset_t * oldset)
{
  struct signal_frame32 __user * sf;
  int i, err, wsaved;
  void __user * tail;
  int sigframe_size;
  u32 psr;
  unsigned int seta[_COMPAT_NSIG_WORDS];
  
  /* 1. Make sure everything is clean */
  synchronize_user_stack();
  save_and_clear_fpu();
  
  wsaved = get_thread_wsaved();
  
  sigframe_size = sizeof (*sf);
  if (current_thread_info()->fpsaved[0] & FPRS_FEF)
  { sigframe_size += sizeof (__siginfo_fpu_t); }
  if (wsaved)
  { sigframe_size += sizeof (__siginfo_rwin_t); }
  
  sf = (struct signal_frame32 __user *)
       get_sigframe (&ka->sa, regs, sigframe_size);
       
  if (invalid_frame_pointer (sf, sigframe_size) )
  { goto sigill; }
  
  tail = (sf + 1);
  
  /* 2. Save the current process state */
  if (test_thread_flag (TIF_32BIT) ) {
    regs->tpc &= 0xffffffff;
    regs->tnpc &= 0xffffffff;
  }
  err  = put_user (regs->tpc, &sf->info.si_regs.pc);
  err |= __put_user (regs->tnpc, &sf->info.si_regs.npc);
  err |= __put_user (regs->y, &sf->info.si_regs.y);
  psr = tstate_to_psr (regs->tstate);
  if (current_thread_info()->fpsaved[0] & FPRS_FEF)
  { psr |= PSR_EF; }
  err |= __put_user (psr, &sf->info.si_regs.psr);
  for (i = 0; i < 16; i++)
  { err |= __put_user (regs->u_regs[i], &sf->info.si_regs.u_regs[i]); }
  err |= __put_user (sizeof (siginfo_extra_v8plus_t), &sf->extra_size);
  err |= __put_user (SIGINFO_EXTRA_V8PLUS_MAGIC, &sf->v8plus.g_upper[0]);
  for (i = 1; i < 16; i++)
    err |= __put_user ( ( (u32 *) regs->u_regs) [2 * i],
                        &sf->v8plus.g_upper[i]);
  err |= __put_user ( (regs->tstate & TSTATE_ASI) >> 24UL,
                      &sf->v8plus.asi);
                      
  if (psr & PSR_EF) {
    __siginfo_fpu_t __user * fp = tail;
    tail += sizeof (*fp);
    err |= save_fpu_state (regs, fp);
    err |= __put_user ( (u64) fp, &sf->fpu_save);
  }
  else {
    err |= __put_user (0, &sf->fpu_save);
  }
  if (wsaved) {
    __siginfo_rwin_t __user * rwp = tail;
    tail += sizeof (*rwp);
    err |= save_rwin_state (wsaved, rwp);
    err |= __put_user ( (u64) rwp, &sf->rwin_save);
    set_thread_wsaved (0);
  }
  else {
    err |= __put_user (0, &sf->rwin_save);
  }
  
  switch (_NSIG_WORDS) {
  case 4: seta[7] = (oldset->sig[3] >> 32);
    seta[6] = oldset->sig[3];
  case 3: seta[5] = (oldset->sig[2] >> 32);
    seta[4] = oldset->sig[2];
  case 2: seta[3] = (oldset->sig[1] >> 32);
    seta[2] = oldset->sig[1];
  case 1: seta[1] = (oldset->sig[0] >> 32);
    seta[0] = oldset->sig[0];
  }
  err |= __put_user (seta[0], &sf->info.si_mask);
  err |= __copy_to_user (sf->extramask, seta + 1,
                         (_COMPAT_NSIG_WORDS - 1) * sizeof (unsigned int) );
                         
  if (!wsaved) {
    err |= copy_in_user ( (u32 __user *) sf,
                          (u32 __user *) (regs->u_regs[UREG_FP]),
                          sizeof (struct reg_window32) );
  }
  else {
    struct reg_window * rp;
    
    rp = &current_thread_info()->reg_window[wsaved - 1];
    for (i = 0; i < 8; i++)
    { err |= __put_user (rp->locals[i], &sf->ss.locals[i]); }
    for (i = 0; i < 6; i++)
    { err |= __put_user (rp->ins[i], &sf->ss.ins[i]); }
    err |= __put_user (rp->ins[6], &sf->ss.fp);
    err |= __put_user (rp->ins[7], &sf->ss.callers_pc);
  }
  if (err)
  { goto sigsegv; }
  
  /* 3. signal handler back-trampoline and parameters */
  regs->u_regs[UREG_FP] = (unsigned long) sf;
  regs->u_regs[UREG_I0] = signo;
  regs->u_regs[UREG_I1] = (unsigned long) &sf->info;
  regs->u_regs[UREG_I2] = (unsigned long) &sf->info;
  
  /* 4. signal handler */
  regs->tpc = (unsigned long) ka->sa.sa_handler;
  regs->tnpc = (regs->tpc + 4);
  if (test_thread_flag (TIF_32BIT) ) {
    regs->tpc &= 0xffffffff;
    regs->tnpc &= 0xffffffff;
  }
  
  /* 5. return to kernel instructions */
  if (ka->ka_restorer) {
    regs->u_regs[UREG_I7] = (unsigned long) ka->ka_restorer;
  }
  else {
    unsigned long address = ( (unsigned long) & (sf->insns[0]) );
    
    regs->u_regs[UREG_I7] = (unsigned long) (& (sf->insns[0]) - 2);
    
    err  = __put_user (0x821020d8, &sf->insns[0]); /*mov __NR_sigreturn, %g1*/
    err |= __put_user (0x91d02010, &sf->insns[1]); /*t 0x10*/
    if (err)
    { goto sigsegv; }
    flush_signal_insns (address);
  }
  return 0;
  
sigill:
  do_exit (SIGILL);
  return -EINVAL;
  
sigsegv:
  force_sigsegv (signo, current);
  return -EFAULT;
}

static int setup_rt_frame32 (struct k_sigaction * ka, struct pt_regs * regs,
                             unsigned long signr, sigset_t * oldset,
                             siginfo_t * info)
{
  struct rt_signal_frame32 __user * sf;
  int i, err, wsaved;
  void __user * tail;
  int sigframe_size;
  u32 psr;
  compat_sigset_t seta;
  
  /* 1. Make sure everything is clean */
  synchronize_user_stack();
  save_and_clear_fpu();
  
  wsaved = get_thread_wsaved();
  
  sigframe_size = sizeof (*sf);
  if (current_thread_info()->fpsaved[0] & FPRS_FEF)
  { sigframe_size += sizeof (__siginfo_fpu_t); }
  if (wsaved)
  { sigframe_size += sizeof (__siginfo_rwin_t); }
  
  sf = (struct rt_signal_frame32 __user *)
       get_sigframe (&ka->sa, regs, sigframe_size);
       
  if (invalid_frame_pointer (sf, sigframe_size) )
  { goto sigill; }
  
  tail = (sf + 1);
  
  /* 2. Save the current process state */
  if (test_thread_flag (TIF_32BIT) ) {
    regs->tpc &= 0xffffffff;
    regs->tnpc &= 0xffffffff;
  }
  err  = put_user (regs->tpc, &sf->regs.pc);
  err |= __put_user (regs->tnpc, &sf->regs.npc);
  err |= __put_user (regs->y, &sf->regs.y);
  psr = tstate_to_psr (regs->tstate);
  if (current_thread_info()->fpsaved[0] & FPRS_FEF)
  { psr |= PSR_EF; }
  err |= __put_user (psr, &sf->regs.psr);
  for (i = 0; i < 16; i++)
  { err |= __put_user (regs->u_regs[i], &sf->regs.u_regs[i]); }
  err |= __put_user (sizeof (siginfo_extra_v8plus_t), &sf->extra_size);
  err |= __put_user (SIGINFO_EXTRA_V8PLUS_MAGIC, &sf->v8plus.g_upper[0]);
  for (i = 1; i < 16; i++)
    err |= __put_user ( ( (u32 *) regs->u_regs) [2 * i],
                        &sf->v8plus.g_upper[i]);
  err |= __put_user ( (regs->tstate & TSTATE_ASI) >> 24UL,
                      &sf->v8plus.asi);
                      
  if (psr & PSR_EF) {
    __siginfo_fpu_t __user * fp = tail;
    tail += sizeof (*fp);
    err |= save_fpu_state (regs, fp);
    err |= __put_user ( (u64) fp, &sf->fpu_save);
  }
  else {
    err |= __put_user (0, &sf->fpu_save);
  }
  if (wsaved) {
    __siginfo_rwin_t __user * rwp = tail;
    tail += sizeof (*rwp);
    err |= save_rwin_state (wsaved, rwp);
    err |= __put_user ( (u64) rwp, &sf->rwin_save);
    set_thread_wsaved (0);
  }
  else {
    err |= __put_user (0, &sf->rwin_save);
  }
  
  /* Update the siginfo structure.  */
  err |= copy_siginfo_to_user32 (&sf->info, info);
  
  /* Setup sigaltstack */
  err |= __put_user (current->sas_ss_sp, &sf->stack.ss_sp);
  err |= __put_user (sas_ss_flags (regs->u_regs[UREG_FP]), &sf->stack.ss_flags);
  err |= __put_user (current->sas_ss_size, &sf->stack.ss_size);
  
  switch (_NSIG_WORDS) {
  case 4: seta.sig[7] = (oldset->sig[3] >> 32);
    seta.sig[6] = oldset->sig[3];
  case 3: seta.sig[5] = (oldset->sig[2] >> 32);
    seta.sig[4] = oldset->sig[2];
  case 2: seta.sig[3] = (oldset->sig[1] >> 32);
    seta.sig[2] = oldset->sig[1];
  case 1: seta.sig[1] = (oldset->sig[0] >> 32);
    seta.sig[0] = oldset->sig[0];
  }
  err |= __copy_to_user (&sf->mask, &seta, sizeof (compat_sigset_t) );
  
  if (!wsaved) {
    err |= copy_in_user ( (u32 __user *) sf,
                          (u32 __user *) (regs->u_regs[UREG_FP]),
                          sizeof (struct reg_window32) );
  }
  else {
    struct reg_window * rp;
    
    rp = &current_thread_info()->reg_window[wsaved - 1];
    for (i = 0; i < 8; i++)
    { err |= __put_user (rp->locals[i], &sf->ss.locals[i]); }
    for (i = 0; i < 6; i++)
    { err |= __put_user (rp->ins[i], &sf->ss.ins[i]); }
    err |= __put_user (rp->ins[6], &sf->ss.fp);
    err |= __put_user (rp->ins[7], &sf->ss.callers_pc);
  }
  if (err)
  { goto sigsegv; }
  
  /* 3. signal handler back-trampoline and parameters */
  regs->u_regs[UREG_FP] = (unsigned long) sf;
  regs->u_regs[UREG_I0] = signr;
  regs->u_regs[UREG_I1] = (unsigned long) &sf->info;
  regs->u_regs[UREG_I2] = (unsigned long) &sf->regs;
  
  /* 4. signal handler */
  regs->tpc = (unsigned long) ka->sa.sa_handler;
  regs->tnpc = (regs->tpc + 4);
  if (test_thread_flag (TIF_32BIT) ) {
    regs->tpc &= 0xffffffff;
    regs->tnpc &= 0xffffffff;
  }
  
  /* 5. return to kernel instructions */
  if (ka->ka_restorer)
  { regs->u_regs[UREG_I7] = (unsigned long) ka->ka_restorer; }
  else {
    unsigned long address = ( (unsigned long) & (sf->insns[0]) );
    
    regs->u_regs[UREG_I7] = (unsigned long) (& (sf->insns[0]) - 2);
    
    /* mov __NR_rt_sigreturn, %g1 */
    err |= __put_user (0x82102065, &sf->insns[0]);
    
    /* t 0x10 */
    err |= __put_user (0x91d02010, &sf->insns[1]);
    if (err)
    { goto sigsegv; }
    
    flush_signal_insns (address);
  }
  return 0;
  
sigill:
  do_exit (SIGILL);
  return -EINVAL;
  
sigsegv:
  force_sigsegv (signr, current);
  return -EFAULT;
}

static inline int handle_signal32 (unsigned long signr, struct k_sigaction * ka,
                                   siginfo_t * info,
                                   sigset_t * oldset, struct pt_regs * regs)
{
  int err;
  
  if (ka->sa.sa_flags & SA_SIGINFO)
  { err = setup_rt_frame32 (ka, regs, signr, oldset, info); }
  else
  { err = setup_frame32 (ka, regs, signr, oldset); }
  
  if (err)
  { return err; }
  
  block_sigmask (ka, signr);
  tracehook_signal_handler (signr, info, ka, regs, 0);
  
  return 0;
}

static inline void syscall_restart32 (unsigned long orig_i0, struct pt_regs * regs,
                                      struct sigaction * sa)
{
  switch (regs->u_regs[UREG_I0]) {
  case ERESTART_RESTARTBLOCK:
  case ERESTARTNOHAND:
no_system_call_restart:
    regs->u_regs[UREG_I0] = EINTR;
    regs->tstate |= TSTATE_ICARRY;
    break;
  case ERESTARTSYS:
    if (! (sa->sa_flags & SA_RESTART) )
    { goto no_system_call_restart; }
  /* fallthrough */
  case ERESTARTNOINTR:
    regs->u_regs[UREG_I0] = orig_i0;
    regs->tpc -= 4;
    regs->tnpc -= 4;
  }
}

/* Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 */
void do_signal32 (sigset_t * oldset, struct pt_regs * regs)
{
  struct k_sigaction ka;
  unsigned long orig_i0;
  int restart_syscall;
  siginfo_t info;
  int signr;
  
  signr = get_signal_to_deliver (&info, &ka, regs, NULL);
  
  restart_syscall = 0;
  orig_i0 = 0;
  if (pt_regs_is_syscall (regs) &&
      (regs->tstate & (TSTATE_XCARRY | TSTATE_ICARRY) ) ) {
    restart_syscall = 1;
    orig_i0 = regs->u_regs[UREG_G6];
  }
  
  if (signr > 0) {
    if (restart_syscall)
    { syscall_restart32 (orig_i0, regs, &ka.sa); }
    if (handle_signal32 (signr, &ka, &info, oldset, regs) == 0) {
      /* A signal was successfully delivered; the saved
       * sigmask will have been stored in the signal frame,
       * and will be restored by sigreturn, so we can simply
       * clear the TS_RESTORE_SIGMASK flag.
       */
      current_thread_info()->status &= ~TS_RESTORE_SIGMASK;
    }
    return;
  }
  if (restart_syscall &&
      (regs->u_regs[UREG_I0] == ERESTARTNOHAND ||
       regs->u_regs[UREG_I0] == ERESTARTSYS ||
       regs->u_regs[UREG_I0] == ERESTARTNOINTR) ) {
    /* replay the system call when we are done */
    regs->u_regs[UREG_I0] = orig_i0;
    regs->tpc -= 4;
    regs->tnpc -= 4;
    pt_regs_clear_syscall (regs);
  }
  if (restart_syscall &&
      regs->u_regs[UREG_I0] == ERESTART_RESTARTBLOCK) {
    regs->u_regs[UREG_G1] = __NR_restart_syscall;
    regs->tpc -= 4;
    regs->tnpc -= 4;
    pt_regs_clear_syscall (regs);
  }
  
  /* If there's no signal to deliver, we just put the saved sigmask
   * back
   */
  if (current_thread_info()->status & TS_RESTORE_SIGMASK) {
    current_thread_info()->status &= ~TS_RESTORE_SIGMASK;
    set_current_blocked (&current->saved_sigmask);
  }
}

struct sigstack32 {
  u32 the_stack;
  int cur_status;
};

asmlinkage int do_sys32_sigstack (u32 u_ssptr, u32 u_ossptr, unsigned long sp)
{
  struct sigstack32 __user * ssptr =
    (struct sigstack32 __user *) ( (unsigned long) (u_ssptr) );
  struct sigstack32 __user * ossptr =
    (struct sigstack32 __user *) ( (unsigned long) (u_ossptr) );
  int ret = -EFAULT;
  
  /* First see if old state is wanted. */
  if (ossptr) {
    if (put_user (current->sas_ss_sp + current->sas_ss_size,
                  &ossptr->the_stack) ||
        __put_user (on_sig_stack (sp), &ossptr->cur_status) )
    { goto out; }
  }
  
  /* Now see if we want to update the new state. */
  if (ssptr) {
    u32 ss_sp;
    
    if (get_user (ss_sp, &ssptr->the_stack) )
    { goto out; }
    
    /* If the current stack was set with sigaltstack, don't
     * swap stacks while we are on it.
     */
    ret = -EPERM;
    if (current->sas_ss_sp && on_sig_stack (sp) )
    { goto out; }
    
    /* Since we don't know the extent of the stack, and we don't
     * track onstack-ness, but rather calculate it, we must
     * presume a size.  Ho hum this interface is lossy.
     */
    current->sas_ss_sp = (unsigned long) ss_sp - SIGSTKSZ;
    current->sas_ss_size = SIGSTKSZ;
  }
  
  ret = 0;
out:
  return ret;
}

asmlinkage long do_sys32_sigaltstack (u32 ussa, u32 uossa, unsigned long sp)
{
  stack_t uss, uoss;
  u32 u_ss_sp = 0;
  int ret;
  mm_segment_t old_fs;
  stack_t32 __user * uss32 = compat_ptr (ussa);
  stack_t32 __user * uoss32 = compat_ptr (uossa);
  
  if (ussa && (get_user (u_ss_sp, &uss32->ss_sp) ||
               __get_user (uss.ss_flags, &uss32->ss_flags) ||
               __get_user (uss.ss_size, &uss32->ss_size) ) )
  { return -EFAULT; }
  uss.ss_sp = compat_ptr (u_ss_sp);
  old_fs = get_fs();
  set_fs (KERNEL_DS);
  ret = do_sigaltstack (ussa ? (stack_t __user *) &uss : NULL,
                        uossa ? (stack_t __user *) &uoss : NULL, sp);
  set_fs (old_fs);
  if (!ret && uossa && (put_user (ptr_to_compat (uoss.ss_sp), &uoss32->ss_sp) ||
                        __put_user (uoss.ss_flags, &uoss32->ss_flags) ||
                        __put_user (uoss.ss_size, &uoss32->ss_size) ) )
  { return -EFAULT; }
  return ret;
}