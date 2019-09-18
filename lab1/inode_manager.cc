#include "inode_manager.h"

// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf) //part1A
{
  assert(id >= 0 && id < BLOCK_NUM);
  assert(buf);
  memcpy(buf,blocks[id],BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf) //part1A
{
  assert(id >= 0 && id < BLOCK_NUM);
  assert(buf);
  memcpy(blocks[id],buf,BLOCK_SIZE);
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block() //Part1B
{
  /*
   * your code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
  //check = IBLOCK(INODE_NUM,sb.nblocks) + 1
  uint32_t base = IBLOCK(INODE_NUM,sb.nblocks) + 1;
  for(uint32_t check = 0;check < BLOCK_NUM; check++){
    if(using_blocks[check + base] == 0){
      using_blocks[check + base] = 1;
      return check + base;
    }
  }

  return 0;
}

void
block_manager::free_block(uint32_t id) //Part1B
{
  /* 
   * your code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  
  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM; // = DISK_SIZE = 16M
  sb.nblocks = BLOCK_NUM; //32K
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  inode* root_dir = get_inode(1);
  if (root_dir != NULL) {
    printf("\tim: error! alloc first inode %d, should be 1\n", 0);// Modified
    exit(0);
  }
  root_dir = (struct inode*)malloc(sizeof(struct inode));
  root_dir->type = (short)0;
  root_dir->atime = 0;
  root_dir->ctime = 0;
  root_dir->mtime = 0;
  root_dir->size = 1;
  put_inode(1,root_dir);
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type) //part1A
{
  /* 
   * your code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
  uint32_t inode_start = 2;
  uint32_t target = inode_start;
  inode* new_inode;
  for(;target < INODE_NUM; target++){
    new_inode = get_inode(target);
    if(new_inode == NULL){
      new_inode = new inode();
      new_inode->type = (short)type;
      new_inode->atime = time(0);
      new_inode->ctime = time(0);
      new_inode->mtime = time(0);
      put_inode(target,new_inode);
      free(new_inode);
      return target;
    }
  }
  return 1;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */

  return;
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);
  
  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);

}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size) //Part1B
{
  /*
   * your code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_Out
   */
  inode* node = get_inode(inum);
  *size = node->size;
  int blocknum = (*size-1)/BLOCK_SIZE + 1;
  if ( blocknum > BLOCK_NUM){
    printf("read_file error: blocknum out of bound.\n");
    return;
  }
  char** buf_out_here = (char **)malloc(blocknum * sizeof(char *));
  for (int i = 0; i < blocknum; i++){
    buf_out_here[i] = (char *)malloc(BLOCK_SIZE);
    bm->read_block(node->blocks[i],buf_out_here[i]);
  }
  node->atime = (unsigned) time(0);
  put_inode(inum,node);
  *buf_out = *buf_out_here;
  return;
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size) //Part1B
{
  /*
   * your code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */
  inode* node = get_inode(inum);
  node->size = size;
  int blocknum = (size - 1) / BLOCK_SIZE + 1;
  if (blocknum > BLOCK_SIZE){
    printf("write_file error: blocknum out of bound.\n");
    return;
  }
  for (int i = 0; i < blocknum; i++){
    node->blocks[i] = bm->alloc_block(); 
    bm->write_block(node->blocks[i],buf);
  }
  node->mtime = (unsigned) time(0);
  node->atime = (unsigned) time(0);
  put_inode(inum,node);
  return;
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a) //part1A
{
  /*
   * your code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  inode* inode = get_inode(inum);
  a.type = inode->type;
  a.atime = inode->atime;
  a.mtime = inode->mtime;
  a.ctime = inode->ctime;
  a.size = inode->size;
  return;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  
  return;
}
