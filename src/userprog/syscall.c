#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "pagedir.h"
#include "process.h"

struct sysarg {
  int32_t NUMBER;
  int32_t ARG0;
  int32_t ARG1;
  int32_t ARG2;
};

static void syscall_handler (struct intr_frame *);
static uint32_t syscall_exit(struct sysarg* arg);
static uint32_t syscall_write(struct sysarg* arg);
static uint32_t syscall_wait(struct sysarg* arg);
static uint32_t syscall_exec(struct sysarg* arg);
static uint32_t syscall_create(struct sysarg* arg);


static inline bool check_ptr(const void *vaddr) {
  if (!is_user_vaddr(vaddr))
    return false;
  void* ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (ptr == NULL) 
    return false;
  return true;
}

static inline void check_nptr(const void *vaddr, int n) {
  bool res = true;
  uint8_t* ptr = (uint8_t*) vaddr;
  for (int i = 0; i < n; i++) {
    if (!check_ptr((void*) (ptr + i))) {
      res = false;
      break;
    }
  }
  if (!res) {
    tinfos[thread_current()->tid].ex_code = -1;
    thread_exit();
  }
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  check_nptr(f->esp, 4);

  uint32_t sys_number = *((uint32_t*)f->esp); 
  
  switch (sys_number)
  {
  case SYS_HALT:
    shutdown_power_off();
    break;
  
  case SYS_EXIT:
    syscall_exit(f->esp);
    break;

  case SYS_EXEC:
    f->eax = syscall_exec(f->esp);
    break;
  case SYS_WRITE:
    f->eax =  syscall_write(f->esp);
    break;

  case SYS_WAIT:
    f->eax = syscall_wait(f->esp);
    break;
  
  case SYS_CREATE:
    f->eax = syscall_create(f->esp);
  
  default:
    thread_exit ();
    break;
  }

}

static uint32_t 
syscall_exit(struct sysarg* arg) {
  check_nptr( (void*) arg, 8);
  tinfos[thread_current()->tid].ex_code = arg->ARG0;
  thread_exit ();
}

static uint32_t 
syscall_exec(struct sysarg* arg) {
  check_nptr((void*) arg, 8);
  char* file_name = (char*) arg->ARG0;
  // get child pid, may add this to 
  // thread struct
  check_nptr((void*)file_name, 2);
  return process_execute(file_name);
}

static uint32_t
syscall_wait(struct sysarg* arg) {
  check_nptr((void*) arg, 8);

  tid_t tid = arg->ARG0;
  return process_wait(tid);
}

static uint32_t 
syscall_write(struct sysarg* arg) {
  check_nptr( (void*)arg , 16);

  int fd = arg->ARG0;
  char* buf = (char*)arg->ARG1;
  size_t size = arg->ARG2;
  if (fd == 1 ) {
    putbuf(buf, size);
    return size;
  }
  return 0 ;
}

static uint32_t
syscall_create(struct sysarg* arg) {
  
}
