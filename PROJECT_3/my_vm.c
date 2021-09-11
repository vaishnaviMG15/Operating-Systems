#include "my_vm.h"
#define sizeOfArray (PGSIZE/sizeof(unsigned long))
#define NUL '\0'

//extra function definitions
void settingUpPT();
void setGlobals();
static void set_bit_at_index(char *bitmap, int index);
static int get_bit_at_index(char *bitmap, int index);
static void reset_bit_at_index(char *bitmap, int index);

//calculating the number of physical and virtual pages

unsigned long long physicalPageNum = (MEMSIZE)/(PGSIZE);
unsigned long long virtualPageNum = (MAX_MEMSIZE)/(PGSIZE);

//These are the number of unsigned long values thta can fit into a page
//of physical/virtual memory
//const int sizeOfArray = PGSIZE/sizeof(unsigned long);

//Now we know that size of the struct page is the same as the size of a physical page
struct page{

    unsigned long array[sizeOfArray];

};

pthread_mutex_t mutex;
struct page * physicalMemory;
char * pBitmap;
char * vBitmap;
pde_t * ptrToPD = NULL;
int initialized = 0;
//globals (represent number of bits in virtual address used for each kind of indexing)
int offsetBitNum = 0;
int headerBitNum = 0;
int innerLevelBitNum = 0;
int outerLevelBitNum = 0;
int tlb_misses = 0;
int tlb_lookups = 0;

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    
    //To allocate physical memory we can allocate multiple struct pages
    //to add up to MEMSIZE

    setGlobals(); 

    physicalMemory = (struct page *)malloc(sizeof(struct page) * physicalPageNum);


    //printf("physical memory: physicalPageNum:%lld , pagesize:%d \n", physicalPageNum, sizeof(struct page));
    //printf("physical memory: basePtr:%p , baseEndPtr:%p \n", physicalMemory, &physicalMemory[physicalPageNum-1]);
    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

    pBitmap = (char *)malloc(physicalPageNum/8);
    
    for(int i = 0; i < sizeof(pBitmap)/sizeof(pBitmap[0]); i ++){
        pBitmap[i] = NUL;
    }
    //calculte the total # of page table entries

    //number of page table entries = number of virtual pages
    
    int numOfPTE = virtualPageNum;

    //There are 8 bits in 1 byte
    vBitmap = (char *)malloc(numOfPTE/8);

    for(int i = 0; i < sizeof(vBitmap)/sizeof(vBitmap[0]); i ++){
        vBitmap[i] = NUL;
    }

    settingUpPT();
    
    

}

void setGlobals(){

	
    //Virtual address is always 32 bits.

    //Splitting these 32 bits to get # of offet bits, inner level indexing bits, and outer level indexing bits. 

    //# of offset bits: log_2(PGSIZE)
    offsetBitNum = log2(PGSIZE);

    //# of bits for the vpn
    headerBitNum = 32 - offsetBitNum;

    if ((PGSIZE) < 131072){

    	//# of bits used for second level = log2(size of page / size of pte)
    	innerLevelBitNum = log2((PGSIZE)/sizeof(pte_t));

    	//# of bits for the outer level are the remaining header bits
    	outerLevelBitNum = headerBitNum - innerLevelBitNum;

    }else{

	//when the page size is 128k = 128 * 1024 = 131072, then the splitting cannot occur the regular way
	//cause then there will be 0 bits left to index into page directory.
	//So, in this case we try to split the header bits evenly
	

	outerLevelBitNum = headerBitNum/2;
	innerLevelBitNum = headerBitNum - outerLevelBitNum;


    }

}


void settingUpPT(){

    	//Allocate last page of physical memory as the page directory
    	//Allocate second to last page of physical memory as the first page table
    	//and so on,

    	//last page of physical memory = page directory = physicalMemory[physicalPageNum-1]

    	//set the last bit in physical bitmap as one 
    	//index of last character in bitmap
	int index = (physicalPageNum) - 1;
    
    	//we need to set the last bit of the character in this ptr to 1
	set_bit_at_index(pBitmap, index);
    
    	//Now we need to assign physical pages for all page tables

    	//we need to calculate the total number of page tables
    	//number of page tables = number of entries in page directory
    	//number of entries in page directory = 2^outerLevelBitNum

   	//printf("outerLevelBitNum: %d\n", outerLevelBitNum);
    	//printf("innerLevelBitNum: %d\n", innerLevelBitNum);
    	//printf("offsetBitNum: %d\n", offsetBitNum);
    	//printf("headerBitNum: %d\n", headerBitNum);

	struct page * ptr = &physicalMemory[physicalPageNum - 1];
	unsigned long * p_array = ptr->array;
	for(int k = 0; k < pow(2, outerLevelBitNum); k++){
		*(p_array + k) = -1;

	}

}

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
//int add_TLB(void *va, void *pa)
int add_TLB(int vpn, int ppn)
{

    	/*Part 2 HINT: Add a virtual to physical page translation to the TLB */
	//unsigned long vpn = (unsigned int)va >> offsetBitNum;
		
	//unsigned long ppn = (unsigned int)pa >> offsetBitNum;

	//getting the set # to add this translation to
	int set = vpn % TLB_ENTRIES;

	tlb_store.array[set][0] = vpn;
	tlb_store.array[set][1] = ppn;

    	return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
int
check_TLB(void *va) {

    	/* Part 2: TLB lookup code here */
	
	unsigned long vpn = (unsigned int)va >> offsetBitNum;
	int set = vpn % TLB_ENTRIES;

	if (tlb_store.array[set][0] == vpn){

		unsigned long ppn = tlb_store.array[set][1];
		//pte_t * addr = (pte_t *)&physicalMemory[ppn];
		return ppn;

	}else{
		return -1;

	}

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    	double miss_rate = 0;	

    	/*Part 2 Code here to calculate and print the TLB miss rate*/

	miss_rate = (tlb_misses * 1.0)/(tlb_lookups * 1.0);


    	fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va) {
	/* Part 1 HINT: Get the Page directory index (1st level) Then get the
    	* 2nd-level-page table index using the virtual address.  Using the page
    	* directory index and page table index get the physical address.
    	*
    	* Part 2 HINT: Check the TLB before performing the translation. If
    	* translation exists, then you can return physical address from the TLB.
    	*/

	unsigned int VA = (unsigned int)va;
	
    	unsigned long outer_bits_mask = (1 << offsetBitNum);
    	outer_bits_mask = outer_bits_mask-1;

    	unsigned long offset = VA & outer_bits_mask;

	tlb_lookups ++;
    	//check in TlB. If present return that physical address
	int tlbpn = check_TLB(va);

	if (tlbpn != -1){

		char * TLBPA = (char *) &physicalMemory[tlbpn];
		TLBPA = TLBPA + offset;
		return (pte_t *) TLBPA;


	}
	

    	//use outer level bits to index into outer level
    	//to get outer level index, get the unsigned int corresponding to top 'outerLevelBitNum' bits of VA

    	int num_bits_to_prune = 32 - outerLevelBitNum; //32 assuming we are using 32-bit address 
    	unsigned long outerIndex = VA >> num_bits_to_prune;

    	//use inner level bits to index into inner level
    	//to get inner level index, get the unsigned int corresponding to middle 'innerLevelBitNum' bits of VA

    	//removing lower order bits
    	int roughVA = VA >> offsetBitNum;
    	outer_bits_mask = (1 << innerLevelBitNum);
    	outer_bits_mask = outer_bits_mask-1;

    	unsigned long innerIndex = roughVA & outer_bits_mask;

    	//Checking that this page table entry is vaid using bitmap
    	//The bit we need to check: (outerIndex * innerLevelBitNum^2) + innerIndex

    	int index = (outerIndex * pow(2,innerLevelBitNum)) + innerIndex;

	int bit = get_bit_at_index(vBitmap, index);


    	if(bit != 1){
		//This PTE is not valid
        	return NULL;
    	}

    	//if its valid we can walk through the page table to get the valid mapping

    	//Use outer and inner index to get the physical page number

    	pde_t * pdEntry = pgdir + outerIndex;

   	//This entry of page directory has the page number of the inner level page table
    	unsigned long pageNumOfInnerLevelPageTable = *pdEntry;

    	pte_t * addressOfInnerLevelPageTable = (pte_t *)&physicalMemory[pageNumOfInnerLevelPageTable];
    
    	pte_t * ptEntry = addressOfInnerLevelPageTable + innerIndex;

    	unsigned long pNum = *ptEntry;

    	pte_t * addressOfPhysicalPage = (pte_t *)&physicalMemory[pNum];

    	unsigned long physicalAddress = (unsigned long)((char *)addressOfPhysicalPage + offset);

	//add entry to tlb
	tlb_misses ++;
	//add_TLB(va, (void *)physicalAddress);
	add_TLB(roughVA, pNum);


    	return (pte_t *)physicalAddress;
 
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
page_map(pde_t *pgdir, void *va, void *pa)
{

   	/*HINT: Similar to translate(), find the page directory (1st level)
    	and page table (2nd-level) indices. If no mapping exists, set the
    	virtual to physical mapping */

    	//use outer level bits to index into outer level
    	//to get outer level index, get the unsigned int corresponding to top 'outerLevelBitNum' bits of VA

    	int num_bits_to_prune = 32 - outerLevelBitNum; //32 assuming we are using 32-bit address 
    
    	unsigned int VA = (unsigned int)va;
    	unsigned long outerIndex = VA >> num_bits_to_prune;


    	//use inner level bits to index into inner level
    	//to get inner level index, get the unsigned int corresponding to middle 'innerLevelBitNum' bits of VA

    	//removing lower order bits
	
    	unsigned long ppn = (unsigned int)pa >> offsetBitNum;
    	unsigned long roughVA = VA >> offsetBitNum;

	//check if this virtual to physical page translation is already in the

	int recievedPage = check_TLB(va);
	tlb_lookups ++;

	if(recievedPage == ppn){

		return 0;   
		
	}

	tlb_misses++;

    	unsigned long outer_bits_mask = (1 << innerLevelBitNum);
    	outer_bits_mask = outer_bits_mask-1;

    	unsigned long innerIndex = roughVA & outer_bits_mask;

    	pde_t * pdEntry = pgdir + outerIndex;

    	if (*pdEntry == -1){
		//the corresponding page table needs to be reserved
		int endPage = physicalPageNum-1;
		while(endPage >= 0){
			//get the bit at index 'end page' from pBitmap
			int bit = get_bit_at_index(pBitmap, endPage);
			if(bit == 0){
				//reserve this page
				//set bit as 1
				set_bit_at_index(pBitmap, endPage);
				//set this page as the page directory
				*pdEntry = endPage;
				break;
			}

			endPage --;
		}

	}
		
    	//This entry of page directory has the page number of the inner level page table
    	unsigned long pageNumOfInnerLevelPageTable = *pdEntry;
	
    	pte_t * addressOfInnerLevelPageTable = (pte_t *)&physicalMemory[pageNumOfInnerLevelPageTable];
    
    	pte_t * ptEntry = addressOfInnerLevelPageTable + innerIndex;

    	//we need to set this entry as the physical page number 
    	//The physical page  number corresponding to physical address pa would be all
    	//the header bits


    	*ptEntry = ppn;
	
	//add this vpn to ppn translation to tlb
	add_TLB(roughVA, ppn);	
    	return 1;
    
}

static int get_bit_at_index(char *bitmap, int index)
{
    //Same as example 3, get to the location in the character bitmap array
    char *region = ((char *) bitmap) + (index / 8);
    
    //Create a value mask that we are going to check if bit is set or not
    char bit = 1 << (index % 8);
    
    return (int)(*region >> (index % 8)) & 0x1;
}
static void set_bit_at_index(char *bitmap, int index)
{
    // We first find the location in the bitmap array where we want to set a bit
    // Because each character can store 8 bits, using the "index", we find which 
    // location in the character array should we set the bit to.
    char *region = ((char *) bitmap) + (index / 8);
    
    // Now, we cannot just write one bit, but we can only write one character. 
    // So, when we set the bit, we should not distrub other bits. 
    // So, we create a mask and OR with existing values
    char bit = 1 << (index % 8);

    // just set the bit to 1. NOTE: If we want to free a bit (*bitmap_region &= ~bit;)
    *region |= bit;
   
    return;
}
static void reset_bit_at_index(char *bitmap, int index)
{
    // We first find the location in the bitmap array where we want to set a bit
    // Because each character can store 8 bits, using the "index", we find which 
    // location in the character array should we set the bit to.
    char *region = ((char *) bitmap) + (index / 8);
    
    // Now, we cannot just write one bit, but we can only write one character. 
    // So, when we set the bit, we should not distrub other bits. 
    // So, we create a mask and OR with existing values
    char bit = 1 << (index % 8);

    // just set the bit to 1. NOTE: If we want to free a bit (*bitmap_region &= ~bit;)
    *region &= ~bit;
   
    return;
}

/*Function that gets the next available page
*/

unsigned long get_next_avail(int num_pages) {
 
	//Use virtual address bitmap to find the next free page
    	int firstPage = 0;
   	int lastPage = 0;

	//printf ("current BitMap[");
	//for(int i=0; i<128; i++)
	//	printf ("%d",get_bit_at_index(vBitmap,i));
	//printf("]\n");	
	//fflush(stdout);
	//printf("virtualPageNum: %d\n",virtualPageNum);
	int index = 0;
	while(index < virtualPageNum){


		int bit = get_bit_at_index(vBitmap,index);


        	if(bit == 0){
            		int temp = 1;
            		int index2 = index+1;

            		while(index2 < virtualPageNum && temp < num_pages){
				bit = get_bit_at_index(vBitmap,index2);
                		if(bit == 1){
                    			break;
                		}else{
                    			temp++;
                    			index2++;
                		}
            		}

            		if (temp == num_pages){
                		//we found continuous pages
                		//The virtual pages we should return are in the index and index+numPages-1
                		firstPage = index;
                		return firstPage;

            		}
			index = index2;
			continue;

        	}
        	index++;
	}

    	return -1;
	
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes) {

    	/* 
     	* HINT: If the physical memory is not yet initialized, then allocate and initialize.
     	*/

   	/* 
    	* HINT: If the page directory is not initialized, then initialize the
    	* page directory. Next, using get_next_avail(), check if there are free pages. If
    	* free pages are available, set the bitmaps and map a new page. Note, you will 
    	* have to mark which physical pages are used. 
    	*/
	//printf("a_malloc: %d\n",num_bytes);
	//fflush(stdout);

	pthread_mutex_lock(&mutex);
	
	if (initialized == 0){
		set_physical_mem();
		initialized = 1;
	}
	//get the number of pages we need to allocate
	//each page has PGSIZE number of bytes
	
	int numPages = num_bytes / (PGSIZE);
	int leftover = num_bytes % (PGSIZE);
	if (leftover > 0){
		//and extra page is needed to accomodate these extra bytes
		numPages += 1;

	}

	//printf("These are the number of pages I am looking for:%d \n", numPages);
    	int physicalPages[numPages];

    	int size = 0;
    	int index = 0;
	
    	while(size < numPages && index < physicalPageNum){
		int bit = get_bit_at_index(pBitmap, index);

        	if (bit == 0){
            		physicalPages[size] = index;
            		size++;
        	}
        	index++;

    	}

    	if(size < numPages){
		pthread_mutex_unlock(&mutex);
        	return NULL;
    	}

    	//check if there are numPages free and adjacent PTE's using virtual bit map
	//if not return NULL
	//printf("This is the first virtual Page that I got: %d\n", firstPage);
    	int firstPage = get_next_avail(numPages);

	//printf("This is the first virtual Page that I got from get next avail: %d\n", firstPage);
    	if (firstPage == -1){
		pthread_mutex_unlock(&mutex);
        	return NULL;
    	}

    	int lastPage = firstPage+numPages-1;

	//printf("This is the last virtual Page that I will allocate: %d\n", lastPage);

	//check if there are numPages free physical pages (dont have to be continuous) using physical bitmap
	//if not return NULL

    	size = 0;
	//Now, use setmap to map the physical pages to virtual pages
    	for(int i = firstPage; i <= lastPage; i++){

        	unsigned long tempVA = i << offsetBitNum;
        	unsigned long tempPA = physicalPages[size] << offsetBitNum;
      		//updating vBitmap 
		set_bit_at_index(vBitmap, i);

        	//updating pBitmap
		set_bit_at_index(pBitmap, physicalPages[size]);

        	size++;
        	page_map((pde_t *)(physicalMemory+physicalPageNum-1), (void *)tempVA, (void *)tempPA);


    	}

    	//The vpn is stored in firstPage
    	unsigned long va = firstPage << offsetBitNum;
	pthread_mutex_unlock(&mutex);

	return (void *)va;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {

    	/* Part 1: Free the page table entries starting from this virtual address
     	* (va). Also mark the pages free in the bitmap. Perform free only if the 
     	* memory from "va" to va+size is valid.
     	*
     	* Part 2: Also, remove the translation from the TLB
     	*/

	pthread_mutex_lock(&mutex);

    	unsigned long firstPage = (int)va >> offsetBitNum;
	int numPagesToFree = size/(PGSIZE);
	if (size % (PGSIZE) > 0){

		numPagesToFree ++;

	}

    	int test = 1;

    	//making sure memory from "va" to va+size is valid

    	for (unsigned long i = firstPage; i < firstPage + numPagesToFree; i++){
       
		int bit = get_bit_at_index(vBitmap, firstPage);

        	if(bit == 0){
            		test = 0;
            		break;
        	}

    	}


    	if(test == 0){
        	//Not valid
		pthread_mutex_unlock(&mutex);
        	return;
    	}

    	//Valid
    	for (unsigned long i = firstPage; i < firstPage + numPagesToFree; i++){

        	void * va = (void *)(firstPage << offsetBitNum);

        	pte_t pa = (pte_t)(translate((pde_t *)(physicalMemory+physicalPageNum-1), va));

        	unsigned long physicalPage = pa >> offsetBitNum;

        	//set bit at index i in vBitMap to 0
		reset_bit_at_index(vBitmap, i);
        
        	//set bit at index physicalPage in pBitMap to 0
		reset_bit_at_index(pBitmap, physicalPage);
        

    	}

	
	//Remove the virtual pages form tlb
	for (unsigned long i = firstPage; i < firstPage + numPagesToFree; i++){

		//remove this vpn 
		int set = i % TLB_ENTRIES;
		if(tlb_store.array[set][0] == i){
			tlb_store.array[set][0] = -1;
			tlb_store.array[set][1] = -1;

		}
		
	}

	pthread_mutex_unlock(&mutex);

    
}

void printTLB(){

	for(int set=0; set<TLB_ENTRIES; set++)
	{
		printf("vitaual page: %lu, physical page: %lu\n", tlb_store.array[set][0], tlb_store.array[set][1]); 
	}


}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void put_value(void *va, void *val, int size) {

    	/* HINT: Using the virtual address and translate(), find the physical page. Copy
     	* the contents of "val" to a physical page. NOTE: The "size" value can be larger 
     	* than one page. Therefore, you may have to find multiple pages using translate()
     	* function.
     	*/

	//checking validity of virtual pages
	pthread_mutex_lock(&mutex);
	

    	char * VAL = (char *)val;
    	pte_t pa = (pte_t)(translate((pde_t *)(physicalMemory+physicalPageNum-1), va));
    	char * PA = (char *)pa;
    	char * VA = (char *) va;
	
	char * VAF = VA + size;
	//checking for the validity of the virtual pages from va to va+size

		
	unsigned int vpn1 = (unsigned int)VA >> offsetBitNum;
	unsigned int vpn2 = (unsigned int)VAF >> offsetBitNum;

	for(unsigned int j = vpn1; j <= vpn2; j++){
		//get the bit at index 'vpn' form vbitmap
		int bit = get_bit_at_index(vBitmap, j);

		if (bit == 0){
			//not a valid virtual address
			pthread_mutex_unlock(&mutex);
			return;
		}

	}

	//at this point we know all virtual addresses from va to va+size are valid

    	int tempsize = 0;
    	while(tempsize < size){


        	*PA = *VAL;
        	PA++;
        	VAL++;
        	tempsize++;
        	VA++;

        	unsigned int vai = (unsigned int)VA;
		
        	int outer_bits_mask = (1 << offsetBitNum);
        	outer_bits_mask = outer_bits_mask-1;

        	int offset = vai & outer_bits_mask;

        	if (offset == 0){
			
			//update the physical page
            		pa = (pte_t)(translate((pde_t *)(physicalMemory+physicalPageNum-1), (void *)VA));
			PA = (char *) pa;

        	}

    	}

	pthread_mutex_unlock(&mutex);

}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    	/* HINT: put the values pointed to by "va" inside the physical memory at given
    	* "val" address. Assume you can access "val" directly by derefencing them.
    	*/

	pthread_mutex_lock(&mutex);
	
    	char * pa = (char *)translate((pde_t *)(physicalMemory+physicalPageNum-1), va);
    	char * VA = (char *) va;
	char * valT = (char *) val;
	char * VAF = VA + size;
	//checking for the validity of the virtual pages from va to va+size

		
	unsigned int vpn1 = (unsigned int)VA >> offsetBitNum;
	unsigned int vpn2 = (unsigned int)VAF >> offsetBitNum;

	for(unsigned int j = vpn1; j <= vpn2; j++){
		//get the bit at index 'vpn' form vbitmap
		int bit = get_bit_at_index(vBitmap, j);

		if (bit == 0){
			//not a valid virtual address
			pthread_mutex_unlock(&mutex);
			return;
		}

	}

	//at this point we know that all virtual page numbers from va to va + size are valid.

	for (int i = 0; i < size; i++){

		*valT = *pa;
		valT++;
		pa++;
		VA++;

		
        	unsigned int vai = (unsigned int)VA;
		
        	int outer_bits_mask = (1 << offsetBitNum);
        	outer_bits_mask = outer_bits_mask-1;

        	int offset = vai & outer_bits_mask;

        	if (offset == 0){
			//update the physical page
            		pa = (char *)(translate((pde_t *)(physicalMemory+physicalPageNum-1), (void *)VA));	

        	}

	}

	pthread_mutex_unlock(&mutex);
}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */

    //A[i][j] = A[(i * size_of_rows * value_size) + (j * value_size)]

    for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){

            int result = 0;

            for (int k = 0; k < size; k++){

                //result += mat1[i][k] * mat2[k][j] 
                int val1 = 0;
                int val2 = 0;

                int address_1 = (unsigned int)mat1 + ((i * size * sizeof(int))) + (k * sizeof(int));
                int address_2 = (unsigned int)mat2 + ((k * size * sizeof(int))) + (j * sizeof(int));
                get_value((void *)address_1, &val1, sizeof(int));
                get_value( (void *)address_2, &val2, sizeof(int));
		
		
                result += val1 * val2;

            }

            int address_answer = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int));
            put_value((void *)address_answer, &result, sizeof(int));

        }

    }
          
}



