package LadderFilter;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import org.apache.flink.api.java.tuple.Tuple2;
import org.apache.flink.streaming.api.functions.KeyedProcessFunction;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;
import org.apache.flink.api.common.functions.MapFunction;
import org.apache.flink.streaming.api.datastream.DataStream;
import org.apache.flink.util.Collector;


public class Main {

    static final int topk = 1000;
    static final int mem_in_kb_all = 1000;
    static final int mem_in_byte_all = mem_in_kb_all * 1024;
    static final int mem_in_byte = mem_in_byte_all - (int)(1.5 * topk * 100);

    static final int bucket_num1 = (int)((mem_in_byte / (8 * 8)) * 0.99);
    static final int bucket_num2 = (int)((mem_in_byte / (8 * 8)) * 0.01);
    static final int cols = 8;
    static final int key_len = 32;
    static final int counter_len = 32;
    static final int thres1 = 20;
    static final int thres2 = 2;

    static int packet_count = 0;
    
    
    static ArrayList<Integer> ips = new ArrayList<Integer>();

    public static class RunTest extends KeyedProcessFunction<Integer, Integer, Integer>{
        SF_LFU SF = new SF_LFU(bucket_num1, bucket_num2,
                            cols, key_len, counter_len,
                            thres1, thres2, (int)(1.5 * topk));
        @Override
        public void processElement(
                Integer item,
                Context ctx,
                Collector<Integer> collector) throws Exception {
            // process each element in the stream
            // System.out.printf("inserting: %d\n", item);
            SF.insert(item);
        }
    }

    public static void main(String[] args) throws Exception {
        
        String filePath = "resources/130000.dat";

        try (FileInputStream fis = new FileInputStream(filePath)) {
            byte[] buffer = new byte[13];

            while (fis.available() > 0) {
                int bytesRead = fis.read(buffer, 0, 13);
                if (bytesRead != 13) {
                    System.out.println("文件未能正确读取13个字节");
                    break;
                }

                int ip = (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
                ips.add(ip);

                packet_count++;

                // if (packet_count > 100000)
                //     break;

                long skipped = fis.skip(8);
                if (skipped != 8) {
                    System.out.println("文件未能正确跳过8个字节");
                    break;
                }
            }

        } catch (IOException e) {
            e.printStackTrace();
        }


        System.out.printf("packet count: %d\n", packet_count);

        
        StreamExecutionEnvironment env = StreamExecutionEnvironment.getExecutionEnvironment();
        // env.setParallelism(2);
      
        long start_time = System.nanoTime();

        DataStream<Integer> intStream = env.fromCollection(ips);

        DataStream<Integer> resultStream = intStream
            .keyBy(value -> value)
            .process(new RunTest());

        resultStream.print();
        
        env.execute();

        long end_time = System.nanoTime();
        long sf_duration = end_time - start_time;

        System.out.printf("SF throughput: %f Mops\n", (double)1000 * packet_count / sf_duration);
        
    }

    

}
