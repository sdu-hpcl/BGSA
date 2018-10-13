package org.sduhpcl.bgsa.generator;

import org.sduhpcl.bgsa.intrinsics.BaseIntrinsics;
import org.sduhpcl.bgsa.arch.SIMDArch;
import org.sduhpcl.bgsa.util.Configuration;
import org.sduhpcl.bgsa.util.FileUtils;
import org.sduhpcl.bgsa.util.ScoreMsg;

/**
 *
 * @author zhangjikai
 * @date 16-9-6
 */
public abstract class BaseGenerator {

    protected FileUtils fileUtils;

    protected int matchScore = 0;
    protected int mismatchScore = 0;
    protected int indelScore = 0;


    protected int maxValue;
    protected int midValue;
    protected int minValue;

    protected int maxLength;
    protected int maxSubtractMid;
    protected int mid2minLength;

    protected int maxBitsNum;

    protected String wordType;

    protected String readType;
    
    protected String resultType;

    protected String writeType;

    protected String allOnes;

    protected String carryBitmask;

    protected String highOne;

    protected String nextHighOne;

    protected String bitMask;

    protected String bitMaskStr;

    protected String declareStr;

    protected String vNumStr;

    protected String wordStr;

    protected String oneStr;

    protected boolean isCarry;

    protected boolean isMic;

    protected BaseIntrinsics baseIntrinsics;

    protected SIMDArch arch;

    public abstract void genKernel();

    public void setArch(SIMDArch arch) {
        this.arch = arch;
        this.baseIntrinsics = arch.getBaseIntrinsics();
        arch.initValues(this);
    }

    protected void initValues() {
        matchScore = Configuration.matchScore;
        mismatchScore = Configuration.mismatchScore;
        indelScore = Configuration.indelScore;
        maxValue = ScoreMsg.maxValue;
        midValue = ScoreMsg.midValue;
        minValue = ScoreMsg.minValue;
        maxLength = ScoreMsg.maxLength;
        maxSubtractMid = ScoreMsg.maxSubtractMid;
        mid2minLength = ScoreMsg.mid2minLength;
        maxBitsNum = ScoreMsg.maxBitsNum;
    }
}
