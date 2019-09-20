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

yfs_client::yfs_client()
{
    ec = new extent_client();

}

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client();
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
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

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
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
    // Oops! is this still correct when you implement symlink?
    return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
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
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

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

    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    //CREATE A FILE OR DIR
    /*
    if(strlen(name) > MAX_FILE_NAMESIZE){
        printf("create error: file size is too large.\n");
        return IOERR;
    }

    bool found;
    if(lookup(parent,name,found,ino_out) != OK){
        printf("create error: fail to lookup\n");
        return IOERR;
    }
    if(found){
        return EXIST;
    }
    

    extent_protocol::extentid_t id;
    extent_protocol::attr attr;
    ec->create(extent_protocol::T_FILE,id);
    ino_out = id;
    ec->getattr(id,attr);
    assert(attr.size == 0);
    assert(attr.type == extent_protocol::T_FILE); 
    
    //TODO: MODIFY THE PARENT INFOMATION
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
    */
    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */

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
    if(ec->get(dir,buf) != extent_protocol::OK){
        printf("readdir error.\n");
        return IOERR;
    }
    ec->getattr(dir,attr);
    if(attr.type != extent_protocol::T_DIR){
        printf("error: lookup in a non-dir\n");
        return IOERR;
    }
    attr.atime = time(0);
    const char* cbuf = buf.c_str();
    uint32_t size = buf.size();
    uint32_t entryNum = size / sizeof(dir_entry);

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

    return r;
}

