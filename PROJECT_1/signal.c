/*signal.c
 *
 *Group Members Names and NetIDs:
 *	1.Vaishnavi Manthena (vm504)
 *	2.Sanjana Pendharkar (sp1631)
 *
 *ILab Machine Tested on: iLab1.cs.rutgers.edu
 */


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* Part 1 - Step 2 to 4: Do your tricks here
 * Your goal must be to change the stack frame of caller (main function)
 * such that you get to the line after "r2 = *( (int *) 0 )"
 */
void segment_fault_handler(int signum) {

    
    printf("I am slain!\n");
    /* Implement Code Here */

    
    int * ptr = &signum ;

   //printf("addr of signum : %p\n", ptr);

    ptr = ptr + 51; //This makes ptr point to an adress of 204  above it. 204/4 = 51

    //printf("addr of signum + 51 : %p\n", ptr);

    

    char * * pcptr_ptr = (char **) ptr;
    //printf("pcptr_ptr : %p\n", pcptr_ptr);
    //printf("pcptr_data : %p\n", *pcptr_ptr);
    
    
    *pcptr_ptr = *pcptr_ptr + 5;
    //printf("pcptr_ptr_2 : %p\n", pcptr_ptr);
    //printf("pcptr_data_2 : %p\n", *pcptr_ptr);

}

int main(int argc, char *argv[]) {
 
    int r2 = 0;
    
    /* Part 1 - Step 1: Registering signal handler */
    /* Implement Code Here */
    signal(SIGSEGV, segment_fault_handler);
   
    
    r2 = *( (int *) 0 ); // This will generate segmentation fault

    printf("I live again!\n");

    return 0;
}
