package org.sduhpcl.bgsa.util;


/**
 * Created by zhangjikai on 16-9-6.
 */
public class ScoreMsg {

    public static int maxValue = 0;

    public static int midValue = 0;

    public static int minValue = 0;

    public static int maxLength = 0;

    public static int maxSubtractMid = 0;

    public static int mid2minLength = 0;

    public static int maxBitsNum = 0;

    public static void calValues(){
        maxValue = Configuration.matchScore - Configuration.indelScore;
        midValue = Configuration.mismatchScore - Configuration.indelScore;
        minValue = Configuration.indelScore;
        maxLength = maxValue - minValue;
        maxSubtractMid = maxValue - midValue;
        mid2minLength = midValue - minValue + 1;
        maxBitsNum = getNumBits(maxLength + 1);
    }

    public static int getNumBits(int value) {
        int i;
        for (i = 0; ; i++) {
            if (Math.pow(2, i) >= value) {
                break;
            }
        }
        return i + 1;
    }

}
