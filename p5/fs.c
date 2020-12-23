// ============================================================================
// fs.c - user FileSytem API
// ============================================================================

#include "bfs.h"
#include "fs.h"

// ============================================================================
// Close the file currently open on file descriptor 'fd'.
// ============================================================================
i32 fsClose(i32 fd) { 
  i32 inum = bfsFdToInum(fd);
  bfsDerefOFT(inum);
  return 0; 
}



// ============================================================================
// Create the file called 'fname'.  Overwrite, if it already exsists.
// On success, return its file descriptor.  On failure, EFNF
// ============================================================================
i32 fsCreate(str fname) {
  i32 inum = bfsCreateFile(fname);
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Format the BFS disk by initializing the SuperBlock, Inodes, Directory and 
// Freelist.  On succes, return 0.  On failure, abort
// ============================================================================
i32 fsFormat() {
  FILE* fp = fopen(BFSDISK, "w+b");
  if (fp == NULL) FATAL(EDISKCREATE);

  i32 ret = bfsInitSuper(fp);               // initialize Super block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitInodes(fp);                  // initialize Inodes block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitDir(fp);                     // initialize Dir block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitFreeList();                  // initialize Freelist
  if (ret != 0) { fclose(fp); FATAL(ret); }

  fclose(fp);
  return 0;
}


// ============================================================================
// Mount the BFS disk.  It must already exist
// ============================================================================
i32 fsMount() {
  FILE* fp = fopen(BFSDISK, "rb");
  if (fp == NULL) FATAL(ENODISK);           // BFSDISK not found
  fclose(fp);
  return 0;
}



// ============================================================================
// Open the existing file called 'fname'.  On success, return its file 
// descriptor.  On failure, return EFNF
// ============================================================================
i32 fsOpen(str fname) {
  i32 inum = bfsLookupFile(fname);        // lookup 'fname' in Directory
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Read 'numb' bytes of data from the cursor in the file currently fsOpen'd on
// File Descriptor 'fd' into 'buf'.  On success, return actual number of bytes
// read (may be less than 'numb' if we hit EOF).  On failure, abort
// ============================================================================
i32 fsRead(i32 fd, i32 numb, void* buf) {
  i8 *buff = (i8 *)buf; // cast buf to i8 array
  int start = 0, end = 513, read = 0, offset = 0; // start index, end index, number of bytes read
  i32 curs = bfsTell(fd), inum = bfsFdToInum(fd), size = fsSize(fd); // cursor position, inum, size of file
  i32 fbn = curs / BYTESPERBLOCK, begin = curs;
  // while read less than original numb and not reading over file size
  while (numb > 0 && read < (size - begin)) {
    offset = read;
    curs = bfsTell(fd); // get cursor position
    i8 bioBuf[BYTESPERBLOCK]; // allocate buffer
    memset(bioBuf, 0, BYTESPERBLOCK);
    bfsRead(inum, fbn++, bioBuf); // read to buffer
    start = curs % 512, end = start + numb; // get index to start reading from
    if (end > BYTESPERBLOCK) end = BYTESPERBLOCK; // if start greater than 512, set to 512
    for (start; start < end; start++) { // loop from start to end
      if (read >= (size - begin)) break; // if reading over file, then stop
      buff[read++] = bioBuf[start]; // add to buffer and increment read
    }
    offset = read - offset; // offset is number of bytes read in this cycle
    fsSeek(fd, offset, SEEK_CUR); // increase cursor by offset
    numb -= offset; 
  }

  return read;
}


// ============================================================================
// Move the cursor for the file currently open on File Descriptor 'fd' to the
// byte-offset 'offset'.  'whence' can be any of:
//
//  SEEK_SET : set cursor to 'offset'
//  SEEK_CUR : add 'offset' to the current cursor
//  SEEK_END : add 'offset' to the size of the file
//
// On success, return 0.  On failure, abort
// ============================================================================
i32 fsSeek(i32 fd, i32 offset, i32 whence) {

  if (offset < 0) FATAL(EBADCURS);
 
  i32 inum = bfsFdToInum(fd);
  i32 ofte = bfsFindOFTE(inum);
  
  switch(whence) {
    case SEEK_SET:
      g_oft[ofte].curs = offset;
      break;
    case SEEK_CUR:
      g_oft[ofte].curs += offset;
      break;
    case SEEK_END: {
        i32 end = fsSize(fd);
        g_oft[ofte].curs = end + offset;
        break;
      }
    default:
        FATAL(EBADWHENCE);
  }
  return 0;
}



// ============================================================================
// Return the cursor position for the file open on File Descriptor 'fd'
// ============================================================================
i32 fsTell(i32 fd) {
  return bfsTell(fd);
}



// ============================================================================
// Retrieve the current file size in bytes.  This depends on the highest offset
// written to the file, or the highest offset set with the fsSeek function.  On
// success, return the file size.  On failure, abort
// ============================================================================
i32 fsSize(i32 fd) {
  i32 inum = bfsFdToInum(fd);
  return bfsGetSize(inum);
}



// ============================================================================
// Write 'numb' bytes of data from 'buf' into the file currently fsOpen'd on
// filedescriptor 'fd'.  The write starts at the current file offset for the
// destination file.  On success, return 0.  On failure, abort
// ============================================================================
i32 fsWrite(i32 fd, i32 numb, void* buf) {
  i8 *buff = (i8 *)buf; // cast buf to i8 array
  int start = 0, end = 513, offset = 0, write = 0, ext = 0;
  i32 sizeOffset = 0; // number of bytes to increase file size by
  i32 curs = bfsTell(fd), inum = bfsFdToInum(fd), fbn = curs / BYTESPERBLOCK;
  // while bytes left to write
  while (numb > 0) {
    offset = write;
    curs = bfsTell(fd); // get cursor position
    i8 bioBuf[BYTESPERBLOCK]; // allocate buffer
    memset(bioBuf, 0, BYTESPERBLOCK);
    i32 dbn = bfsFbnToDbn(inum, fbn); // get dbn
    // if invalid dbn
    if (dbn < 0 || dbn > BLOCKSPERDISK) {
      dbn = bfsAllocBlock(inum, fbn++); // allocate new block in file
      ext = 1; // set extend flag to true
    } else {
      bfsRead(inum, fbn++, bioBuf); // otherwise read to buffer
    }
    start = curs % BYTESPERBLOCK, end = start + numb; // find start write position in block
    if (end > BYTESPERBLOCK) end = BYTESPERBLOCK; // if position over 512, set to 512
    // loop from start to end
    for (start; start < end; start++) {
      bioBuf[start] = buff[write++]; // add to buffer
    }
    bioWrite(dbn, bioBuf); // write contents of buffer to file
    offset = write - offset; // get number of bytes written in this cycle
    fsSeek(fd, offset, SEEK_CUR); // update cursor
    numb -= offset; // update bytes remaining to be written
    // if file extended
    if (ext) {
      sizeOffset += offset; // update sizeOffset
    }
  }

  i32 size = fsSize(fd); // get file size
  // if file extended
  if (ext) {
    bfsSetSize(inum, size + sizeOffset); // update size of file
  }

  return 0;
}