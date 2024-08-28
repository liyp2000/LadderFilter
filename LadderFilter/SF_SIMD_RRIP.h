#ifndef SF_H
#define SF_H

/**
 * cols fixed to 8
 * keylen fixed to 32
 * counterlen fixed to 32
 * maybe push() is better done with swap() ?
*/

#include <iostream>
#include <cassert>

#include "BOBHash32.h"
#include <cstring>
#include <algorithm>
#include <inttypes.h>
#include <immintrin.h>
#include <emmintrin.h>

using namespace std;

const int CELL_NUM = 8;

uint32_t getFP(uint32_t key) {
    return key;
}

__m128i push_index[] = {
    _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15),
    _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15),
    _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 12, 13)
};
__m128i squeeze_index = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 14, 15, 8, 9, 10, 11, 12, 13);
    

class Bucket {

public:
    uint16_t fp[CELL_NUM * 2];
    uint16_t counter[CELL_NUM * 2];

    Bucket() {
        memset(fp, 0, sizeof(fp));
        memset(counter, 0, sizeof(counter));
    }

    int find(uint16_t in_fp, int bid) {
        __m128i _in_fp = _mm_set1_epi16(in_fp);
        __m128i *_fp = (__m128i *)(bid == 0 ? fp : fp + CELL_NUM);

        uint32_t match = _mm_cmpeq_epi16_mask(_in_fp, _fp[0]);
        if (match)
            return _tzcnt_u32(match);
        return -1;
    }

    void push(int p, int bid) {
        if (p > 0) {
            swap(fp[bid == 0 ? p : p + CELL_NUM], fp[bid == 0 ? p - 1 : p - 1 + CELL_NUM]);
            swap(counter[bid == 0 ? p : p + CELL_NUM], counter[bid == 0 ? p - 1 : p - 1 + CELL_NUM]);
        }
        // __m128i *_fp = (__m128i *)(bid == 0 ? fp : fp + CELL_NUM);
        // _fp[0] = _mm_shuffle_epi8(_fp[0], push_index[p]);

        // __m128i *_counter = (__m128i *)(bid == 0 ? counter : counter + CELL_NUM);
        // _counter[0] = _mm_shuffle_epi8(_counter[0], push_index[p]);
    }

    void squeeze(int bid) {
        __m128i *_fp = (__m128i *)(bid == 0 ? fp : fp + CELL_NUM);
        _fp[0] = _mm_shuffle_epi8(_fp[0], squeeze_index);

        __m128i *_counter = (__m128i *)(bid == 0 ? counter : counter + CELL_NUM);
        _counter[0] = _mm_shuffle_epi8(_counter[0], squeeze_index);
    }

};

class LadderFilter {

public:
    Bucket *Bucket1;
    Bucket *Bucket2;
    BOBHash32 *l1Hash;
    BOBHash32 *l2Hash;
    int bucket_num1, bucket_num2;
    int thres2;

    LadderFilter() {}
    LadderFilter(int _bucket_num1, int _bucket_num2,
                 int _cols, int _key_len, int _counter_len,
                 int _thres1, int _thres2,
                 int rand_seed1, int rand_seed2)
        : bucket_num1(_bucket_num1), bucket_num2(_bucket_num2),
          thres2(_thres2) {
        // if (_cols != 8 || _key_len != 16 || _counter_len != 16) {
        //     cerr << "cols, key_len, counter_len must be 8, 16, 16. check parameters." << endl;
        //     assert(0);
        // }
        Bucket1 = new Bucket[(bucket_num1 + 1) / 2];
        Bucket2 = new Bucket[(bucket_num2 + 1) / 2];
        l1Hash = new BOBHash32(rand_seed1);
        l2Hash = new BOBHash32(rand_seed2);
    }

    void addCounter(uint16_t &counter, uint32_t add_val) {
        counter = counter + add_val >= counter ? counter + add_val : 0xFFFFFFFFU;
    }

    int insert(uint32_t key, int count = 1) {
        auto fp = getFP(key);

        static int t = 0;
        ++t;

        /* if the item is in B1 */
        int id1 = l1Hash->run((const char *)&key, 4) % bucket_num1;
        auto &B1 = Bucket1[id1 >> 1];
        int p1 = B1.find(fp, id1 & 1);
        
        if (p1 != -1) {
            B1.push(p1, id1 & 1);
            if (p1 > 0)
                p1--;
            addCounter(B1.counter[(id1 & 1) == 0 ? p1 : CELL_NUM + p1], count);
            return B1.counter[(id1 & 1) == 0 ? p1 : CELL_NUM + p1];
        }
        
        /* if the item is in B2 */
        int id2 = l2Hash->run((const char *)&key, 4) % bucket_num2;
        auto &B2 = Bucket2[id2 >> 1];
        int p2 = B2.find(fp, id2 & 1);
        if (p2 != -1) {
            B2.push(p2, id2 & 1);
            if (p2 > 0)
                p2--;
            addCounter(B2.counter[(id2 & 1) == 0 ? p2 : CELL_NUM + p2], count);
            return B2.counter[(id2 & 1) == 0 ? p2 : CELL_NUM + p2];
        }

        /* if B1 is full, squeeze B1. [0,1,2,3,4,5,6,7] ---> [0,1,2,3,7,4,5,6]
           if the LRU item is a promising item, insert it to B2 */
        B1.squeeze(id1 & 1);
        if (B1.counter[(id1 & 1) == 0 ? 4 : CELL_NUM + 4] >= thres2) {
            B2.squeeze(id2 & 1);
            B2.fp[(id2 & 1) == 0 ? 4 : CELL_NUM + 4] = B1.fp[(id1 & 1) == 0 ? 4 : CELL_NUM + 4];
            B2.counter[(id2 & 1) == 0 ? 4 : CELL_NUM + 4] = B1.counter[(id1 & 1) == 0 ? 4 : CELL_NUM + 4];
        }

        B1.fp[(id1 & 1) == 0 ? 4 : CELL_NUM + 4] = fp;
        B1.counter[(id1 & 1) == 0 ? 4 : CELL_NUM + 4] = count;
        return B1.counter[(id1 & 1) == 0 ? 4 : CELL_NUM + 4];
    }

    int query(int key) {
        auto fp = getFP(key);

        /* if the item is in B1 */
        int id1 = l1Hash->run((const char *)&key, 4) % bucket_num1;
        auto &B1 = Bucket1[id1 >> 1];
        int p1 = B1.find(fp, id1 & 1);
        if (p1 != -1) {
            return B1.counter[(id1 & 1) == 0 ? p1 : CELL_NUM + p1];
        }

        /* if the item is in B2 */
        int id2 = l2Hash->run((const char *)&key, 4) % bucket_num2;
        auto &B2 = Bucket2[id2 >> 1];
        int p2 = B2.find(fp, id2 & 1);
        if (p2 != -1) {
            return B2.counter[(id2 & 1) == 0 ? p2 : CELL_NUM + p2];
        }

        return 0;

    }

};

#endif
