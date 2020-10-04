#ifndef SC6_BLOCK_H
#define SC6_BLOCK_H

struct block {
    char *data;
    unsigned int size;
    unsigned int allocated;
};

struct block *initBlock();

void freeBlock(struct block *block);

struct block *resetBlock(struct block * block);

void addStringBlock(struct block *block, void *string, unsigned int requiredSpace);

void addFormatStringBlock(struct block *block, unsigned int requiredSpace, char* format, ...);

void addFileBlock(struct block *block, unsigned int requiredSpace, char *filename);

#endif //SC6_BLOCK_H
