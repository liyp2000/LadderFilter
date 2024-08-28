#include <iostream>
#include <string>
#include <queue>
#include <stack>
#include <list>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include <numeric>
#include <cmath>
#include <stdexcept>
#include <cassert>
#include <cstring>

#include "SF_LFU.h"
// #include "SF_RRIP.h"

#include "SpaceSaving.h"
#include "SF_SpaceSaving.h"
#include "SC_SpaceSaving.h"
#include "LF_SpaceSaving.h"


// Bob hash
#include "BOBHash32.h"
#include "SPA.h"
// SIMD 
#include <immintrin.h>
#define UNIX
#ifdef UNIX
#include <x86intrin.h>
#else
#include <intrin.h>
#endif

using namespace std;

#define MAX_INSERT_PACKAGE 33000000

unordered_map<uint32_t, int> ground_truth;
uint32_t insert_data[MAX_INSERT_PACKAGE];
uint32_t query_data[MAX_INSERT_PACKAGE];
FILE* SS_ACC;
FILE* SS_AAE;
FILE* SS_TPT;



// load data
int load_data(const char *filename) {
    FILE *pf = fopen(filename, "rb");
    if (!pf) {
        cerr << filename << " not found." << endl;
        exit(-1);
    }

    ground_truth.clear();

    char ip[13];
    char ts[8];
    int ret = 0;
    while (1) {
        size_t rsize;

        /* CAIDA */
        // rsize = fread(ip, 1, 13, pf);
        // if(rsize != 13) break;
        // rsize = fread(ts, 1, 8, pf);
        // if(rsize != 8) break;

        /* zipf */
        rsize = fread(ip, 1, 4, pf);
        if(rsize != 4) break;

        /* webdoc */
        // rsize = fread(ip, 1, 4, pf);
        // if(rsize != 4) break;
        // rsize = fread(ts, 1, 4, pf);
        // if(rsize != 4) break;

        uint32_t key = *(uint32_t *) ip;
        insert_data[ret] = key;
        ground_truth[key]++;
        ret++;
        if (ret == MAX_INSERT_PACKAGE){
            cout << "MAX_INSERT_PACKAGE" << endl;
            break;
        }
    }
    fclose(pf);

    int i = 0;
    for (auto itr: ground_truth) {
        query_data[i++] = itr.first;
    }

    printf("Total stream size = %d\n", ret);
    printf("Distinct item number = %ld\n", ground_truth.size());

    int max_freq = 0;
    for (auto itr: ground_truth) {
        max_freq = std::max(max_freq, itr.second);
    }
    printf("Max frequency = %d\n", max_freq);

    vector<int> s;
    for (auto itr: ground_truth) {
        s.push_back(itr.second);
    }
    sort(s.begin(), s.end());
    cout << s[s.size() - 1] << " " << s[s.size() - 1000] << endl;

    return ret;
}


pair<double, double> ss_compare_value_with_ground_truth(uint32_t k, vector<pair<uint32_t, uint32_t>> & result)
{
    // prepare top-k ground truth
    vector<pair<uint32_t, int>> gt_ordered;
    for (auto itr: ground_truth) {
        gt_ordered.emplace_back(itr);
    }
    std::sort(gt_ordered.begin(), gt_ordered.end(), [](const std::pair<uint32_t, int> &left, const std::pair<uint32_t, int> &right) {
        return left.second > right.second;
    });

    // for (int i = 0; i < std::min(k, (uint32_t)gt_ordered.size()); i++) {
    //     printf("ground_truth : %u, %d\n", gt_ordered[i].first, gt_ordered[i].second);
    // }
    // for (int i = 0; i < std::min(k, (uint32_t)result.size()); i++) {
    //     printf("result : %u, %u\n", result[i].first, result[i].second);
    // }


    set<uint32_t> set_gt;
    int i = 0;
    int th;
    for (auto itr: gt_ordered) {
        if (i >= k && itr.second < th) {
            break;
        }
        set_gt.insert(itr.first);
        i++;
        if (i == k) {
            th = itr.second;
        }
    }

    double aae = 0;

    set<uint32_t> set_rp;
    unordered_map<uint32_t, uint32_t> mp_rp;

    int lp = 0;
    for (lp = 0; lp < k; ++lp) {
        set_rp.insert(result[lp].first);
        mp_rp[result[lp].first] = result[lp].second;
    }

    vector<uint32_t> intersection(k);
    auto end_itr = std::set_intersection(
            set_gt.begin(), set_gt.end(),
            set_rp.begin(), set_rp.end(),
            intersection.begin()
    );


    for (auto itr = intersection.begin(); itr != end_itr; ++itr) {
        int diff = int(mp_rp[*itr]) - int(ground_truth[*itr]);
        //cout << int(mp_rp[*itr]) << " " << int(ground_truth[*itr]) << endl;
	aae += abs(diff);
    }

    int num = end_itr - intersection.begin();
    num = num > 0 ? num : 1;

    return make_pair(double(num) / k, aae / num);
}

template<uint32_t mem_in_byte_all, uint32_t topk, uint32_t L2Thres>
void demo_ss(int packet_num)
{
    constexpr double f = 1.5;
    constexpr uint32_t mem_in_byte = mem_in_byte_all - topk * f * 100;

    // printf("\nExp for top-k query:\n");
    // printf("\ttest top %d\n", topk);
    // printf("\tmem_in_byte %d\n",mem_in_byte);
    // printf("\tmem_in_byte_all %d\n", mem_in_byte_all);

    /*
    int _bucket_num1, int _bucket_num2,
    int _cols, int _key_len, int _counter_len,
    int _thres1, int _thres2, 
    int rand_seed1, int rand_seed2
    */

    
    // LRU
    // auto sf_lfu = new SF_LFU<(int)(topk*f)>((mem_in_byte / (8 * 8)) * 0.99, (mem_in_byte / (8 * 8)) * 0.01,
    //                                       8, 32, 32,
    //                                       18, 2,
    //                                       750, 800);

    // RRIP

    auto sf_lfu = new SF_LFU<(int)(topk*f)>((mem_in_byte / (8 * 8)) * 0.9, (mem_in_byte / (8 * 8)) * 0.1,
                                          8, 32, 32,
                                          20, 2,
                                          750, 800);



    auto cf_ss = new SC_SpaceSaving<(int)(topk*1.5), mem_in_byte, L2Thres, 16>();
    auto lf_ss = new SpaceSavingWithLF<(int)(topk*1.5), mem_in_byte>();
    auto ss = new SpaceSaving<(int)(topk*1.5)+mem_in_byte/100>();


    // zipf1.0 2000, 50
    // zipf0.5 390, 50
    // CAIDA 3400, 50
    // webdoc 4000, 50

    auto sf_ss = new SF_SpaceSaving<(int)(topk*1.5)>((mem_in_byte / (8 * 8)) * 0.9, (mem_in_byte / (8 * 8)) * 0.1,
                                          8, 32, 32,
                                          390, 50,
                                          750, 800);
    
    
    timespec dtime1, dtime2;
    long long delay = 0.0;
    
    int k = topk;
    vector<pair<uint32_t, uint32_t>> result(k);
    
    printf("-----------------------------------------------\n");
    
    vector< pair<double, double> > res;
    
    // SF-SS

    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    sf_ss->build(insert_data, packet_num);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    
    {

        /* the original version */
        sf_ss->get_top_k_with_frequency(k, result);
        auto ret = ss_compare_value_with_ground_truth(k, result);
        res.push_back(ret);
        printf("\tLadder Filter + SS \t");
        printf("\t  accuracy %lf, AAE %lf, throughput %lfMops\n", ret.first, ret.second, (double)(1000)*(double)(packet_num)/(double)delay);
    }

    return;


    // SF-LFU

    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    sf_lfu->build(insert_data, packet_num);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    
    {

        /* the original version */
        sf_lfu->get_top_k_with_frequency(k, result);
        auto ret = ss_compare_value_with_ground_truth(k, result);
        res.push_back(ret);
        printf("\tLadder Filter LFU \t");
        printf("\t  accuracy %lf, AAE %lf, throughput %lfMops\n", ret.first, ret.second, (double)(1000)*(double)(packet_num)/(double)delay);
    }

    return;

    // ColdFilter
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    cf_ss->build(insert_data, packet_num);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    
    {
        cf_ss->get_top_k_with_frequency(k, result);
        auto ret = ss_compare_value_with_ground_truth(k, result);
        res.push_back(ret);
        printf("\tCold Filter:\t");
        printf("\t  accuracy %lf, AAE %lf, throughput %lfMops\n", ret.first, ret.second, (double)(1000)*(double)(packet_num)/(double)delay);
        // fprintf(SS_ACC,"%lf,",ret.first);
        // fprintf(SS_AAE,"%lf,",ret.second);
        // fprintf(SS_TPT,"%lf,",(double)(1000)*(double)(packet_num)/(double)delay);
    }
    


    // LogLogFilter
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    lf_ss->build(insert_data, packet_num);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    
    {
        lf_ss->get_top_k_with_frequency(k, result);
        auto ret = ss_compare_value_with_ground_truth(k, result);
        res.push_back(ret);
        printf("\tLogLog Filter:\t");
        printf("\t  accuracy %lf, AAE %lf, throughput %lfMops\n", ret.first, ret.second, (double)(1000)*(double)(packet_num)/(double)delay);
        // fprintf(SS_ACC,"%lf,",ret.first);
        // fprintf(SS_AAE,"%lf,",ret.second);
        // fprintf(SS_TPT,"%lf,",(double)(1000)*(double)(packet_num)/(double)delay);
    }

    // SpaceSaving
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    ss->build(insert_data, packet_num);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    
    {
        ss->get_top_k_with_frequency(k, result);
        auto ret = ss_compare_value_with_ground_truth(k, result);
        res.push_back(ret);
        printf("\tSpaceSaving:\t");
        printf("\t  accuracy %lf, AAE %lf, throughput %lfMops\n", ret.first, ret.second, (double)(1000)*(double)(packet_num)/(double)delay);
        // fprintf(SS_ACC,"%lf\n",ret.first);
        // fprintf(SS_AAE,"%lf\n",ret.second);
        // fprintf(SS_TPT,"%lf\n",(double)(1000)*(double)(packet_num)/(double)delay);
    }

    return;
}

int main(){
    srand((unsigned long long)new char);
    // SS_ACC=fopen("topk1000-a-accuracy.csv","w");
    // SS_AAE=fopen("topk1000-a-AAE.csv","w");
    // SS_TPT=fopen("topk1000-a-throughput.csv","w");

    int packet_num = load_data("../datasets/zipf_0.5.dat");
 
    constexpr int L2Thres = 500;

    demo_ss<(int)(1024*1024*0.2), 1000, L2Thres>(packet_num);
    demo_ss<(int)(1024*1024*0.3), 1000, L2Thres>(packet_num);
    demo_ss<(int)(1024*1024*0.4), 1000, L2Thres>(packet_num);
    demo_ss<(int)(1024*1024*0.5), 1000, L2Thres>(packet_num);
    demo_ss<(int)(1024*1024*0.6), 1000, L2Thres>(packet_num);
    demo_ss<(int)(1024*1024*0.7), 1000, L2Thres>(packet_num);
    demo_ss<(int)(1024*1024*0.8), 1000, L2Thres>(packet_num);
    demo_ss<(int)(1024*1024*0.9), 1000, L2Thres>(packet_num);
    demo_ss<(int)(1024*1024*1.0), 1000, L2Thres>(packet_num);

    
    // fclose(SS_ACC);
    // fclose(SS_AAE);
    // fclose(SS_TPT);

    return 0;
}