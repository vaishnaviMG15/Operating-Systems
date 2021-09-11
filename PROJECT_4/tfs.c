/*
 *  Copyright (C) 2021 CS416 Rutgers CS
 *	Tiny File System
 *	File:	tfs.c
 *
 */

#define FUSE_USE_VERSION 26
#define F 0
#define D 1
#define RW 0
#define RWX 1

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"

pthread_mutex_t mutex;

static int tfs_unlink(const char *path);

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here

//Stores the actual super block struct information for reference
struct superblock sb;

//This is the pointer to block 0 (block containing superblock info) of virtual disk
void * sbPointer;

bitmap_t inodeBitmap;
bitmap_t dataBitmap;

//This is the number of inodes that fit into a single block of the inode region
int numberOfInodesPerBlock = BLOCK_SIZE/sizeof(struct inode);

//This is the number of dirents that fit into a single block of the data region
int numberOfDirentsPerBlock = BLOCK_SIZE/sizeof(struct dirent);

//Helper functions that I added:
void blockToDirents (int blockno);
void writeBitMap(bitmap_t bitmap, char c);
int checkIfNameExists(struct inode currDirectory, const char * fname);


//This function formats the block with block number 'blockno'
//The block will then hold an array of dirents
void blockToDirents (int blockno){

	struct dirent * array = (struct dirent *)malloc(numberOfDirentsPerBlock * sizeof(struct dirent));
	
	//bio_write(blockno, buffer);
	memset(array, '\0', numberOfDirentsPerBlock * sizeof(struct dirent));

	for (int i = 0; i < numberOfDirentsPerBlock; i ++){

		array[i].valid = 0;


	}

	void * buffer = malloc(BLOCK_SIZE);
	memset(buffer, '\0', BLOCK_SIZE);

	memcpy(buffer, (void *)array, numberOfDirentsPerBlock * sizeof(struct dirent));
	bio_write(blockno, buffer);

	//bio_write(blockno, (const void *)array); 

	free(array);

}

//Helper function to write bitmaps to the disk
void writeBitMap(bitmap_t bitmap, char c){

	void * buffer = malloc(BLOCK_SIZE);
	if(c == 'i'){
    	memcpy(buffer, (const void *) inodeBitmap, (MAX_INUM)/8);
    	bio_write(sb.i_bitmap_blk, (const void *)buffer);
	}else{
		memcpy(buffer, (const void *) dataBitmap, (MAX_DNUM)/8);
		bio_write(sb.d_bitmap_blk, (const void *)buffer);
	}
    
    free(buffer);

}


/* 
 * Get available inode number from bitmap
 *This function returns -1 if there is no inode remaining
 *
 */
int get_avail_ino() {

	// Step 1: Read inode bitmap from disk
	//We will do this directly from memory as specified in FAQ

	// Step 2: Traverse inode bitmap to find an available slot

	// Step 3: Update inode bitmap and write to disk

	int i;

	for(i = 0; i < MAX_INUM; i++){

		uint8_t bit = get_bitmap(inodeBitmap, i);

		if (bit == 0){
			//set this bit
			set_bitmap(inodeBitmap, i);

			//writing the bitmap to disk
			writeBitMap(inodeBitmap, 'i');

			//return index
			return i;
		}

	}

	 //At this point we know there is no available inode

	return -1;
}

/* 
 * Get available data block number from bitmap
 *
 * The returned value is the data block number, not the actual block number.
 * While using this value in the calling function we will work with the block:
 * sb.d_start_blk + i
 *
 * This function returns -1 if there is no data block remaining
 *
 */
int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	//We will do this directly from memory as specified in FAQ

	// Step 2: Traverse data block bitmap to find an available slot

	// Step 3: Update data block bitmap and write to disk

	int i;

	for(i = 0; i < MAX_DNUM; i++){

		uint8_t bit = get_bitmap(dataBitmap, i);

		if (bit == 0){
			//set this bit
			set_bitmap(dataBitmap, i);

			//write to disk
			writeBitMap(dataBitmap, 'd');

			//return index
			return i;
		}

	}

	//At this point we know there is no available data block
	
	return -1;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {
	

  // Step 1: Get the inode's on-disk block number

	int blockNum = ino/numberOfInodesPerBlock;

	/*
	if (ino % numberOfInodesPerBlock == 0){
		blockNum ++;
	}
	*/


	//we need to go to the blockNum'th block of the inode area

	//int blockToRetrieve = sb.i_start_blk + blockNum - 1;
	int blockToRetrieve = sb.i_start_blk + blockNum;


  // Step 2: Get offset of the inode in the inode on-disk block

	//int offset = (ino % numberOfInodesPerBlock) - 1;
	int offset = (ino % numberOfInodesPerBlock) ;


  // Step 3: Read the block from disk and then copy into inode structure

	void * retrievedBlock = malloc(BLOCK_SIZE);

	bio_read(blockToRetrieve, retrievedBlock);



	struct inode * ptr;

	ptr = retrievedBlock;

	ptr = ptr + offset;

	memcpy((void *)inode, (const void *) ptr, sizeof(struct inode));


	free(retrievedBlock);
	return 0;

}

int writei(uint16_t ino, struct inode *inode) {


	// Step 1: Get the block number where this inode resides on disk
	int blockNum = ino/numberOfInodesPerBlock;

	/*if (ino % numberOfInodesPerBlock == 0){
		blockNum ++;
	}*/

	//we need to go to the blockNum'th block of the inode area

	//int blockToRetrieve = sb.i_start_blk + blockNum - 1;
	int blockToRetrieve = sb.i_start_blk + blockNum ;
	
	// Step 2: Get the offset in the block where this inode resides on disk

	//int offset = (ino % numberOfInodesPerBlock) - 1;
	int offset = (ino % numberOfInodesPerBlock);


	// Step 3: Write inode to disk 

	void * retrievedBlock = malloc(BLOCK_SIZE);

	bio_read(blockToRetrieve, retrievedBlock);

	struct inode * ptr;

	ptr = retrievedBlock;

	ptr = ptr + offset;

	memcpy((void *)ptr, (const void *) inode, sizeof(struct inode));

	bio_write(blockToRetrieve, retrievedBlock);


	free(retrievedBlock);
	return 0;

}


//returns 0 if the file already exists, else returns 1
int checkIfNameExists(struct inode currDirectory, const char * fname){

	int check = 1;

	int numOfDataBlocks = currDirectory.size;

	//loop through the datablocks
	for(int i = 0; i < numOfDataBlocks; i++){

		int dataBlock = currDirectory.direct_ptr[i];

		//bio read this data block into a buffer, so that
		//we can look through its dirents

		void * buffer = malloc(BLOCK_SIZE);
		bio_read(dataBlock, buffer);


		struct dirent * ptr = buffer; 

		//loop through the dirents of this block
		for (int j = 0; j < numberOfDirentsPerBlock; j++){


			if((ptr+j) -> valid == 1){

				char * currname = (ptr+j) -> name;

				//string comparison
				if (strcmp(currname, fname) == 0){
					//we found the file
					check = 0;
					j = numberOfDirentsPerBlock+1;
					i = numOfDataBlocks + 1;
				}
			}
		}

		free(buffer);

	}

	return check;

}

/* 
 *directory operations
 *This function returns 1 if there is a a file name and is copied to dirent.
 *Else the function returns 0
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)

	struct inode currDirectory;
	readi(ino, &currDirectory);

	//Now currDirectory has all the inode information for our directory

  // Step 2: Get data block of current directory from inode
  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure

	

	int numOfDataBlocks = currDirectory.size;


	//loop through the datablocks
	for(int i = 0; i < numOfDataBlocks; i++){

		int dataBlock = currDirectory.direct_ptr[i];


		//bio read this data block into a buffer, so that
		//we can look through its dirents

		void * buffer = malloc(BLOCK_SIZE);
		bio_read(dataBlock, buffer);

		struct dirent * ptr = buffer; 

		//loop through the dirents of this block
		for (int j = 0; j < numberOfDirentsPerBlock; j++){

			if((ptr+j) -> valid == 1){

				char * currname = (ptr+j) -> name;

				//string comparison
				if (strcmp(currname, fname) == 0){
					//we found the file
					//copying the dirent to the variable dirent
					memcpy((void *) dirent, (void *)(ptr + j), sizeof(struct dirent));
					free(buffer);
					return 1;
				}
			}
		}

		free(buffer);

	}

	return 0;
}

//returns -1 if the file already exists, else returns 0
int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {
	

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode

	int check = checkIfNameExists(dir_inode, fname);

	// Step 2: Check if fname (directory name) is already used in other entries
	if(check == 0){
		return -1;
	}

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry

	int checkValid = 0;

	int numOfDataBlocks = dir_inode.size;


	//loop through the datablocks
	for(int i = 0; i < numOfDataBlocks; i++){

		int dataBlock = dir_inode.direct_ptr[i];


		//bio read this data block into a buffer, so that
		//we can look through its dirents

		void * buffer = malloc(BLOCK_SIZE);
		bio_read(dataBlock, buffer);

		struct dirent * ptr = buffer; 

		//loop through the dirents of this block
		for (int j = 0; j < numberOfDirentsPerBlock; j++){

			if((ptr+j) -> valid == 0){

				checkValid = 1;

				//Write the directory entry at this location
				struct dirent * location = ptr+j;
				location -> ino = f_ino;
				location -> valid = 1;
				//location -> name = fname;
				strcpy(location->name, fname);
				location -> len = name_len;

				//write this buffer back to the disk
				bio_write(dataBlock, buffer);

				j = numberOfDirentsPerBlock+1;
				i = numOfDataBlocks + 1;

			}
		}

		free(buffer);

	}

	if(checkValid == 0){

		//a new block is required for this directory

		//get the next available block
		int newblockno = get_avail_blkno() + sb.d_start_blk;


		//covert this block into an array of dirents
		blockToDirents (newblockno);

		//update the direct_ptr & size of this directory's inode

		struct inode * newdirInode = (struct inode *)malloc(sizeof(struct inode)); 

		readi(dir_inode.ino, newdirInode);

		newdirInode -> size += 1;

		*((newdirInode -> direct_ptr) + numOfDataBlocks) = newblockno;

		writei(dir_inode.ino, newdirInode);


		//end of update

		void * buffer = malloc(BLOCK_SIZE);

		//newly adding
		bio_read(newblockno, buffer);
		struct dirent * location = buffer;

		//struct dirent * location = (struct dirent *)malloc(sizeof(struct dirent));
		location -> ino = f_ino;
		location -> valid = 1;
		//location -> name = fname;
		strcpy(location->name, fname);
		location -> len = name_len;

		//memcpy(buffer, (void *) location, sizeof(struct dirent));

		//free(location);

		bio_write(newblockno, buffer);

		free(buffer);

		free(newdirInode);


	}


	return 0;

}

//returns 1 if fname has been removed
//returns 0 of no fname

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {


	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	// Step 2: Check if fname exist
	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

	int numOfDataBlocks = dir_inode.size;

	for(int i = 0; i < numOfDataBlocks; i++){

		int dataBlock = dir_inode.direct_ptr[i];

		//bio read this data block into a buffer, so that
		//we can look through its dirents

		void * buffer = malloc(BLOCK_SIZE);
		bio_read(dataBlock, buffer);

		struct dirent * ptr = buffer; 

		//loop through the dirents of this block
		for (int j = 0; j < numberOfDirentsPerBlock; j++){

			if((ptr+j) -> valid == 1){

				char * currname = (ptr+j) -> name;

				//string comparison
				if (strcmp(currname, fname) == 0){
					//we found the file
					(ptr + j) -> valid = 0;
					
					bio_write(dataBlock, buffer);

					free(buffer);
					return 1;
				}
			}
		}

		free(buffer);

	}

	return 0;
}

/* 
 * namei operation
 * returns 0 if path is valid and the specific inode structure is found
 * else it returns -1.
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way

	/*
	/foo/bar/a.txt  -> /foo/bar and /a.txt

	/foo/bar -> /foo and /bar

	/foo -> /foo

	/ -> /
	*/

	char * path1 = strdup(path);
	char * path2 = strdup(path);
	

	char * baseName = basename(path1);
	char * parentName = dirname(path2);
	


	//base case: if baseName == NULL
	if(strcmp(baseName,"/")==0){
	//if (baseName.equals("/")){
		//The path is just the root
		readi(0, inode);
              	return 0;
	}


	struct dirent * dirent = (struct dirent *)malloc(sizeof(struct dirent));

	//recursive case
	struct inode * recursiveInode = (struct inode *)malloc(sizeof(struct inode));

        int a =  get_node_by_path(parentName, 0, recursiveInode);

	if (a == -1){
		//the path is invalid
		free(dirent);
		free(recursiveInode);
		return -1;

	}
	
	int b = dir_find(recursiveInode -> ino, baseName, strlen(baseName), dirent);


        free(recursiveInode);

	if (b == 0){
               //File does not exist (path is not valid)
	       free(dirent);
               return -1;

        }

	int inodeNum = dirent->ino;

        readi(inodeNum, inode);

        free(dirent);

    	return 0;

}

/* 
 * Make file system
 */
int tfs_mkfs() {

	//pthread_mutex_lock(&mutex);

	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);

	// write superblock information
	
	sb.magic_num = MAGIC_NUM;
	sb.max_inum = MAX_INUM;
	sb.max_dnum = MAX_DNUM;

	//starting block of inode bitmap = number of blocks required to store the superblock struct

	sb.i_bitmap_blk = sizeof(struct superblock)/BLOCK_SIZE;

	if (sizeof(struct superblock)%BLOCK_SIZE != 0){
		sb.i_bitmap_blk ++;
	}

	//Calculation for number of blocks required by inode bitmap:
	//((Maximum number of inodes)/8)/4k

	int numOfInodeBitMapBlocks = ((MAX_INUM)/8)/BLOCK_SIZE;

	if ((((MAX_INUM)/8)%BLOCK_SIZE) !=0){

			numOfInodeBitMapBlocks++;

	}

	sb.d_bitmap_blk = sb.i_bitmap_blk + numOfInodeBitMapBlocks;

	int numOfDataBitMapBlocks = ((MAX_DNUM)/8)/BLOCK_SIZE;

	if ((((MAX_DNUM)/8)%BLOCK_SIZE) !=0){

			numOfDataBitMapBlocks++;

	}

	sb.i_start_blk = sb.d_bitmap_blk + numOfDataBitMapBlocks;

	int numOfInodeBlocks = (sizeof(struct inode) * MAX_INUM)/BLOCK_SIZE;

	if ((sizeof(struct inode) * MAX_INUM)%BLOCK_SIZE !=0){

			numOfInodeBlocks++;

	}

	sb.d_start_blk = sb.i_start_blk + numOfInodeBlocks;

	//sbPointer = &sb;

	memcpy((void *)sbPointer, (const void *) &sb, sizeof(struct superblock));


	bio_write(0, (const void *) sbPointer);


	// initialize inode bitmap

	for(int i = 0; i < sizeof(inodeBitmap)/sizeof(inodeBitmap[0]); i ++){
        	inodeBitmap[i] = '\0';
	}

    	writeBitMap(inodeBitmap, 'i');


	// initialize data block bitmap

	for(int i = 0; i < sizeof(dataBitmap)/sizeof(dataBitmap[0]); i ++){
		dataBitmap[i] = '\0';
	}

	writeBitMap(dataBitmap, 'd');
	


	// update bitmap information for root directory

	//root directory would get the first inode number
	set_bitmap(inodeBitmap, 0);
	writeBitMap(inodeBitmap, 'i');

	//we could reserve the first data block for the root directory
	set_bitmap(dataBitmap, 0);
	writeBitMap(dataBitmap, 'd');

	//Format data block 0 into an array of dirents
	//blockToDirents(0);
	blockToDirents (sb.d_start_blk);

	// update inode for root directory
	struct inode rd;

	rd.ino = 0;
	rd.valid = 1;
	rd.size = 1;
	rd.type = D;
	rd.link = 2;
	rd.direct_ptr[0] = sb.d_start_blk;
	(rd.vstat).st_atime = time(NULL);
	(rd.vstat).st_mtime = time(NULL);
	(rd.vstat).st_mode = S_IFDIR | 0755;

	void * buffer = malloc(BLOCK_SIZE);
	bio_read(sb.i_start_blk, buffer);
	//struct inode * ptr = buffer;
	//memcpy((void *)ptr, (const void *) &rd, sizeof(struct inode));
	memcpy(buffer, (const void *) &rd, sizeof(struct inode));
	bio_write(sb.i_start_blk, (const void *)buffer);
	free(buffer);

	//add the dirent for '.'
	const char * filename = ".";
	dir_add(rd, 0, filename, 1);


	//pthread_mutex_unlock(&mutex);
	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {

	pthread_mutex_lock(&mutex);
	// Step 1a: If disk file is not found, call mkfs
	if(dev_open(diskfile_path) == -1){

		sbPointer = malloc(BLOCK_SIZE);
		inodeBitmap = (bitmap_t) malloc((MAX_INUM)/8);
		dataBitmap = (bitmap_t) malloc((MAX_DNUM)/8);
		tfs_mkfs();

	}else{

  // Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk
  
	sbPointer = malloc(BLOCK_SIZE);
	inodeBitmap = (bitmap_t) malloc((MAX_INUM)/8);
	dataBitmap = (bitmap_t) malloc((MAX_DNUM)/8);

	bio_read(0, sbPointer);

	//extra code added
	
	struct superblock * ptr = sbPointer;

	sb = *ptr;

	//read the inode bitmap from the disk
	void * buffer = malloc(BLOCK_SIZE);

	bio_read(sb.i_bitmap_blk,buffer);

	memcpy((void *)inodeBitmap, buffer, MAX_INUM/8);

	free(buffer);

	//read the data bitmap from the disk
	
	buffer = malloc(BLOCK_SIZE);

        bio_read(sb.d_bitmap_blk,buffer);

        memcpy((void *)dataBitmap, buffer, MAX_DNUM/8);

        free(buffer);

	}

	pthread_mutex_unlock(&mutex);
	return NULL;

}

static void tfs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
	pthread_mutex_lock(&mutex);

	//bio_write(0, (const void *) sbPointer);
	free(sbPointer);
	free(inodeBitmap);
	free(dataBitmap);

	// Step 2: Close diskfile

	dev_close();
	pthread_mutex_unlock(&mutex);
	

}

static int tfs_getattr(const char *path, struct stat *stbuf) {

	pthread_mutex_lock(&mutex);
	// Step 1: call get_node_by_path() to get inode from path
	struct inode * pathsInode = (struct inode *)malloc(sizeof(struct inode));

	int check = get_node_by_path(path, 0, pathsInode);

	if (check == -1){
		//path is not valid
		free(pathsInode);
		pthread_mutex_unlock(&mutex);
		return -ENOENT;
	}


	// Step 2: fill attribute of file into stbuf from inode
	if(pathsInode -> type == F){
		stbuf->st_mode   = S_IFREG | 0755;
	}else{
		stbuf->st_mode   = S_IFDIR | 0755;
	}
	stbuf->st_nlink  = 2;
	time(&stbuf->st_mtime);

	stbuf -> st_uid = getuid();
	stbuf -> st_gid = getgid();
	stbuf -> st_size = (pathsInode -> size) * BLOCK_SIZE;
	//stbuf -> st_mode = (pathsInode -> vstat).st_mode;

	free(pathsInode);

	pthread_mutex_unlock(&mutex);
	return 0;

}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {
	pthread_mutex_lock(&mutex);

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode * pathsInode = (struct inode *)malloc(sizeof(struct inode));

	int check = get_node_by_path(path, 0, pathsInode);

	// Step 2: If not find, return -1

	if (check == -1){
		//path is not valid
		free(pathsInode);
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	free(pathsInode);
    pthread_mutex_unlock(&mutex);
    return 0;

}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	
	pthread_mutex_lock(&mutex);	

	struct inode * pathsInode = (struct inode *)malloc(sizeof(struct inode));

	int check = get_node_by_path(path, 0, pathsInode);


	if (check == -1){
		//path is not valid
		free(pathsInode);
		pthread_mutex_unlock(&mutex);
		return -1;
	}


	//paths inode is the inode of the directory whose directory entries we need to copy

	// Step 2: Read directory entries from its data blocks, and copy them to filler

	//filler(buffer, fname, NULL, 0)

	int numOfDataBlocks = pathsInode -> size;

	//loop through the datablocks
	for(int i = 0; i < numOfDataBlocks; i++){

		int dataBlock = *((pathsInode -> direct_ptr) + i);

		//bio read this data block into a buffer, so that
		//we can look through its dirents

		void * bufferT = malloc(BLOCK_SIZE);
		bio_read(dataBlock, bufferT);

		struct dirent * ptr = bufferT;

		//loop through the dirents of this block
		for (int j = 0; j < numberOfDirentsPerBlock; j++){

			if((ptr+j) -> valid == 1){

				char * currname = (ptr+j) -> name;

				filler(buffer, currname, NULL, 0);

			}
		}

		free(bufferT);

	}

	free(pathsInode);

	pthread_mutex_unlock(&mutex);
	return 0;

}


static int tfs_mkdir(const char *path, mode_t mode) {
	pthread_mutex_lock(&mutex);
	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name



	char * path1 = strdup(path);
	char * path2 = strdup(path);
	char * directoryName = dirname(path1) ;
	char * baseName = basename(path2);
	
	// Step 2: Call get_node_by_path() to get inode of parent directory

	struct inode * pathsInode = (struct inode *)malloc(sizeof(struct inode));

	int check1 = get_node_by_path(directoryName, 0, pathsInode);

	if (check1 == -1){
		//path is not valid
		pthread_mutex_unlock(&mutex);
		return 0;
	}


	// Step 3: Call get_avail_ino() to get an available inode number

	int check2 = get_avail_ino();

	if(check2 == -1){
		pthread_mutex_unlock(&mutex);
		//no space for file
		return 0;
	}


	// Step 4: Call dir_add() to add directory entry of target directory to parent directory
	

	int check3 = dir_add(*pathsInode, check2, baseName, strlen(baseName));

	if(check3 == -1){
		//already exists
		unset_bitmap(inodeBitmap, check2);
		writeBitMap(inodeBitmap, 'i');
		pthread_mutex_unlock(&mutex);
		return -1;
	}


	// Step 5: Update inode for target directory
	// Step 6: Call writei() to write inode to disk

	//updating directiry name's inode to update its link count
	readi(pathsInode->ino, pathsInode);
	pathsInode -> link += 1;
	writei(pathsInode -> ino, pathsInode);

	//creating a new inode structure for the base directory

	struct inode * newInode = (struct inode *)malloc(sizeof(struct inode));

	newInode -> ino = check2;
	newInode -> valid = 1;
	newInode -> size = 0;
	newInode -> type = D;
	newInode -> link = 2;
	//newInode -> direct_ptr = NULL;
	time(&(newInode -> vstat).st_mtime);
	(newInode -> vstat).st_mode = RW;
	writei(check2, newInode);

	//creating 2 directory entries '.' and '..' for the base file
	dir_add(*newInode, check2, (const char *) ".", 1);
	readi(newInode->ino, newInode);
	dir_add(*newInode, pathsInode -> ino, (const char *)"..", 2);

	free(pathsInode);
	free(newInode);
	pthread_mutex_unlock(&mutex);
	return 0;

}

//returns -1 if path is not valid
static int tfs_rmdir(const char *path) {
	pthread_mutex_lock(&mutex);
	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name


	char * path1 = strdup(path);
	char * path2 = strdup(path);
	
	char * directoryName = dirname(path1);
        char * baseName = basename(path2);
	

	// Step 2: Call get_node_by_path() to get inode of target directory
	struct inode * pathsInode = (struct inode *)malloc(sizeof(struct inode));
	int check = get_node_by_path((char *) path, 0, pathsInode);
	if (check == -1){
		//path is not valid
		pthread_mutex_unlock(&mutex);
		return -1;
	}



	//traverse through all directory entries of the target directory
	
	int numOfDataBlocks = pathsInode -> size;
	//loop through the datablocks
	for(int i = 0; i < numOfDataBlocks; i++){

		//int dataBlock = currDirectory.direct_ptr[i];

		int dataBlock = *((pathsInode->direct_ptr)+i);

		//bio read this data block into a buffer, so that
		//we can look through its dirents

		void * buffer = malloc(BLOCK_SIZE);
		bio_read(dataBlock, buffer);

		struct dirent * ptr = buffer; 

		//loop through the dirents of this block
		for (int j = 0; j < numberOfDirentsPerBlock; j++){

			if((ptr+j) -> valid == 1){

				if(strcmp((ptr + j)->name, ".") == 0 || strcmp((ptr + j)->name, "..") == 0 ){
					continue;
				}
				else
				{

					free(buffer);
					//free(parentsInode);
        				free(pathsInode);
					//return error code
					pthread_mutex_unlock(&mutex);
					return -ENOTEMPTY;

				}

				/*
				int inodeNumOfDirent = (ptr + j)->ino;
				struct inode * direntsInode = (struct inode *)malloc(sizeof(struct inode));
				readi(inodeNumOfDirent, direntsInode);
				int type = direntsInode -> type;
				char * newPath = ""; 
				strcat(newPath, path);
				strcat(newPath, "/");
				strcat(newPath, (ptr+j)->name);
				if (type == 'D'){
					tfs_rmdir(newPath);
					
				}else{

					tfs_unlink((const char *)newPath);
				}
				*/
			}
		}

		free(buffer);

	}


	// Step 3: Clear data block bitmap of target directory
        //go the direc_ptr and traverse through all block numbers
        //free these blocks in the data bitmap

        int * ptr = pathsInode -> direct_ptr;

        for(int i = 0; i < pathsInode -> size; i ++){

                int * curr = ptr + i;

                int blockno = *curr;

		//memsetting this blocknumber back 
		blockToDirents(blockno);

                int index = blockno - sb.d_start_blk;

                unset_bitmap(dataBitmap, index);


        }

        // Step 4: Clear inode bitmap and its data block

        int index2 = pathsInode -> ino;

        unset_bitmap(inodeBitmap, index2);

        //write these bitmaps to disk
        writeBitMap(inodeBitmap, 'i');
        writeBitMap(dataBitmap, 'd');

	// Step 5: Call get_node_by_path() to get inode of parent directory
	struct inode * parentsInode = (struct inode *)malloc(sizeof(struct inode));
        get_node_by_path(directoryName, 0, parentsInode);
	//int parentsIno = parentsInode -> ino;

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory
	dir_remove(*parentsInode, baseName, strlen(baseName));

	free(parentsInode);
	free(pathsInode);
	pthread_mutex_unlock(&mutex);
	return 0;

}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	pthread_mutex_lock(&mutex);

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char * path1 = strdup(path);
	char * path2 = strdup(path);
	
	char * directoryName = dirname(path1);
	char * baseName = basename(path2);
	


	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode * pathsInode = (struct inode *)malloc(sizeof(struct inode));

	int check1 = get_node_by_path(directoryName, 0, pathsInode);

	if (check1 == -1){
		//path is not valid
		pthread_mutex_unlock(&mutex);
		return 0;
	}


	// Step 3: Call get_avail_ino() to get an available inode number

	int check2 = get_avail_ino();

	if(check2 == -1){
		//no space for file
		pthread_mutex_unlock(&mutex);
		return 0;
	}


	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	int check3 = dir_add(*pathsInode, check2, baseName, strlen(baseName));

	if(check3 == -1){
		//already exists
		pthread_mutex_unlock(&mutex);
		return -1;
	}

	// Step 5: Update inode for target file
	struct inode * newInode = (struct inode *)malloc(sizeof(struct inode));

	newInode -> ino = check2;
	newInode -> valid = 1;
	newInode -> size = 0;
	newInode -> type = F;
	newInode -> link = 1;
	//newInode -> direct_ptr = NULL;
	time(&(newInode -> vstat).st_mtime);
	(newInode -> vstat).st_mode = mode;
	writei(check2, newInode);

	// Step 6: Call writei() to write inode to disk

	free(pathsInode);
	free(newInode);
	
	//void * buff = malloc(BLOCK_SIZE);
	//bio_read(0, buff);
	//struct superblock * ptr = buff;



	pthread_mutex_unlock(&mutex);
	return 0;
}

//returns -1 if path is not valid, else returns 1 and succeffully reads the inode
static int tfs_open(const char *path, struct fuse_file_info *fi) {
	pthread_mutex_lock(&mutex);

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode * pathsInode = (struct inode *)malloc(sizeof(struct inode));

	int check = get_node_by_path(path, 0, pathsInode);

	// Step 2: If not find, return -1

	if (check == -1){
		//path is not valid
		free(pathsInode);
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	free(pathsInode);
	pthread_mutex_unlock(&mutex);
	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	pthread_mutex_lock(&mutex);

	int sizeT = size;
	// Step 1: You could call get_node_by_path() to get inode from path
	struct inode * pathsInode = (struct inode *)malloc(sizeof(struct inode));

        int check = get_node_by_path(path, 0, pathsInode);

        if (check == -1){
                //path is not valid
                free(pathsInode);
		pthread_mutex_unlock(&mutex);
                return 0;
        }

	int fileSize = (pathsInode -> size) * BLOCK_SIZE;
	//check
	if((offset > fileSize) || (size > fileSize)){
		pthread_mutex_unlock(&mutex);
		return 0;

	}

	// Step 2: Based on size and offset, read its data blocks from disk
	
	int blockIndex = offset/BLOCK_SIZE;
	int blockToStart = *((pathsInode -> direct_ptr) + blockIndex);
	int locationToStart = offset % BLOCK_SIZE;

	if ((fileSize - offset < size)){
		pthread_mutex_unlock(&mutex);
		return 0;

	}

	// Step 3: copy the correct amount of data from offset to buffer
	char * bufferT = (char *)buffer;
	
	while(size > 0){

		int remaining = BLOCK_SIZE - locationToStart;

		void * buffT = malloc(BLOCK_SIZE);

		bio_read(blockToStart, buffT);

		char * buffTemp = (char *)buffT;

		char * location = buffTemp + locationToStart;
		
		if (size <= remaining){
			memcpy((void *)bufferT, (void *)location, size);
			free(buffT);
			break;
		}

		memcpy((void *)bufferT, (void *)location, remaining);
		bufferT += remaining;
		size = size - remaining;
		locationToStart = 0;
		blockIndex ++;
		blockToStart = *((pathsInode -> direct_ptr) + blockIndex); 
		free(buffT);	
	}
	
	free(pathsInode);
	// Note: this function should return the amount of bytes you copied to buffer
	pthread_mutex_unlock(&mutex);
	return sizeT;
}


static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path
	pthread_mutex_lock(&mutex);

	int sizeT = size;
	struct inode * pathsInode = (struct inode *)malloc(sizeof(struct inode));

        int check = get_node_by_path(path, 0, pathsInode);


        if (check == -1){
                //path is not valid
                free(pathsInode);
		pthread_mutex_unlock(&mutex);
                return 0;
        }


	
	 //int fileSize = (pathsInode -> size) * BLOCK_SIZE;
        //check
	/*
        if((offset > fileSize) || (size > fileSize)){

                return 0;

        }

	if ((fileSize - offset < size)){

                return 0;

        }
	*/

	// Step 2: Based on size and offset, read its data blocks from disk
	
	int blockIndex = offset/BLOCK_SIZE;
        int locationToStart = offset % BLOCK_SIZE;


	//getting the extra data blocks needed for this operation
	int currNumOfBlocks = pathsInode->size;

	//Total file size for this operation:
	int totalFileSize = (blockIndex * BLOCK_SIZE) + locationToStart + size;
	//total number of blocks needed for this operation
	int numOfBlocksTotal = totalFileSize / BLOCK_SIZE;
	

	if (totalFileSize % BLOCK_SIZE != 0){
		numOfBlocksTotal ++;
	}

	int diff = numOfBlocksTotal - currNumOfBlocks;


	if (diff > 0){

		//we need to add extra blocks
		for (int index = 0; index < diff; index ++){

			int d = get_avail_blkno();
			if (d == -1){
				break;
			}
			*((pathsInode->direct_ptr) + currNumOfBlocks + index) = d + sb.d_start_blk;
			pathsInode ->size ++;
		}

	}

	if (pathsInode->size == 0){
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	if ((pathsInode -> size) < blockIndex + 1){
		pthread_mutex_unlock(&mutex);
		return 0 ;
	}

	// Step 3: Write the correct amount of data from offset to disk

	int blockToStart = *((pathsInode -> direct_ptr) + blockIndex);
	char * bufferT = (char *)buffer;

        while(size > 0){

                int remaining = BLOCK_SIZE - locationToStart;

                void * buffT = malloc(BLOCK_SIZE);

                bio_read(blockToStart, buffT);

                char * buffTemp = (char *)buffT;

                char * location = buffTemp + locationToStart;

                if (size <= remaining){
                        memcpy((void *)location, (void *)bufferT, size);
			bio_write(blockToStart,buffT);
                        free(buffT);
                        break;
                }

                memcpy((void *)location, (void *)bufferT, remaining);
		bio_write(blockToStart, buffT);
                bufferT += remaining;
                size = size - remaining;
                locationToStart = 0;
                blockIndex ++;
		if((pathsInode -> size) < blockIndex + 1){
			time(&(pathsInode -> vstat).st_mtime);
			writei(pathsInode -> ino, pathsInode);
			pthread_mutex_unlock(&mutex);
			return sizeT - size;
				
		}
                blockToStart = *((pathsInode -> direct_ptr) + blockIndex);
                free(buffT);
        }

	// Step 4: Update the inode info and write it to disk
	time(&(pathsInode -> vstat).st_mtime);


	writei(pathsInode -> ino, pathsInode);
	// Note: this function should return the amount of bytes you write to disk
	
	free(pathsInode);
	pthread_mutex_unlock(&mutex);
	return sizeT;
}

static int tfs_unlink(const char *path) {
	pthread_mutex_lock(&mutex);

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char * path1 = strdup(path);
	char * path2 = strdup(path);
	
	char * directoryName = dirname(path1);
	char * baseName = basename(path2);
	

	// Step 2: Call get_node_by_path() to get inode of target file
	struct inode * targetsInode = (struct inode *)malloc(sizeof(struct inode));

        int check = get_node_by_path(path, 0, targetsInode);

        if (check == -1){
                //path is not valid
                free(targetsInode);
		pthread_mutex_unlock(&mutex);
                return 0;
        }

	// Step 3: Clear data block bitmap of target file
	for(int i = 0; i < targetsInode -> size; i++){
		
		int blockToRem = *((targetsInode -> direct_ptr) + i);
		int dataIndex = blockToRem - sb.d_start_blk;
		unset_bitmap(dataBitmap, dataIndex);

	}


	// Step 4: Clear inode bitmap and its data block
	unset_bitmap(inodeBitmap, targetsInode -> ino);
	writeBitMap(inodeBitmap, 'i');
	writeBitMap(dataBitmap, 'd');

	targetsInode -> valid = 0;
	writei(targetsInode -> ino, targetsInode);

	// Step 5: Call get_node_by_path() to get inode of parent directory
	struct inode * parentsInode = (struct inode *)malloc(sizeof(struct inode));
	get_node_by_path(directoryName, 0, parentsInode);
	
	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory
	dir_remove(*parentsInode, baseName, strlen(baseName));
	pthread_mutex_unlock(&mutex);
	return 0;
}

static int tfs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations tfs_ope = {
	.init		= tfs_init,
	.destroy	= tfs_destroy,

	.getattr	= tfs_getattr,
	.readdir	= tfs_readdir,
	.opendir	= tfs_opendir,
	.releasedir	= tfs_releasedir,
	.mkdir		= tfs_mkdir,
	.rmdir		= tfs_rmdir,

	.create		= tfs_create,
	.open		= tfs_open,
	.read 		= tfs_read,
	.write		= tfs_write,
	.unlink		= tfs_unlink,

	.truncate   = tfs_truncate,
	.flush      = tfs_flush,
	.utimens    = tfs_utimens,
	.release	= tfs_release
};


int main(int argc, char *argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	//return 0;
	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);
	return fuse_stat;
}

