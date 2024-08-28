package LadderFilter;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.apache.commons.lang3.tuple.Pair;

public class SF_LFU implements java.io.Serializable {

    public LadderFilter_LRU sf;
    public int thres, key_len, bucket_num3, cols;
    public Bucket[] Bucket3;

    public int hash(int key, int n) {// returns 0 ~ n - 1
        return (key % n + n) % n;
    }

    public SF_LFU(int _bucket_num1, int _bucket_num2,
                  int _cols, int _key_len, int _counter_len,
                  int _thres1, int _thres2, int ss_capacity) {
        
        thres = _thres1;
        sf = new LadderFilter_LRU(_bucket_num1, _bucket_num2,
                            _cols, _key_len, _counter_len, _thres2);
        
        key_len = _key_len;
        cols = _cols;
        bucket_num3 = ss_capacity * 100 / (_cols * 8);
        Bucket3 = new Bucket[bucket_num3];
        for (int i = 0; i < bucket_num3; i++) {
            Bucket3[i] = new Bucket(cols);
        }
        
    }

    public void insert(int key) {
        Bucket B3 = Bucket3[hash(key, bucket_num3)];
        
        for (int i = 0; i < cols; ++i) {
            if (B3.fp[i] == key) {
                B3.counter[i] += 1;
                return;
            }
        }

        int res = sf.insert(key, 1);

        if (res >= thres) {
            int min_i = 0, min_c = B3.counter[0];
            for (int i = 1; i < cols; ++i) {
                if (B3.counter[i] < min_c) {
                    min_i = i;
                    min_c = B3.counter[i];
                }
            }
            
            if (res > min_c) {
                B3.fp[min_i] = key;
                B3.counter[min_i] = res;
            }
        }
    }

    public void build(ArrayList<Integer> ips, int n) {
        for (int i = 0; i < n; i++) {
            insert(ips.get(i));
        }
    }

    public void get_top_k_with_frequency(int k, List<Pair<Integer, Integer>> result) {
        result.clear();
        for (int i = 0; i < bucket_num3; i++) {
            for (int j = 0; j < cols; j++) {
                if (Bucket3[i].fp[j] > 0)
                    result.add(Pair.of(Bucket3[i].fp[j], Bucket3[i].counter[j]));
            }
        }

        Collections.sort(result, (p1, p2) -> p2.getValue().compareTo(p1.getValue()));

        if (result.size() > k)
            result.subList(k, result.size()).clear();
    }

}
