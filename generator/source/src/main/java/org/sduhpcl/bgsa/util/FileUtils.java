package org.sduhpcl.bgsa.util;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

/**
 * @author zhangjikai
 * @date 16-3-14
 */
public class FileUtils {
    
    private static final int OPEN_FILE_SUCCESS = 1;
    private static final int OPEN_FILE_FAILURE = 0;
    
    private static FileUtils fileUtils;
    
    
    private int marginLeft = 0;
    
    private BufferedWriter bufferedWriter;
    
    private FileUtils() {
    }
    
    private FileUtils(String fileName) {
        openFile(fileName);
    }
    
    private FileUtils(String dirName, String fileName) {
        openFile(dirName, fileName);
    }
    
    
    public static FileUtils newInstance() {
        fileUtils = new FileUtils(Configuration.outputDir, Configuration.outputFileName);
        return fileUtils;
    }
    
    public static FileUtils newInstance(String fileName) {
        fileUtils = new FileUtils(fileName);
        return fileUtils;
    }
    
    public static FileUtils newInstance(String dirName, String fileName) {
        fileUtils = new FileUtils(dirName, fileName);
        return fileUtils;
    }
    
    public static FileUtils getInstance() {
        if (fileUtils == null) {
            fileUtils = new FileUtils(Configuration.outputDir, Configuration.outputFileName);
        }
        return fileUtils;
    }
    
    
    public int openFile(String fileName) {
        try {
            bufferedWriter = new BufferedWriter(new FileWriter(fileName));
            return OPEN_FILE_SUCCESS;
        } catch (IOException e) {
            System.err.println("Can't create file:" + fileName);
            e.printStackTrace();
            System.exit(-1);
            return OPEN_FILE_FAILURE;
        }
    }
    
    public void setMarignLeft(int margin) {
        marginLeft = margin;
    }
    
    public int getMarginLeft() {
        return marginLeft;
    }
    
    public int openFile(String dirName, String fileName) {
        File dir = new File(dirName);
        if (!dir.exists() && !dir.isDirectory()) {
            boolean success = dir.mkdirs();
            if (!success) {
                System.err.println("Can't create folder:" + dir);
                System.exit(-1);
            }
        }
        
        try {
            bufferedWriter = new BufferedWriter(new FileWriter(dirName + System.getProperty("file.separator") + fileName));
            return OPEN_FILE_SUCCESS;
        } catch (IOException e) {
            System.err.println("Can't create file:" + fileName);
            e.printStackTrace();
            System.exit(-1);
            return OPEN_FILE_FAILURE;
        }
    }
    
    public void blankLine() {
        writeContent("", 0);
    }
    
    
    public void writeContent(String content) {
        writeContent(content, marginLeft);
    }
    
    public void writeContent(String content, int level) {
        if (bufferedWriter == null) {
            return;
        }
        try {
            for (int i = 0; i < level; i++) {
                bufferedWriter.write("    ");
            }
            bufferedWriter.write(content);
            bufferedWriter.newLine();
            bufferedWriter.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    
    public void writeContentWithoutNewline(String content, int level) {
        if (bufferedWriter == null) {
            return;
        }
        try {
            for (int i = 0; i < level; i++) {
                bufferedWriter.write("    ");
            }
            bufferedWriter.write(content);
            bufferedWriter.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    
    public void closeFile() {
        if (bufferedWriter != null) {
            try {
                bufferedWriter.close();
                bufferedWriter = null;
            } catch (IOException e) {
                e.printStackTrace();
            }
            
        }
    }
}
