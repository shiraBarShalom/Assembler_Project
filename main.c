#include "pre_assembler.h"

int main() {
    /* test with file name including extension */
    if (run_pre_assembler("test.as")) {
        printf("pre assembler finished successfully\n");
    } else {
        printf("pre assembler failed due to errors\n");
    }
    
    return 0;
}