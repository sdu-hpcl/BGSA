package org.sduhpcl.bgsa.util;

import java.io.File;
import java.util.HashSet;
import java.util.Set;

/**
 *
 * @author zhangjikai
 * @date 16-3-20
 */
public class Configuration {
    
    public enum ArchType {
        NONE, SSE, AVX2, KNC, AVX512
    }
    
    public enum ElementType {
        E8, E16, E32, E64
    }
    
    public static int matchScore = 2;
    public static int mismatchScore = -3;
    public static int indelScore = -5;
    public static String outputDir = (new File("").getAbsolutePath()) + System.getProperty("file.separator") + "generated";
    public static String outputFileName = "align_core.c";
    public static Set<ArchType> archTypes = new HashSet<>();
    public static ElementType elementType = ElementType.E32;
    public static boolean isEdit = false;
    public static boolean isMyers = false;
    public static boolean isPacked = true;
    public static int factor = 1;
    public static boolean isSemiGlobal = false;
    public static boolean isBanded = false;
}
