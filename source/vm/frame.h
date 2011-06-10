#ifndef FRAME_H
#define FRAME_H

class Block;

struct Frame
{
	Frame() { }
	Frame(int ip, Block* block) : ip(ip), block(block) { }
	int ip;
	Block* block;
};

#endif
