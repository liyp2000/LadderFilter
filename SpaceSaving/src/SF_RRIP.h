#ifndef SF_RRIP_H
#define SF_RRIP_H

#include "../../LadderFilter/SF_noSIMD_LFU.h"
#include <algorithm>

class RRIPBucket
{
public:
    uint32_t *fp;
    uint32_t *counter;

    RRIPBucket() {}

    RRIPBucket(int cols)
    {
        fp = new uint32_t[cols];
        counter = new uint32_t[cols];
        memset(fp, 0, sizeof(uint32_t) * cols);
        memset(counter, 0, sizeof(uint32_t) * cols);
    }
};

template<uint32_t ss_capacity>
class SF_RRIP
{
public:
    LadderFilter sf;
    int thres, key_len, bucket_num3, cols;
    RRIPBucket *Bucket3;
    BOBHash32 *l3Hash;

    SF_RRIP(int _bucket_num1, int _bucket_num2,
                  int _cols, int _key_len, int _counter_len,
                  int _thres1, int _thres2, 
                  int rand_seed1, int rand_seed2)
    {
        thres = _thres1;
        sf = LadderFilter(_bucket_num1, _bucket_num2,
                           _cols, _key_len, _counter_len,
                           _thres1, _thres2,
                           rand_seed1, rand_seed2);
        
        key_len = _key_len;
        cols = _cols;
        bucket_num3 = ss_capacity * 100 / (_cols * 8);
        // cout << "# items: " << bucket_num3 * cols << endl;

        Bucket3 = new RRIPBucket[bucket_num3];
        for (int i = 0; i < bucket_num3; ++i)
            Bucket3[i] = RRIPBucket(_cols);

        l3Hash = new BOBHash32(123);
    }

    inline void insert(uint32_t key)
    {
        /* if the item is in B3 */
        auto &B3 = Bucket3[l3Hash->run((const char *)&key, key_len) % bucket_num3];
        for (int i = 0; i < cols; ++i)
            if (B3.fp[i] == key)
            {
                B3.counter[i] += 1;
                if (i > 0) {
                    swap(B3.counter[i], B3.counter[i - 1]);
                    swap(B3.fp[i], B3.fp[i - 1]);
                }
                return;
            }

        auto res = sf.insert(key);
        if (res >= thres)
        {
            int pos = cols / 2;// AAE sways wen pos = cols * 2/3
            for (int i = cols - 1; i > pos; i--) {
                B3.counter[i] = B3.counter[i - 1];
                B3.fp[i] = B3.fp[i - 1];
            }
            B3.counter[pos] = res;
            B3.fp[pos] = key;
 
        }
    }

    inline void build(uint32_t * items, int n)
    {
        for (int i = 0; i < n; ++i)
            insert(items[i]);
    }

    void get_top_k(uint32_t k, uint32_t items[])
    {
        vector<pair<uint32_t, uint32_t>> result;

        for (int i = 0; i < bucket_num3; ++i)
            for (int j = 0; j < cols; ++j)
                if (Bucket3[i].counter[j] > 0)
                    result.push_back(make_pair(Bucket3[i].fp[j], Bucket3[i].counter[j]));
        
        sort(result.begin(), result.end(), [](const pair<uint32_t, uint32_t>& a, const pair<uint32_t, uint32_t>& b){
                return a.second > b.second;
            });
        
        if (result.size() > k)
            result.resize(k);

        for (int i = 0; i < k; ++i)
            items[i] = result[i].first;
    }

    void get_top_k_with_frequency(uint32_t k, vector<pair<uint32_t, uint32_t>> & result)
    {
        result.clear();

        for (int i = 0; i < bucket_num3; ++i)
            for (int j = 0; j < cols; ++j)
                if (Bucket3[i].counter[j] > 0)
                    result.push_back(make_pair(Bucket3[i].fp[j], Bucket3[i].counter[j]));
        
        sort(result.begin(), result.end(), [](const pair<uint32_t, uint32_t>& a, const pair<uint32_t, uint32_t>& b){
                return a.second > b.second;
            });
        
        if (result.size() > k)
            result.resize(k);
    }
};


#endif
