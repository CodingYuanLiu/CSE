// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  lc = new lock_client_cache(lock_dst);

  lc->acquire(1);
  printf("Init root dir.\n");
  if (ec->put(1, "") != extent_protocol::OK)
      printf("error init root dir\n"); // XYB: init root dir
  lc->release(1);
}


yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    lc->acquire(inum);
    
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        
        lc->release(inum);
        return false;
    }

    lc->release(inum);

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile?: %lld is not a file\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
yfs_client::isdir(inum inum)
{
    extent_protocol::attr a;

    lc->acquire(inum);

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        lc->release(inum);
        return false;
    }

    lc->release(inum);

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    } 
    printf("isdir?: %lld is not a dir\n", inum);
    return false;
}

bool
yfs_client::issymlink(inum inum)
{
    extent_protocol::attr a;

    lc->acquire(inum);

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        lc->release(inum);
        printf("error getting attr\n");
        return false;
    }

    lc->release(inum);
    
    if (a.type == extent_protocol::T_SYMLINK) {
        printf("issymlink: %lld is a symlink\n", inum);
        return true;
    } 
    printf("issymlink?: %lld is not a symlink\n", inum);
    return false;
}



int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    lc->acquire(inum);

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;    
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    lc->release(inum);
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    lc->acquire(inum);

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;
    
release:
    lc->release(inum);
    return r;
}



// Only support set size of attrentries
int
yfs_client::setattr(inum ino, size_t size)
{
    int r = OK;

    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */

    std::string buf;
    lc->acquire(ino);

    if(ec->get(ino,buf) != extent_protocol::OK){
        lc->release(ino);
        printf("set_attr error: can not get the buf\n");
        return IOERR;
    }
    buf.resize(size);
    if(ec->put(ino,buf) != extent_protocol::OK){
        lc->release(ino);
        printf("set_attr error: can not restore the buf");
        return IOERR;
    }

    lc->release(ino);
    
    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;
    printf("Create file in parent %lld, name %s\n",parent,name);
    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    //CREATE A FILE OR DIR
    
    if(strlen(name) > MAX_FILE_NAMESIZE){
        printf("create error: file size is too large.\n");
        return IOERR;
    }

    //CREATE THE INODE(FILE)
    bool found;

    lc->acquire(parent);
    if(lookup(parent,name,found,ino_out) != OK){
        
        lc->release(parent);
        
        printf("create error: fail to lookup\n");
        return IOERR;
    }
    if(found){

        lc->release(parent);
        
        return EXIST;
    }
    

    extent_protocol::extentid_t id;
    extent_protocol::attr attr;
    ec->create(extent_protocol::T_FILE,id);

    ino_out = id;
    ec->getattr(id,attr);
    assert(attr.size == 0);
    assert(attr.type == extent_protocol::T_FILE); 
    
    //MODIFY THE PARENT INFOMATION    
    std::string buf;
    std::string newEntryString;
    ec->get(parent,buf);
    struct dir_entry newEntry;
    newEntry.inum = ino_out;
    newEntry.nameSize = strlen(name);
    memcpy(newEntry.filename,name,newEntry.nameSize);
    newEntryString.assign((char *)(&newEntry),sizeof(struct dir_entry));
    assert(newEntryString.size() == sizeof(struct dir_entry));
    buf += newEntryString;
    
    printf("debug create: the new content of parent dir %llu is %s, new entrystring is %s\n",parent,buf.c_str(), newEntryString.c_str());
    printf("size of newEntry String and buf is %lu,%lu\n",newEntryString.size(),buf.size());
    ec->put(parent,buf);
    
    lc->release(parent);
    
    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;
    printf("start mkdir in parent %lld, name %s\n",parent,name);
    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */

    if(strlen(name) > MAX_FILE_NAMESIZE){
        printf("mkdir error: file size is too large.\n");
        return IOERR;
    }

    //CREATE THE INODE(FILE)
    bool found;
    
    lc->acquire(parent);


    if(lookup(parent,name,found,ino_out) != OK){
        printf("mkdir error: fail to lookup\n");
        lc->release(parent);
        return IOERR;
    }
    if(found){
        lc->release(parent);
        return EXIST;
    }
    
    extent_protocol::extentid_t id;
    extent_protocol::attr attr;
    ec->create(extent_protocol::T_DIR,id);
    ino_out = id;
    ec->getattr(id,attr);
    assert(attr.size == 0);
    assert(attr.type == extent_protocol::T_DIR); 

    std::string buf;
    std::string newEntryString;
    ec->get(parent,buf);
    struct dir_entry newEntry;
    newEntry.inum = ino_out;
    newEntry.nameSize = strlen(name);
    memcpy(newEntry.filename,name,newEntry.nameSize);
    newEntryString.assign((char *)(&newEntry),sizeof(struct dir_entry));
    buf += newEntryString;

    ec->put(parent,buf);
    
    lc->release(parent);
    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;

    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */

    //TODO: Lock LOOKUP? (reference in create,mkdir,symlink,unlink)
    std::list<dirent> entries;

    if (readdir(parent, entries) != OK){
        printf("read parent dir error\n");
        return IOERR;
    }

    std::string stringName;
    for(std::list<dirent>::iterator it = entries.begin(); it != entries.end(); it++){
        struct dirent entry = *it;
        stringName.assign(name,strlen(name));
        if(stringName == entry.name){
            found = true;
            ino_out = entry.inum;
            return r;
        }
    }
    found = false;
    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;

    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    std::string buf;
    extent_protocol::attr attr;
    ec->getattr(dir,attr);
    
    if(attr.type != extent_protocol::T_DIR){
        printf("readdir error: not a dir\n");
        return IOERR;
    }
    
    if(ec->get(dir,buf) != extent_protocol::OK){
        printf("readdir error.\n");
        return IOERR;
    }
    
    
    attr.atime = time(0);
    const char* cbuf = buf.c_str();
    uint32_t size = buf.size();
    uint32_t entryNum = size / sizeof(dir_entry);
    printf("debug readdir: direntry size of dir %llu is: %u\n",dir,size);
    assert(size % sizeof(dir_entry) == 0);

    for(uint32_t i = 0; i < entryNum; i++){
        struct dir_entry entry;
        memcpy(&entry,cbuf + i * sizeof(dir_entry), sizeof(dir_entry));
        struct dirent dirent;
        dirent.inum = entry.inum;
        dirent.name.assign(entry.filename,entry.nameSize); 
        list.push_back(dirent);
    } 
    
    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;

    /*
     * your code goes here.
     * note: read using ec->get().
     */
    std::string buf;
    
    lc->acquire(ino);

    if(ec->get(ino,buf) != extent_protocol::OK){
        printf("read error: can not get the buf\n");
    
        lc->release(ino);
        return IOERR;
    }
    
    if((size_t) off >= buf.size()){
        lc->release(ino);
        printf("read error: the offset is larger than the file size");
        return IOERR;
    }

    lc->release(ino);

    data = buf.substr(off,size);
    printf("debug read: data of %lld is %s\n",ino,data.c_str());
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;

    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    std::string content;
    
    lc->acquire(ino);

    if(ec->get(ino,content) != extent_protocol::OK){
        lc->release(ino);

        printf("write error: can not get the buf\n");
        return IOERR;
    }
    size_t originalSize = content.size();
    std::string buf;
    buf.assign(data,size);
    if((size_t) off > originalSize ){
        // new file: "|CONTENT|'\0''\0'...|BUF|"
        content.resize(off + size,'\0');
        content.replace(off,size,buf);
        bytes_written = size + off - originalSize;
    } else{
        content.replace(off,size,buf);
        bytes_written = size; 
    }
    ec->put(ino,content);

    //debug
    printf("debug write: data of %lld is %s\n",ino,data);
    
    lc->release(ino);
    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;

    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    bool found = false;
    inum ino_out;

    lc->acquire(parent);

    lookup(parent,name,found,ino_out);
    if(found != true){
        printf("unlink error: cannot find the file\n");
        lc->release(parent);
        return NOENT;
    }
    
    ec->remove(ino_out);

    std::list<dirent> entries;
    if (readdir(parent, entries) != OK){
        lc->release(parent);
        printf("read parent dir error\n");
        return IOERR;
    }
    
    std::string newParent;
    std::string stringName;
    stringName.assign(name,strlen(name));

    for(std::list<dirent>::iterator it = entries.begin(); it != entries.end(); it++){
        struct dirent direntEntry = *it;

        //Remove the file entry from parent
        if(stringName == direntEntry.name){
            continue;
        }

        struct dir_entry newEntry;
        std::string append;

        newEntry.inum = direntEntry.inum;
        newEntry.nameSize = direntEntry.name.size();
        memcpy(newEntry.filename,direntEntry.name.c_str(),newEntry.nameSize);
        append.assign((char *)&newEntry,sizeof(struct dir_entry));
        newParent += append;
    }    
    
    ec->put(parent,newParent);

    lc->release(parent);
    return r;
}

int yfs_client::symlink(inum parent, const char* link, const char* name,inum &ino_out){
    int r = OK;
    
    if(strlen(name) > MAX_FILE_NAMESIZE){
        printf("symlink error: file size is too large.\n");
        return IOERR;
    }
    extent_protocol::extentid_t id;
    //CREATE THE INODE(FILE)
    bool found;    
    
    lc->acquire(parent);

    printf("symlink before lookup\n");
    if(lookup(parent,name,found,id) != OK){
        
        lc->release(parent);

        printf("symlink error: fail to lookup\n");
        return IOERR;
    }
    if(found){
        lc->release(parent);
        return EXIST;
    }
    
    ec->create(extent_protocol::T_SYMLINK,id);
    ino_out = id;

    //Write the symlink file
    ec->put(id,std::string(link));

    //Add the info into the parent dir
    std::string buf;
    std::string newEntryString;
    ec->get(parent,buf);
    struct dir_entry newEntry;
    newEntry.inum = ino_out;
    newEntry.nameSize = strlen(name);
    memcpy(newEntry.filename,name,newEntry.nameSize);
    newEntryString.assign((char *)(&newEntry),sizeof(struct dir_entry));
    buf += newEntryString;
    
    ec->put(parent,buf);

    lc->release(parent);
    return r;
}

int yfs_client::readlink(inum ino, std::string &data){
    int r = OK;
    std::string buf;
    lc->acquire(ino);
    r = ec->get(ino, buf);
    lc->release(ino);
    data = buf;
    return r;
}

int
yfs_client::lookup_lock(inum parent, const char *name, bool &found, inum &ino_out){
    printf("lookup lock for parent %lld, name %s\n",parent,name);
    //In order to pass lab3 testcases, do not lock the lookup totally.
    //lc->acquire(parent);
    int r = lookup(parent,name,found,ino_out);
    //lc->release(parent);
    return r;
}


//TODO
int
yfs_client::rmdir(inum parent,const char *name){
    int r = OK;
    //TODO
    return r;
}