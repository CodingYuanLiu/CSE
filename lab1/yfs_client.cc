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

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

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

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

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

    std::string buf;
    if(ec->get(ino,buf) != extent_protocol::OK){
        printf("set_attr error: can not get the buf\n");
        return IOERR;
    }
    buf.resize(size);
    if(ec->put(ino,buf) != extent_protocol::OK){
        printf("set_attr error: can not restore the buf");
        return IOERR;
    }
    
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
    
    if(strlen(name) > MAX_FILE_NAMESIZE){
        printf("create error: file size is too large.\n");
        return IOERR;
    }

    //CREATE THE INODE(FILE)
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
    
    //MODIFY THE PARENT INFOMATION
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
    //TODO: CREATE A DIR FILE WITH NAME
    if(strlen(name) > MAX_FILE_NAMESIZE){
        printf("mkdir error: file size is too large.\n");
        return IOERR;
    }

    //CREATE THE INODE(FILE)
    bool found;
    if(lookup(parent,name,found,ino_out) != OK){
        printf("mkdir error: fail to lookup\n");
        return IOERR;
    }
    if(found){
        return EXIST;
    }
    
    extent_protocol::extentid_t id;
    extent_protocol::attr attr;
    ec->create(extent_protocol::T_DIR,id);
    ino_out = id;
    ec->getattr(id,attr);
    assert(attr.size == 0);
    assert(attr.type == extent_protocol::T_DIR); 

    //TODO: ADD THE ENTRY TO THE PARENT DIR
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
        return IOERR;
    }
    attr.atime = time(0);
    const char* cbuf = buf.c_str();
    uint32_t size = buf.size();
    uint32_t entryNum = size / sizeof(dir_entry);
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
    if(ec->get(ino,buf) != extent_protocol::OK){
        printf("read error: can not get the buf\n");
        return IOERR;
    }
    
    if((size_t) off >= buf.size()){
        printf("read error: the offset is larger than the file size");
        return IOERR;
    }

    data = buf.substr(off,size);

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
    if(ec->get(ino,content) != extent_protocol::OK){
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
        bytes_written = size;
    } else{
        content.replace(off,size,buf);
        bytes_written = size + off - originalSize; 
    }
    ec->put(ino,content);

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
    //TODO: Remove the existing file
    bool found = false;
    inum ino_out;
    lookup(parent,name,found,ino_out);
    if(found != true){
        printf("unlink error: cannot find the file\n");
        return NOENT;
    }
    
    ec->remove(ino_out);

    //TODO: Update the parent directory content
    std::list<dirent> entries;
    if (readdir(parent, entries) != OK){
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

    return r;
}

int yfs_client::symlink(inum parent, const char* link, const char* name,inum &ino_out){
    int r = OK;
    //TODO: Create the symlink file
    if(strlen(name) > MAX_FILE_NAMESIZE){
        printf("symlink error: file size is too large.\n");
        return IOERR;
    }
    extent_protocol::extentid_t id;
    //CREATE THE INODE(FILE)
    bool found;
    if(lookup(parent,name,found,id) != OK){
        printf("symlink error: fail to lookup\n");
        return IOERR;
    }
    if(found){
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

    return r;
}

int yfs_client::readlink(inum ino, std::string &data){
    int r = OK;
    std::string buf;
    r = ec->get(ino, buf);
    data = buf;
    return r;
}