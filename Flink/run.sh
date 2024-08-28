for i in {1..10}
do
   flink run -p 5 -c LadderFilter.Main target/flink-1.5-SNAPSHOT.jar >> parallel_5_5workers.txt
done