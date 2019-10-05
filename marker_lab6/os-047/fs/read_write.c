#include "testfs.h"
#include "list.h"
#include "super.h"
#include "block.h"
#include "inode.h"

/* given logical block number, read the corresponding physical block into block.
 * return physical block number.
 * returns 0 if physical block does not exist.
 * returns negative value on other errors. */

static int
testfs_read_block(struct inode *in, int log_block_nr, char *block)
{
	int phy_block_nr = 0;
	assert(log_block_nr >= 0);
	if (log_block_nr >= NR_DIRECT_BLOCKS + NR_INDIRECT_BLOCKS + (NR_INDIRECT_BLOCKS * NR_INDIRECT_BLOCKS))
	    return -EFBIG;

	if (log_block_nr < NR_DIRECT_BLOCKS) {
		phy_block_nr = (int)in->in.i_block_nr[log_block_nr]; // we're linking it directly to a direct block
	} else {
		log_block_nr -= NR_DIRECT_BLOCKS;

		if (log_block_nr >= NR_INDIRECT_BLOCKS && in->in.i_dindirect > 0){ // THIS IS FOR DOUBLY INDIRECT BLOCKS ****************************************

				read_blocks(in->sb, block, in->in.i_dindirect,1);
				log_block_nr -= NR_INDIRECT_BLOCKS;
				if(((int *)block)[log_block_nr/NR_INDIRECT_BLOCKS] == 0){
					return 0;
				}
				read_blocks(in->sb, block, ((int *)block)[log_block_nr/NR_INDIRECT_BLOCKS], 1);
				phy_block_nr = ((int *)block)[log_block_nr % NR_INDIRECT_BLOCKS];
				
		}// *******************************************************************************************************************
		if (in->in.i_dindirect <= 0 && in->in.i_indirect > 0) {
				read_blocks(in->sb, block, in->in.i_indirect, 1);
				phy_block_nr = ((int *)block)[log_block_nr];
			
		}
	}
	if (phy_block_nr > 0) {
		read_blocks(in->sb, block, phy_block_nr, 1);
	} else {
		/* we support sparse files by zeroing out a block that is not
		 * allocated on disk. */
		bzero(block, BLOCK_SIZE);
	}
	return phy_block_nr;
}


int
testfs_read_data(struct inode *in, char *buf, off_t start, size_t size)
{
	char block[BLOCK_SIZE];
	long block_nr = start / BLOCK_SIZE; // logical block number in the file
	long block_ix = start % BLOCK_SIZE; //  index or offset in the block
	int ret;
	int written = 0;
	//size_t remaining_size;
	assert(buf);
	if (start + (off_t) size > in->in.i_size) {
		size = in->in.i_size - start;
	}
	//remaining_size = size;
	if (block_ix + size > BLOCK_SIZE) {
		//remaining_size -= (BLOCK_SIZE - block_ix);
		if ((ret = testfs_read_block(in, block_nr, block)) < 0)
			return ret;
		memcpy(buf, block + block_ix, BLOCK_SIZE - block_ix);
		written += (BLOCK_SIZE - block_ix);
		block_nr++;
		block_ix = 0;
		while(size - written > BLOCK_SIZE){
			//remaining_size -= BLOCK_SIZE;
			if ((ret = testfs_read_block(in, block_nr, block)) < 0)
				return ret;
			memcpy(buf + written, block + block_ix, BLOCK_SIZE);
			written += BLOCK_SIZE;
			block_nr++;
			block_ix = 0;
		}
		
	}
	if ((ret = testfs_read_block(in, block_nr, block)) < 0)
		return ret;
	memcpy(buf + written, block + block_ix, size - written);
	/* return the number of bytes read or any error */
	return size;
}

/* given logical block number, allocate a new physical block, if it does not
 * exist already, and return the physical block number that is allocated.
 * returns negative value on error. */
static int
testfs_allocate_block(struct inode *in, int log_block_nr, char *block)
{
	int phy_block_nr;
	char indirect[BLOCK_SIZE];
	char dindirect[BLOCK_SIZE];
	int indirect_allocated = 0;
	int dindirect_allocated = 0;
	assert(log_block_nr >= 0);
	phy_block_nr = testfs_read_block(in, log_block_nr, block);

	/* phy_block_nr > 0: block exists, so we don't need to allocate it, 
	   phy_block_nr < 0: some error */
	if (phy_block_nr != 0)
		return phy_block_nr;

	/* allocate a direct block */
	if (log_block_nr < NR_DIRECT_BLOCKS) {
		assert(in->in.i_block_nr[log_block_nr] == 0);
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if (phy_block_nr >= 0) {
			in->in.i_block_nr[log_block_nr] = phy_block_nr; //storing which physical block is associated with logical block
		}
		return phy_block_nr;
	}

	log_block_nr -= NR_DIRECT_BLOCKS;
	if (log_block_nr >= NR_INDIRECT_BLOCKS){// ********************************************************************************
		log_block_nr -= NR_INDIRECT_BLOCKS;
		if(in->in.i_dindirect == 0){///there isnt an dindirect block
			bzero(dindirect, BLOCK_SIZE);
			phy_block_nr = testfs_alloc_block_for_inode(in);
			if (phy_block_nr < 0)
				return phy_block_nr;//failed to create dindirect block
			dindirect_allocated = 1;
			in->in.i_dindirect = phy_block_nr;
		}else{//there's already a dindirect block
			read_blocks(in->sb, dindirect, in->in.i_dindirect, 1);//read the dindirect block
		}
		if(((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS] == 0){// if the indirect block wasn't allocated
			bzero(indirect, BLOCK_SIZE);
			phy_block_nr = testfs_alloc_block_for_inode(in);
			if (phy_block_nr < 0){//failed to create indirect block inside dindirect
				if(phy_block_nr == -ENOSPC && dindirect_allocated == 1){
					testfs_free_block_from_inode(in, in->in.i_dindirect);
					in->in.i_dindirect = 0;
				}
				return phy_block_nr;
			}
			((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS] = phy_block_nr;
			
			indirect_allocated =1;
		}else{//indirect block already existed
			read_blocks(in->sb, indirect, ((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS], 1);
		}
		// THIS IS IFFY
		phy_block_nr = testfs_alloc_block_for_inode(in); //now we're creating the direct block
		if(phy_block_nr < 0){
			if(indirect_allocated == 1){//if there's no more space & we created a new indirect block
				//free the indirect block
				testfs_free_block_from_inode(in, ((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS]); //free the indirect block
				((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS] = 0;
			}if(dindirect_allocated == 1){//if there is no more space & we created a dindirect block
				//free the indirect block and the dindirect block
				testfs_free_block_from_inode(in, in->in.i_dindirect);//free indirect and dindirect blocks
				in->in.i_dindirect = 0;
			}
			return phy_block_nr;
		}else{
			((int *)indirect)[log_block_nr % NR_INDIRECT_BLOCKS] = phy_block_nr;
			write_blocks(in->sb, indirect, ((int *)dindirect)[log_block_nr / NR_INDIRECT_BLOCKS], 1);
			write_blocks(in->sb, dindirect, in->in.i_dindirect, 1);
			
		}

		return phy_block_nr;
	}// ***********************************************************************************************************************

	/* allocate an indirect block */
	if (in->in.i_indirect == 0) {	
		bzero(indirect, BLOCK_SIZE);
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if (phy_block_nr < 0)
			return phy_block_nr;
		indirect_allocated = 1;
		in->in.i_indirect = phy_block_nr;
	} else {	/* read indirect block */
		read_blocks(in->sb, indirect, in->in.i_indirect, 1);
	}

	/* allocate direct block */
	assert(((int *)indirect)[log_block_nr] == 0);	
	phy_block_nr = testfs_alloc_block_for_inode(in);

	if (phy_block_nr >= 0) {
		/* update indirect block */
		((int *)indirect)[log_block_nr] = phy_block_nr;
		write_blocks(in->sb, indirect, in->in.i_indirect, 1);
	} else if (indirect_allocated) {
		/* there was an error while allocating the direct block, 
		 * free the indirect block that was previously allocated */
		testfs_free_block_from_inode(in, in->in.i_indirect);
		in->in.i_indirect = 0;
	}
	return phy_block_nr;
}

int
testfs_write_data(struct inode *in, const char *buf, off_t start, size_t size)
{
	char block[BLOCK_SIZE];
	long block_nr = start / BLOCK_SIZE; // logical block number in the file
	long block_ix = start % BLOCK_SIZE; //  index or offset in the block
	int ret;
	//size_t remaining_size = size;
	int written = 0;

	if (block_ix + size > BLOCK_SIZE) {
		ret = testfs_allocate_block(in, block_nr, block);
		if (ret < 0){
			if(ret == -EFBIG){ //COME BACK HERE ****************************
				in->in.i_size = MAX(in->in.i_size, start + written);
			}
			return ret;
		}
		memcpy(block + block_ix, buf + written, BLOCK_SIZE - block_ix);
		write_blocks(in->sb, block, ret, 1);
		written += (BLOCK_SIZE - block_ix);
		block_ix = 0;
		block_nr++; //we just finished inserting the remainder of the first block to be inserted
		//remaining_size -= (BLOCK_SIZE - block_ix);
		while(size - written > BLOCK_SIZE){
			ret = testfs_allocate_block(in, block_nr, block);
			if (ret < 0){
				in->in.i_size = MAX(in->in.i_size, start + written);
				return ret;
			}
			memcpy(block + block_ix, buf + written, BLOCK_SIZE);
			write_blocks(in->sb, block, ret, 1);
			written += BLOCK_SIZE;
			block_ix = 0; //note block_ix is zero through this whole loop and onwards
			block_nr++;
			//remaining_size -= BLOCK_SIZE;
		}

	}
	/* ret is the newly allocated physical block number */
	ret = testfs_allocate_block(in, block_nr, block);
	if (ret < 0){
		in->in.i_size = MAX(in->in.i_size, start + written);
		return ret;
	}
	memcpy(block + block_ix, buf + written, size-written);
	write_blocks(in->sb, block, ret, 1);
	/* increment i_size by the number of bytes written. */
	if (size > 0)
		in->in.i_size = MAX(in->in.i_size, start + (off_t) size);
	in->i_flags |= I_FLAGS_DIRTY;
	/* return the number of bytes written or any error */
	return size;
}

int
testfs_free_blocks(struct inode *in)
{
	int i;
	int e_block_nr;
	char indirect[BLOCK_SIZE];
	char dindirect[BLOCK_SIZE];

	/* last block number */
	e_block_nr = DIVROUNDUP(in->in.i_size, BLOCK_SIZE);

	/* remove direct blocks */
	for (i = 0; i < e_block_nr && i < NR_DIRECT_BLOCKS; i++) {
		if (in->in.i_block_nr[i] == 0)
			continue;
		testfs_free_block_from_inode(in, in->in.i_block_nr[i]);
		in->in.i_block_nr[i] = 0;
	}
	e_block_nr -= NR_DIRECT_BLOCKS;

	/* remove indirect blocks */
	if (in->in.i_indirect > 0) {
		char block[BLOCK_SIZE];
		read_blocks(in->sb, block, in->in.i_indirect, 1);
		for (i = 0; i < e_block_nr && i < NR_INDIRECT_BLOCKS; i++) {
			testfs_free_block_from_inode(in, ((int *)block)[i]);
			((int *)block)[i] = 0;
		}
		testfs_free_block_from_inode(in, in->in.i_indirect);
		in->in.i_indirect = 0;
	}

	e_block_nr -= NR_INDIRECT_BLOCKS;
	if (e_block_nr >= 0) {
	  	if(in->in.i_dindirect > 0){
	    	read_blocks(in->sb, dindirect, in->in.i_dindirect, 1);
	    	for (i = 0; i <= e_block_nr/NR_INDIRECT_BLOCKS && i < NR_INDIRECT_BLOCKS; i++){ //indirect blocks found in the dindirect
	    		if (((int *)dindirect)[i] > 0){
					read_blocks(in->sb, indirect, ((int *)dindirect)[i], 1);
					for (int j = 0; j + (NR_INDIRECT_BLOCKS * i) < e_block_nr && j < NR_INDIRECT_BLOCKS; j++) { //direct found inside the indirect
		 				if(((int *)indirect)[j] > 0){
		     				testfs_free_block_from_inode(in, ((int *)indirect)[j]);
		    		  		((int *)indirect)[j] = 0;
		    			}
					}
					testfs_free_block_from_inode(in, ((int *)dindirect)[i]);
					((int *)dindirect)[i] = 0;
	      		}
	   		}
	    	testfs_free_block_from_inode(in, in->in.i_dindirect);
	    	in->in.i_dindirect = 0;
	  	}
	}

	in->in.i_size = 0;
	in->i_flags |= I_FLAGS_DIRTY;
	return 0;
}