#pragma GCC optimize("O3")
#include <iostream>
#include <fstream>
#include <iomanip>
#include <time.h>
#include <unordered_map>
#include <algorithm>
#include <chrono>

#include "abstract.h"
#include "Waving.h"
#include "Waving_Pure.h"
#include <vector>

using namespace std;

#define BLOCK 10000
const int interval = 10000000;

typedef std::chrono::high_resolution_clock::time_point TP;
inline TP now()
{
    return std::chrono::high_resolution_clock::now();
}

const string FOLDER = "./";

const string FILE_NAME{"../datasets/130000.dat"};

void Test_Hitter(string PATH);
void Test_Speed(string PATH);

int HIT;

int Get_TopK(HashMap mp, int k)
{
    int size = mp.size();
    int *num = new int[size];
    int pos = 0;
    HashMap::iterator it;
    for (it = mp.begin(); it != mp.end(); ++it)
    {
        num[pos++] = it->second;
    }
    nth_element(num, num + size - k, num + size);
    int ret = num[size - k];
    delete num;
    return ret;
}

vector<Data> all_from;
vector<Data> all_to;
ofstream fout("result.txt", ios::app);
HashMap mp;
StreamMap sp;

int main()
{
    fout << "Mem"
         << "\t"
         << "Alg"
         << "\t"
         << "f1"
         << "\t"
         << "pr"
         << "\t"
         << "cr"
         << "\t"
         << "aae"
         << "\t"
         << "are" << endl;
    all_from.clear();
    all_to.clear();
    // ===================== Determine the threshold==================
    cout << FILE_NAME << endl;

    FILE *file = fopen((FOLDER + FILE_NAME).c_str(), "rb");
    Data from;
    Data to;
    uint num = 0;
    char tmp[20];
    TP start = now();
    while (1)
    {
        int m = fread(from.str, DATA_LEN, 1, file);
        int n = fread(to.str, DATA_LEN, 1, file);

        char padding[13];

        fread(padding, 13, 1, file);

        all_from.push_back(from);
        all_to.push_back(to);

        if (m != 1 || n != 1)
            break;
        Stream stream(from, to);
        if (sp.find(stream) == sp.end())
        {
            sp[stream] = 1;
            if (mp.find(from) == mp.end())
                mp[from] = 1;
            else
                mp[from] += 1;
        }
        num++;
    }
    TP finish = now();

    double duration = (double)std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1000000>>>(finish - start).count();
    cout << "Read File: " << (num + 0.0) / duration << endl;

    cout << "True Distinct Item: " << sp.size() << endl;
    cout << "Total Packet Number: " << num << endl;
    HIT = Get_TopK(mp, 500);
    printf("HIT=%d\n", HIT);
    fclose(file);

    // printf("\033[0m\033[1;32m====================================================\n\033[0m");
    // printf("\033[0m\033[1;32m|         Application: Find Super Spreader         |\n\033[0m");
    // printf("\033[0m\033[1;32m====================================================\n\033[0m");
    // printf("\033[0m\033[1;32m|                     F1 SCORE                     |\n\033[0m");
    // printf("\033[0m\033[1;32m====================================================\n\033[0m");
    // Test_Hitter(FOLDER + FILE_NAME);

    printf("\033[0m\033[1;32m====================================================\n\033[0m");
    printf("\033[0m\033[1;32m|                    THROUGHPUT                    |\n\033[0m");
    printf("\033[0m\033[1;32m====================================================\n\033[0m");
    Test_Speed(FOLDER + FILE_NAME);
    printf("\033[0m\033[1;32m====================================================\n\033[0m");
}

void Test_Hitter(string PATH)
{
    // Pure Baseline
    for (int i = 2; i <= 20; i += 2)
    {
        int memory = BLOCK * (i);

        // Bloom Filter Size: 6M Memory
        int BF_memory = 6000000;

        int SKETCH_NUM = 1;
        Abstract *sketch[SKETCH_NUM];

        sketch[0] = new WavingSketch_P<16, 8>(memory, HIT, BF_memory);

        Data from;
        Data to;
        int num = 0;

        // SKETCH_NUM = 1;
        num = all_from.size();
        for (int kkk = 0; kkk < num; kkk++)
        {
            auto from = all_from[kkk];
            auto to = all_to[kkk];
            for (int j = 0; j < SKETCH_NUM; ++j)
            {
                sketch[j]->Init(from, to);
            }
        }
        cout << num << endl;

        for (int j = 0; j < SKETCH_NUM; ++j)
        {
            sketch[j]->Check(mp);
            printf("\033[0m\033[1;36m|\033[0m\t");
            fout << memory << '\t';
            sketch[j]->print_f1(fout, memory);
            sketch[j]->print_aae(fout, memory);
            sketch[j]->print_info(fout);
        }

        for (int j = 0; j < SKETCH_NUM; ++j)
        {
            delete sketch[j];
        }
    }

    int admit_thr = 3;
    int partition = 3;
    for (int i = 2; i <= 20; i += 2)
    {
        int memory = BLOCK * i;

        // Bloom Filter Size: 6M Memory
        int BF_memory = 6000000;
        int sf_memory = (memory * partition * 1.0 / 10.0);
        int sf_bucket_num = sf_memory / 24;

        // here

        // LRU

        // uint32_t l1_bucket_num = int(sf_bucket_num * 0.99);
        // uint32_t l2_bucket_num = int(sf_bucket_num * 0.01);

        // RRIP
        uint32_t l1_bucket_num = int(sf_bucket_num * 0.1);
        uint32_t l2_bucket_num = int(sf_bucket_num * 0.9);

        // Note that l2 = 0, we only use one layer in Super Spreader scenarios

        uint32_t total_bucket_num = l1_bucket_num + l2_bucket_num;

        uint32_t l1_thres = 18;
        uint32_t l2_thres = 2;

        printf("\033[0m\033[1;4;36m> Memory size: %dKB\n\033[0m", memory / 1000);

        int SKETCH_NUM = 1;
        Abstract *sketch[SKETCH_NUM];

        sketch[0] = new WavingSketch<16, 8>(memory - 24 * total_bucket_num, HIT, BF_memory, l1_bucket_num, l1_thres, l2_bucket_num, l2_thres, 0, admit_thr);

        Data from;
        Data to;
        int num = 0;

        num = all_from.size();
        for (int kkk = 0; kkk < num; kkk++)
        {
            auto from = all_from[kkk];
            auto to = all_to[kkk];
            for (int j = 0; j < SKETCH_NUM; ++j)
            {
                sketch[j]->Init(from, to);
            }
        }
        cout << num << endl;

        for (int j = 0; j < SKETCH_NUM; ++j)
        {
            sketch[j]->Check(mp);
            printf("\033[0m\033[1;36m|\033[0m\t");
            fout << memory << '\t';
            sketch[j]->print_f1(fout, memory);
            sketch[j]->print_aae(fout, memory);
            sketch[j]->print_info(fout);
        }

        for (int j = 0; j < SKETCH_NUM; ++j)
        {
            delete sketch[j];
        }
    }
}

void Test_Speed(string PATH)
{
    const int CYCLE = 1;
    Data from;
    Data to;

    for (int i = 20; i <= 20; ++i)
    {
        int memory = BLOCK * i;
        int BF_memory = 5000000;
        int bucket_num = 1000;

        printf("\033[0m\033[1;4;36m> Memory size: %dKB\n\033[0m", memory / 1000);
        for (int k = 0; k < CYCLE; ++k)
        {
            int admit_thr = 3;
            int partition = 3;
            int sf_memory = (memory * partition * 1.0 / 10.0);
            int sf_bucket_num = sf_memory / 24;

            uint32_t l1_bucket_num = int(sf_bucket_num * 1.0);
            // Note that l2 = 0, we only use one layer in Super Spreader scenarios
            uint32_t total_bucket_num = l1_bucket_num;
            uint32_t l1_thres = 1;

            printf("HERE1\n");

            WavingSketch<16, 8> sketch(memory - 24 * total_bucket_num, HIT, BF_memory, l1_bucket_num, l1_thres, 0, 0, 0, admit_thr);

            printf("HERE1.5\n");
            int num = 0;

            TP start = now();
            num = all_from.size();
            for (int i = 0; i < num; i++)
            {
                sketch.Init(all_from[i], all_to[i]);
            }

            TP finish = now();
            printf("HERE2\n");
            double duration = (double)std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1000000>>>(finish - start).count();
            printf("\033[0m\033[1;36m|\033[0m\t");
            printf("HERE3\n");
            cout << sketch.name << sketch.sep << (num + 0.0) / duration << endl;
            fout << sketch.name << sketch.sep << (num + 0.0) / duration << endl;
        }
        for (int k = 0; k < CYCLE; ++k)
        {
            WavingSketch_P<16, 8> sketch(memory, HIT, BF_memory);
            int num = 0;

            TP start = now();
            num = all_from.size();
            for (int i = 0; i < num; i++)
            {
                sketch.Init(all_from[i], all_to[i]);
            }
            TP finish = now();
            double duration = (double)std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1000000>>>(finish - start).count();
            printf("\033[0m\033[1;36m|\033[0m\t");
            cout << sketch.name << sketch.sep << (num + 0.0) / duration << endl;
            fout << sketch.name << sketch.sep << (num + 0.0) / duration << endl;
        }
    }
}