package LadderFilter;

import static java.lang.Integer.min;
import java.util.Collections;
import java.util.List;

import org.apache.commons.lang3.tuple.Pair;

public class LadderFilter_LRU implements java.io.Serializable {
    private Bucket[] Bucket1;
    private Bucket[] Bucket2;
    private final int bucket_num1, bucket_num2;
    private final int cols, key_len, counter_len;
    private final int thres2;

    public LadderFilter_LRU(int _bucket_num1, int _bucket_num2,
                        int _cols, int _key_len, int _counter_len,
                        int _thres2) {
        Bucket1 = new Bucket[_bucket_num1];
        Bucket2 = new Bucket[_bucket_num2];
        bucket_num1 = _bucket_num1;
        bucket_num2 = _bucket_num2;
        cols = _cols;
        key_len = _key_len;
        counter_len = _counter_len;
        thres2 = _thres2;

        
        for (int i = 0; i < bucket_num1; i++) {
            Bucket1[i] = new Bucket(cols);
        }
        
        for (int i = 0; i < bucket_num2; i++) {
            Bucket2[i] = new Bucket(cols);
        }

    }

    public int getFP(int key) {
        return key;
    }

    public int hash(int key, int n) {// returns 0 ~ n - 1
        return (key % n + n) % n;
    }

    public int insert(int key, int count) {
        System.out.printf("key = %d\n", key);
        int keyfp = getFP(key);
        
        /* if the item is in B1 */
        Bucket B1 = Bucket1[hash(keyfp, bucket_num1)];
        for (int i = 0; i < cols; i++) {
            if (B1.fp[i] == keyfp) {
                B1.permutation(i);
                B1.counter[0] = min(B1.counter[0] + count, (int)((1L << counter_len) - 1));
                return B1.counter[0];
            }
        }

        /* if the item is in B2 */
        Bucket B2 = Bucket2[hash(keyfp, bucket_num2)];
        for (int i = 0; i < cols; i++) {
            if (B2.fp[i] == keyfp) {
                B2.permutation(i);
                B2.counter[0] = min(B2.counter[0] + count, (int)((1L << counter_len) - 1));
                return B2.counter[0];
            }
        }

        /* if B1 is not full */
        for (int i = 0; i < cols; ++i) {
            if (B1.fp[i] == 0) {
                B1.permutation(i);
                B1.fp[0] = keyfp;
                B1.counter[0] = count;
                return B1.counter[0];
            }
        }

        /* dequeue the LRU item in B1, if is promising item, insert to B2 */
        if (B1.counter[cols - 1] >= thres2) {
            /* if Bucket2 is not full*/
            boolean isFull = true;
            for (int i = 0; i < cols; ++i) {
                if (B2.fp[i] == 0) {
                    B2.permutation(i);
                    B2.fp[0] = B1.fp[cols - 1];
                    B2.counter[0] = B1.counter[cols - 1];
                    isFull = false;
                    break;
                }
            }
            if (isFull) {
                B2.permutation(cols - 1);
                B2.fp[0] = B1.fp[cols - 1];
                B2.counter[0] = B1.counter[cols - 1];
            }
        }

        /* insert the item to the empty cell */
        B1.permutation(cols - 1);
        B1.fp[0] = keyfp;
        B1.counter[0] = count;
        return B1.counter[0];
    }

    public void get_top_k_with_frequency(int k, List<Pair<Integer, Integer>> result) {
        result.clear();
        for (int i = 0; i < bucket_num1; i++) {
            for (int j = 0; j < cols; j++) {
                if (Bucket1[i].fp[j] > 0)
                    result.add(Pair.of(Bucket1[i].fp[j], Bucket1[i].counter[j]));
            }
        }
        
        for (int i = 0; i < bucket_num2; i++) {
            for (int j = 0; j < cols; j++) {
                if (Bucket2[i].fp[j] > 0)
                    result.add(Pair.of(Bucket2[i].fp[j], Bucket2[i].counter[j]));
            }
        }

        Collections.sort(result, (p1, p2) -> p2.getValue().compareTo(p1.getValue()));

        if (result.size() > k)
            result.subList(k, result.size()).clear();
    }

    public int query(int key) {
        
        int keyfp = getFP(key);
        Bucket B1 = Bucket1[hash(keyfp, bucket_num1)];
        for (int i = 0; i < cols; i++) {
            if (B1.fp[i] == keyfp)
                return B1.counter[i];
        }
        Bucket B2 = Bucket2[hash(keyfp, bucket_num2)];
        for (int i = 0; i < cols; i++) {
            if (B2.fp[i] == keyfp)
                return B2.counter[i];
        }
        return 0;
    }
}
