#include "ex.h"

void main(){
    global_var = 1; 
    __CPROVER_assert(global_var == 1, "global variable value is correct");
    test2(); 
    test3(); 
    __CPROVER_havoc_object(&global_var);
    __CPROVER_printf("HELLO SOPHIA global_var: %d", global_var);
    __CPROVER_assert(global_var == 3, "global variable value is correct");
}