package LadderFilter;

public class LadderFilterTest {
    public static void main(String[] args) {
        
        String filePath = "src/main/resources/130000.dat";


        int count = 0;

        // try (FileInputStream fis = new FileInputStream(filePath)) {
        //     byte[] buffer = new byte[13];

        //     while (fis.available() > 0) {
        //         int bytesRead = fis.read(buffer, 0, 13);
        //         if (bytesRead != 13) {
        //             System.out.println("文件未能正确读取13个字节");
        //             break;
        //         }

        //         count++;


        //         long skipped = fis.skip(8);
        //         if (skipped != 8) {
        //             System.out.println("文件未能正确跳过8个字节");
        //             break;
        //         }
        //     }

        // } catch (IOException e) {
        //     e.printStackTrace();
        // }
        System.out.println("HERE !!!");
        System.out.println(count);
    }
}
