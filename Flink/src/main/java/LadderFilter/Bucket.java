package LadderFilter;

public class Bucket implements java.io.Serializable {

    public int[] fp;
    public int[] counter;

    public Bucket(int cols) {
        fp = new int[cols];
        counter = new int[cols];
    }

    public void permutation(int p) {
        for (int i = p, tmp; i > 0; i--) {
            tmp = fp[i];
            fp[i] = fp[i - 1];
            fp[i - 1] = tmp;
            tmp = counter[i];
            counter[i] = counter[i - 1];
            counter[i - 1] = tmp;
        }
    }
    
}
