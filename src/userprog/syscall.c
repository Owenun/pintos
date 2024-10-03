#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "pagedir.h"
#include "process.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "string.h"


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
static uint32_t syscall_remove(struct sysarg* arg);
static uint32_t syscall_open(struct sysarg* arg);
static uint32_t syscall_seek(struct sysarg* arg);
static uint32_t syscall_filesize(struct sysarg* arg);
static uint32_t syscall_tell(struct sysarg* arg UNUSED);
static uint32_t syscall_read(struct sysarg* arg);
static uint32_t syscall_close(struct sysarg* arg);


static int allocfd(struct file * f);
static bool closefd (int fd);
static inline bool check_ptr(const void *vaddr);
static inline void check_nptr(const void *vaddr, int n);



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

  case SYS_WAIT:
    f->eax = syscall_wait(f->esp);
    break;
  
  case SYS_CREATE:
    f->eax = syscall_create(f->esp);
    break;
  
  case SYS_REMOVE:
    f->eax = syscall_remove(f->esp);
    break;

  case SYS_WRITE:
    f->eax =  syscall_write(f->esp);
    break;

  case SYS_READ:
    f->eax =  syscall_read(f->esp);
    break;

  case SYS_OPEN:
    f->eax = syscall_open(f->esp);
    break;

  case SYS_CLOSE:
    f->eax = syscall_close(f->esp);
    break;

  case SYS_FILESIZE:
    f->eax = syscall_filesize(f->esp);
    break;
  case SYS_SEEK:
    f->eax = syscall_seek(f->esp);
    break;
  case SYS_TELL:
    f->eax = syscall_tell(f->esp);
    break;
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
  size_t size = arg->ARG2;
  char* buf = (char*)arg->ARG1;
  check_nptr((void*) buf, 2);
  if (fd >= 128 || fd <= 0) return -1;
  if (fd == 1 ) {
    putbuf(buf, size);
    return size;
  }
  struct fdelem* fde = &thread_current()->fds[fd];
  if (fde->fd != fd) return -1;

  lock_acquire(&fslock);
  int len = file_write(fde->f, buf, size);
  lock_release(&fslock);
  return len;
}

static bool check_file_name(char* file_name) {
  check_nptr((void*)file_name, 2);
  size_t len = strnlen(file_name, 15);
  if (len >= 15) return false;
  return true;
}

static uint32_t
syscall_create(struct sysarg* arg) {  
  check_nptr( (void*)arg , 12);
  char* file_name = (char*) arg->ARG0;
  if (!check_file_name(file_name)) {
    return -1;
  }
  unsigned initial_size = arg->ARG1;

  lock_acquire(&fslock);
  bool success = filesys_create(file_name, initial_size);
  lock_release(&fslock);
  return success;
}

static uint32_t
syscall_remove(struct sysarg* arg) {
  check_nptr( (void*)arg , 8);
  char* file_name = (char*) arg->ARG0;
  if(!check_file_name(file_name)) {
    return -1;
  }
  lock_acquire(&fslock);
  bool success = filesys_remove(file_name);
  lock_release(&fslock);
  return success;
}


static uint32_t
syscall_open(struct sysarg* arg) {
  check_nptr( (void*)arg , 8);
  char* file_name = (char*) arg->ARG0;
  if(!check_file_name(file_name)) {
    return -1;
  }

  lock_acquire(&fslock);
  struct file* f = filesys_open(file_name);
  if (f == NULL) {
    /** stupid */
    lock_release(&fslock);
    return -1;
  }
  int fd = allocfd(f);
  if (fd == -1) file_close(f);
  lock_release(&fslock);
  return fd;
}


static uint32_t
syscall_close(struct sysarg* arg) {
  check_nptr( (void*)arg , 8);
  int fd = arg->ARG0;
  if (fd < 0 || fd >= 128 || fd == 0 || fd == 1) {
    return -1;
  }

  lock_acquire(&fslock);
  bool success = closefd(fd);
  lock_release(&fslock);
  return success ? 0 : -1;
}

static uint32_t 
syscall_read(struct sysarg* arg) {
  check_nptr( (void*)arg , 16);
  void* buffer = (char*) arg->ARG1;
  int fd = arg->ARG0;
  unsigned size = arg->ARG2;
  check_nptr(buffer, 2);
  
  if (fd < 0 || fd >= 128) return -1;
  if (fd == 0) return -1;
  if (fd == 1) return -1;

  struct fdelem* fde = &thread_current()->fds[fd];
  if (fde->fd != fd) return -1;
  struct file * f = fde->f;

  lock_acquire(&fslock);
  int len =  file_read(f, buffer, size);
  lock_release(&fslock);
  return len;
}

static uint32_t 
syscall_seek(struct sysarg* arg) {
  check_nptr( (void*)arg , 12);
  int fd = arg->ARG0;
  off_t position = arg->ARG1;
  if (fd <= 1 || fd >= 128) return -1;
  struct fdelem* fde = &thread_current()->fds[fd];
  if (fde->fd != fd) return -1;

  lock_acquire(&fslock);
  file_seek(fde->f, position);
  lock_release(&fslock);

  return 0;
}


static uint32_t 
syscall_tell(struct sysarg* arg UNUSED) {
  check_nptr( (void*)arg , 8);
  int fd = arg->ARG0;
  if (fd <= 1 || fd >= 128) return -1;
  struct fdelem* fde = &thread_current()->fds[fd];
  if (fde->fd != fd) return -1;

  lock_acquire(&fslock);
  off_t c = file_tell(fde->f);
  lock_release(&fslock);
  return c;
}

static uint32_t
syscall_filesize(struct sysarg* arg) {
  check_nptr( (void*)arg , 8);
  int fd = arg->ARG0;
  if (fd <= 1 || fd >= 128) return -1;
  struct fdelem* fde = &thread_current()->fds[fd];
  if (fde->fd != fd) return -1;

  lock_acquire(&fslock);
  int len = file_length(fde->f);
  lock_release(&fslock);
  return len;
}


static int allocfd(struct file * f) {
  if (thread_current()->fdmax >= 128) {
    return -1;
  }
  thread_current()->fdmax++;
  int newfd = thread_current()->fdmax;
  ASSERT(newfd > 1 && newfd < 128);
  struct fdelem* fde =  &thread_current()->fds[newfd];
  fde->f = f;
  fde->fd = newfd;
  return newfd;
}

static bool closefd (int fd) {
  ASSERT(fd > 1 && fd < 128);
  struct fdelem* fde = &thread_current()->fds[fd];
  if (fde->fd != fd) return false;
  file_close(fde->f);
  fde->f = NULL;
  fde->fd = 0;
  return true;
}

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