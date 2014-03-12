#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ext4_sb.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <linux/types.h>

#define BLOCK_SIZE 4096
#define DIGEST_LENGTH 16
int get_target_device_size(char *blk_device, uint64_t *device_size);

unsigned char *str2md5(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char *digest= (unsigned char*)malloc(DIGEST_LENGTH);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

#if 0
    for (n = 0; n < 16; ++n) {
        printf("%02x", (unsigned int)digest[n]);
    }
#endif

    return digest;
}

int merkle(FILE *fp_data, uint64_t size, char *fname) {

    int i =0;
    int ret = 0;
    int n = 0;
    int blk_count = 0;
    char buff[BLOCK_SIZE];
    unsigned char *digest;
    uint64_t req_blk_read = 0;
    uint64_t req_byte_read = 0;
    FILE *fp_hash;

    req_blk_read = size / BLOCK_SIZE;
    req_byte_read = size % BLOCK_SIZE;
    printf("Have to read %lu blocks and %lu bytes\n",req_blk_read,req_byte_read);
    fflush(stdout);
    /* Open Hash handle */
    /*if(!(fp_hash = open(argv[1],O_RDWR))) {*/
    if(!(fp_hash = open("/tmp/out",O_RDWR))) {
        perror("fopen failed for hash file\n");
        return -1;
    }
    perror("Open hash file handle");

    if((req_blk_read >1)) {
        size = 0;
        /*lseek(fp_hash,size,SEEK_CUR);*/

        while(req_blk_read >0) {
            if(BLOCK_SIZE ==(ret = read(fp_data,buff,BLOCK_SIZE))){
                digest = str2md5((const char *)buff,sizeof(buff));
                write(fp_hash,digest,DIGEST_LENGTH);
                size+=DIGEST_LENGTH;
                if(digest) free(digest);
            }
            req_blk_read--;
        }
        if(req_byte_read) {
            memset(buff,'0',BLOCK_SIZE-req_byte_read);
            if(req_byte_read ==(ret = read(fp_data,buff,req_byte_read))){
                digest = str2md5((const char *)buff,sizeof(buff));
                write(fp_hash,digest,DIGEST_LENGTH);
                size+=DIGEST_LENGTH;
                if(digest) free(digest);
                req_byte_read = 0;
            }
        }
        if(BLOCK_SIZE - size) {
            char *zero_buff = (char *)malloc(sizeof(char) * (BLOCK_SIZE -size));
            memset(zero_buff,"0",(BLOCK_SIZE -size));
            write(fp_hash,zero_buff,(BLOCK_SIZE -size));
            size+=(BLOCK_SIZE -size);
            /*fwrite("0",size%BLOCK_SIZE,1,fp_hash);*/
        }
        printf("written %d bytes of hash\n",size);
        if(fp_data) close(fp_data);
        if(fp_hash) close(fp_hash);

        sync();
        /* Open data handle */
        if(!(fp_data = open(fname,O_RDWR))) {
            perror("fopen failed for data file\n");
            return -1;
        }
        /*perror("Open data file handle");*/
        /* Open Hash handle */
        if(!(fp_hash = open("/tmp/out",O_RDONLY))) {
            perror("fopen failed for hash file\n");
            return -1;
        }
        lseek(fp_data,0,SEEK_END);

        req_blk_read = size / BLOCK_SIZE;
        while(req_blk_read) {
            if(BLOCK_SIZE == read(fp_hash,buff,BLOCK_SIZE) ) {
                write(fp_data,buff,BLOCK_SIZE);
            }
            req_blk_read--;
        }

        /*return 0;*/
        lseek(fp_data,0-size,SEEK_END);

        merkle(fp_data,size,fname);
    }
    else {
        printf("Final block\n");
        if(req_blk_read == 1) {
        printf("Read a block\n");
            if(BLOCK_SIZE ==(ret = read(fp_data,buff,BLOCK_SIZE))){
                digest = str2md5((const char *)buff,sizeof(buff));
                /*write(fp_data,digest,DIGEST_LENGTH);*/
            }
        }
        if(req_byte_read) {
            memset(buff,'0',BLOCK_SIZE-req_byte_read);
            if(req_byte_read ==(ret = read(fp_data,buff,req_byte_read))){
                digest = str2md5((const char *)buff,sizeof(buff));
                /*write(fp_data,digest,DIGEST_LENGTH);*/
                req_byte_read = 0;
            }
        }
        /* return root hash */
        printf("Root hash : ");
        for (n = 0; n < 16; ++n) {
            printf("%02x", (unsigned int)digest[n]);
        }
        printf("\n");
        free (digest);
        return 0;
    }

    perror("End of Hashing : ");
#if 0
    for(i =0; i<BLOCK_SIZE;i++) {
        printf("%x",buff[i]);
    }
#endif
}

int main(int argc, char *argv[]) {
    FILE *fp_data = NULL;
    FILE *fp_hash = NULL;
    uint64_t size = 0;

    /* Input validation */
    if(argc!=3) {
        printf("Usage\n\t %s data_file hash_file 0 | 1 \n",argv[0]);
        printf("\t\t0 - verify\n\t\t1 - generate\n");
        goto end;
    }

    /* Get size of filesystem */
    if(get_target_device_size(argv[1],&size)) {
        perror("Getting size of ext4 imag ");
        goto end;
    }
    /* Open data handle */
    if(!(fp_data = open(argv[1],O_RDONLY))) {
        perror("fopen failed for data file\n");
        goto end;
    }
    perror("Open data file handle");

    merkle(fp_data,size,argv[1]);
end:
    if(fp_data) close(fp_data);
    if(fp_hash) close(fp_hash);
}
