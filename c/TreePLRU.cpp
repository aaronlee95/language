// Tree-Base PLRU Example
// 4-Way Set Associative D-Cache
#include<iostream>
#include<vector>
#include<algorithm>
#include<cstdint>

using namespace std;
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>

#define BS 16  /* 16*4=64B */
#define BS_BITS 4

struct AddressField {
    uint64_t wd_idx : 2;
    uint64_t offset : 4;
    uint64_t index  : 8;
    uint64_t tag    : 18;
    uint64_t pad    : 32;
};

union Address {
      uint32_t*    p;
      AddressField fields;
};

struct Cell {
   bool v;
   uint32_t tag;
   uint32_t data[BS];
   Cell() : v(false), tag(0) {
     memset(data, 0, sizeof(data));
   }
   bool isHit(uint32_t tag) {
     return v && (tag == this->tag);
   }
   void fetch(uint32_t* address) {
	 Address addr;
	 addr.p = address;
	 addr.fields.offset  = 0;
	 addr.fields.wd_idx  = 0;
	 for (int i = 0; i < BS; i++) {
               data[i] = addr.p[i];
         }
         tag = addr.fields.tag;
         v = true;
   }
};

ostream& operator<<(ostream & out, const Cell& cell) {
	out << " v:" << cell.v << " tag:" << hex << cell.tag << " data:";
        for (int i = 0; i < BS; i++)  {
		out << " " << cell.data[i];
        }
	return out;
}

#define NWAY 4 /* N-ways */
/*
Ref: https://people.cs.clemson.edu/~mark/464/p_lru.txt
four-way set associative - three bits

   each bit represents one branch point in a binary decision tree; let 1
   represent that the left side has been referenced more recently than the
   right side, and 0 vice-versa

              are all 4 lines valid?
                   /       \
                 yes        no, use an invalid line
                  |
                  |
                  |
             bit_0 == 0?            state | replace      ref to | next state
              /       \             ------+--------      -------+-----------
             y         n             00x  |  line_0      line_0 |    11_
            /           \            01x  |  line_1      line_1 |    10_
     bit_1 == 0?    bit_2 == 0?      1x0  |  line_2      line_2 |    0_1
       /    \          /    \        1x1  |  line_3      line_3 |    0_0
      y      n        y      n
     /        \      /        \        ('x' means       ('_' means unchanged)
   line_0  line_1  line_2  line_3      don't care)

 8-way set associative - 7  = 1+2+4 bits
16-way set associative - 15 = 1+2+4+8 bits
32-way set associative - 31 = 1+2+4+8+16 bits
64-way set associative - 63 = 1+2+4+8+16+32 bits
*/
#define HIT  0
#define MISS 1
#define LINE_0_MASK 6
#define LINE_1_MASK 6
#define LINE_2_MASK 5
#define LINE_3_MASK 5
struct Block {
   Cell cell[NWAY];
   uint32_t state;
   Block() : state(0){ }

   uint32_t* getByTag(uint32_t tag, int* pway) {
      for (int i=0; i<NWAY; ++i) {
         if (cell[i].isHit(tag)) {
	    *pway = i;
            return cell[i].data;
         }
      }
      return NULL;
   }

   void setLRU(uint32_t* address) {
         int way = 0;
	 uint32_t st = state;
         uint32_t mask[] = { 
		LINE_0_MASK,
		LINE_1_MASK,
		LINE_2_MASK,
		LINE_3_MASK
         };
	 for (int i = 0; i < NWAY; ++i) {
	   if ( (state & mask[i]) == 0) {
             state ^= mask[i];
	     way = i;
	     break;
           }
	 }
         cell[way].fetch(address);
         cout << "way:" << way << " address:" << address << " state:" << st << "->" << state << endl;
   }

   uint32_t* get(uint32_t* address, int *pway) {
	 Address addr;
	 addr.p = address;
         uint32_t* d = getByTag(addr.fields.tag, pway);
         if (d != NULL) {
             return &d[addr.fields.offset];
         }
	 return d;
   }

   int set(uint32_t* address) {
       int way = 0;
       uint32_t* p = get(address, &way);
       if (p != NULL) {
	   switch (way) {
           case 0:
           case 1: state ^= LINE_0_MASK; break;
           case 2:
           case 3: state ^= LINE_2_MASK; break;
           default: assert(0);
           }
           *p = *address;
           return HIT;
       } else {
          setLRU(address);
          return MISS;
       }
   }
};
ostream& operator<<(ostream & out, const Block& block) {
	out << "state:" << block.state << " ";
	for (int i = 0; i<NWAY; i++) {
            out << block.cell[i];
	}
	return out;
}


#define NLINE      256
#define NLINE_BITS 8
#define NLINE_MASK 0xFF
#define NWORD_BITS 2
struct Cache {
    Block block[NLINE];
    uint32_t count[2];
    Cache() { count[HIT] = 0; count[MISS] = 0; }
    void access(uint32_t* address) {
	 Address addr;
	 addr.p = address;
         Block& b = block[addr.fields.index];
         ++count[b.set(address)];
    }

};
ostream& operator<<(ostream & out, const Cache& cache) {
	out << "Hit: " << cache.count[HIT] <<  " Miss: " << cache.count[MISS] << std::endl;
	for (int i = 0; i < NLINE; i++) {
            out << cache.block[i] << endl;
        }
	return out;
}


// C program to multiply two square matrices
// http://www.geeksforgeeks.org/c-program-multiply-two-matrices/
#include<stdio.h>

// #define MS 10000
#define MS  256
// #define MS  16

uint32_t m1[MS][MS];
uint32_t m2[MS][MS];
uint32_t res[MS][MS];

Cache cache;
void multiply(uint32_t m1[][MS], uint32_t m2[][MS])
{
    int x, i, j;
    for (i = 0; i < MS; i++) {
        for (j = 0; j < MS; j++) {
            cache.access(&res[i][j]);
            res[i][j] = 0;
            for (x = 0; x < MS; x++) {
		cache.access(&m1[i][x]);
		cache.access(&m1[x][j]);
		cache.access(&res[i][j]);
                res[i][j] += m1[i][x] * m2[x][j];
		cache.access(&res[i][j]);
            }
        }
    }
#if 0
    for (i = 0; i < MS; i++) {
        for (j = 0; j < MS; j++) {
            printf("%d ", res[i][j]);
        }
        printf("\n");
    }
    printf("\n");
#endif
}

void init(uint32_t m[][MS])
{
  for(int i=0; i<MS; i++) {
    for(int j=0; j<MS; j++) {
        m[i][j] = i*MS +j;
    }
  }
}

int main()
{
    init(m1);
    init(m2);
    multiply(m1, m2);
    cout << cache << endl;
    return 0;
}