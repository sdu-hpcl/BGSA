package org.sduhpcl.bgsa.util;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by zhangjikai on 16-9-7.
 */
public class GeneratorUtils {
    
    public static void genBitIncluded() {
        FileUtils fileUtils = FileUtils.getInstance();
        List<String> includeHeaders = new ArrayList<>();
        includeHeaders.add("stdio.h");
        includeHeaders.add("stdlib.h");
        includeHeaders.add("stdint.h");
        includeHeaders.add("string.h");
        includeHeaders.add("omp.h");
        includeHeaders.add("pthread.h");
        includeHeaders.add("sys/stat.h");
        includeHeaders.add("sys/types.h");
        includeHeaders.add("unistd.h");
        for (int i = 0; i < includeHeaders.size(); i++) {
            fileUtils.writeContent("#include <" + includeHeaders.get(i) + ">", 0);
        }
        
        includeHeaders.clear();
        includeHeaders.add("cal.h");
        includeHeaders.add("align_core.h");
        for (int i = 0; i < includeHeaders.size(); i++) {
            fileUtils.writeContent("#include \"" + includeHeaders.get(i) + "\"", 0);
        }
        fileUtils.blankLine();
    }
    
    public static void genBitGlobal() {
        FileUtils fileUtils = FileUtils.getInstance();
        int matchScore = Configuration.matchScore;
        int mismatchScore = Configuration.mismatchScore;
        int indelScore = Configuration.indelScore;
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent("int match_score = " + matchScore + ";");
        fileUtils.writeContent("int mismatch_score = " + mismatchScore + ";");
        fileUtils.writeContent("int gap_score = " + indelScore + ";");
        int dvdhLen = 16;
        if (!Configuration.isMyers) {
            int maxLength = matchScore - 2 * indelScore + 1;
            int maxBitNum = ScoreMsg.getNumBits(maxLength);
            if (Configuration.isPacked) {
                if (dvdhLen < maxBitNum * 2) {
                    dvdhLen = maxBitNum * 2;
                }
                
            } else {
                if (dvdhLen < maxLength * 2) {
                    dvdhLen = maxLength * 2;
                }
            }
        }
        if (Configuration.archTypes.contains(Configuration.ArchType.KNC)) {
            fileUtils.writeContent("__ONMIC__  int dvdh_len = " + dvdhLen + ";");
        } else {
            fileUtils.writeContent("int dvdh_len = " + dvdhLen + ";");
        }
        
        if(Configuration.isSemiGlobal && Configuration.isMyers) {
            fileUtils.writeContent("int full_bits = 1;");
        } else {
            fileUtils.writeContent("int full_bits = 0;");
        }
        fileUtils.blankLine();
    }
    
    
    /**
     * 将整形转为补码字符串
     */
    public static String convertIntToComplementCode(int number) {
        number = (~Math.abs(number)) + 1;
        return Integer.toBinaryString(number);
    }
}
