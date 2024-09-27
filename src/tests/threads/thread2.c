
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"

void looop(void* arg UNUSED) ;

void looop(void* arg UNUSED) {
    uint64_t i = 0;
    while (true) {
        i += 1;
    }
}


void test_thread2(void) {
    thread_create("loop1", PRI_DEFAULT, looop, (void*) 0);
    thread_create("loop2", PRI_DEFAULT, looop, (void*) 0);
    looop((void*) 0);
}

