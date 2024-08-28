package LadderFilter;
import org.apache.flink.api.common.functions.RichMapFunction;
import org.apache.flink.api.java.DataSet;
import org.apache.flink.api.java.ExecutionEnvironment;
import org.apache.flink.api.java.tuple.Tuple2;

import java.util.ArrayList;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;

public class FlinkLocalExecutionExample {
    public static void main(String[] args) throws Exception {
        // 创建执行环境
        StreamExecutionEnvironment env = StreamExecutionEnvironment.getExecutionEnvironment();

        // env.setRestartStrategy(RestartStrategies.noRestart());
        
        // 在本地执行 Flink 程序
        env.fromElements(1, 2, 3, 4, 5)
            .map(value -> value * 2)
            .print();

        // 执行程序
        env.execute("Flink Local Execution Example");
    }
}
