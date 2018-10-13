package org.sduhpcl.bgsa.intrinsics;

import java.util.List;

/**
 * Created by zhangjikai on 17-3-6.
 */
public class AVX2Intrinsics extends BaseIntrinsics {
    private static int opBitNum = 0;
    
    public static int getOpBitNum() {
        return opBitNum;
    }
    
    private void countOp() {
        opBitNum++;
    }
    
    @Override
    public String opAnd(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm256_and_si256(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opAnd(String result, String operand) {
        countOp();
        return result + " = _mm256_and_si256(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opXor(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm256_xor_si256(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opXor(String result, String operand) {
        countOp();
        return result + " = _mm256_xor_si256(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opOr(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm256_or_si256(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opOr(String result, String operand) {
        countOp();
        return result + " = _mm256_or_si256(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opNot(String result, String operand) {
        countOp();
        return result + " = _mm256_xor_si256(" + operand + ", all_ones);";
    }
    
    @Override
    public String opRightShift(String result, String operand, String shiftLen) {
        countOp();
        return result + " = _mm256_srli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opRightShift(String operand, String shiftLen) {
        countOp();
        return operand + " = _mm256_srli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opLeftShift(String result, String operand, String shiftLen) {
        countOp();
        return result + " = _mm256_slli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opLeftShift(String operand, String shiftLen) {
        countOp();
        return operand + " = _mm256_slli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opAdd(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm256_add_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opAdc(String result, String operand, String carryBit, String operand2, String carryStore) {
        return null;
    }
    
    @Override
    public String opAdd(String result, String operand) {
        countOp();
        return result + " = _mm256_add_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opMultiply(String result, String operand, String operand2) {
        return result + " = _mm256_mullo_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opMultiply(String result, String operand) {
        return result + " = _mm256_mullo_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opSubtract(String result, String operand, String operand2) {
        return result + " = _mm256_sub_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opSubtract(String result, String operand) {
        return result + " = _mm256_sub_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opSetValue(String result, String value) {
        
        if(element.getBitSize() == 64) {
            return result + " = _mm256_set1_epi" + element.getBitSize() + "x(" + value + ");";
        }
        return result + " = _mm256_set1_epi" + element.getBitSize() + "(" + value + ");";
    }
    
    @Override
    public String opEqualValue(String result, String value) {
        return result + " = " + value + ";";
    }
    
    @Override
    public String opSetValue(List<String> results, String value) {
        StringBuilder builder = new StringBuilder();
        
        for (String s : results) {
            builder.append(s);
            builder.append(" = ");
        }
        if(element.getBitSize() == 64) {
            builder.append("_mm256_set1_epi" + element.getBitSize() + "x(");
        } else {
            builder.append("_mm256_set1_epi" + element.getBitSize() + "(");
        }
        
        builder.append(value);
        builder.append(");");
        return builder.toString();
    }
    
    @Override
    public String opLoad(String result, String operand) {
        return result + " = _mm256_load_si256( (__m256i *)" + operand + ");";
    }
    
    @Override
    public String opMax(String result, String operand, String operand2) {
        
        int bitSize = element.getBitSize();
        if (bitSize == 64) {
            bitSize = 32;
        }
        return result + " = _mm256_max_epi" + bitSize + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opMin(String result, String operand, String operand2) {
        int bitSize = element.getBitSize();
        if (bitSize == 64) {
            bitSize = 32;
        }
        return result + " = _mm256_min_epi" + bitSize + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opCmpgt(String result, String operand, String operand2) {
        return result + "= _mm256_cmpgt_epi" + element.getBitSize() +  "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opCmpEpu(String result, String operand, String operand2, String type) {
        return null;
    }
}
