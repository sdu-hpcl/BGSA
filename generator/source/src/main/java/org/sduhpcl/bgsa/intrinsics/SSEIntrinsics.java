package org.sduhpcl.bgsa.intrinsics;

import java.util.List;

/**
 * Created by zhangjikai on 16-9-6.
 */
public class SSEIntrinsics extends BaseIntrinsics {
    
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
        return result + " = _mm_and_si128(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opAnd(String result, String operand) {
        countOp();
        return result + " = _mm_and_si128(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opXor(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm_xor_si128(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opXor(String result, String operand) {
        countOp();
        return result + " = _mm_xor_si128(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opOr(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm_or_si128(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opOr(String result, String operand) {
        countOp();
        return result + " = _mm_or_si128(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opNot(String result, String operand) {
        countOp();
        return result + " = _mm_xor_si128(" + operand + ", all_ones);";
    }
    
    @Override
    public String opRightShift(String result, String operand, String shiftLen) {
        countOp();
        return result + " = _mm_srli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opRightShift(String operand, String shiftLen) {
        countOp();
        return operand + " = _mm_srli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opLeftShift(String result, String operand, String shiftLen) {
        countOp();
        return result + " = _mm_slli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opLeftShift(String operand, String shiftLen) {
        countOp();
        return operand + " = _mm_slli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opAdd(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm_add_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opAdc(String result, String operand, String carryBit, String operand2, String carryStore) {
        return null;
    }
    
    @Override
    public String opAdd(String result, String operand) {
        countOp();
        return result + " = _mm_add_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opMultiply(String result, String operand, String operand2) {
        return result + " = _mm_mullo_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opMultiply(String result, String operand) {
        return result + " = _mm_mullo_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opSubtract(String result, String operand, String operand2) {
        return result + " = _mm_sub_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opSubtract(String result, String operand) {
        return result + " = _mm_sub_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opSetValue(String result, String value) {
        if (element.getBitSize() == 64) {
            return result + " = _mm_set1_epi" + element.getBitSize() + "x(" + value + ");";
        }
        
        return result + " = _mm_set1_epi" + element.getBitSize() + "(" + value + ");";
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
        if (element.getBitSize() == 64) {
            builder.append("_mm_set1_epi").append(element.getBitSize()).append("x(");
        } else {
            builder.append("_mm_set1_epi").append(element.getBitSize()).append("(");
        }
        
        builder.append(value);
        builder.append(");");
        return builder.toString();
    }
    
    @Override
    public String opLoad(String result, String operand) {
        return result + " = _mm_load_si128( (__m128i *)" + operand + ");";
    }
    
    @Override
    public String opMax(String result, String operand, String operand2) {
        int bitSize = element.getBitSize();
        if (bitSize == 64) {
            bitSize = 32;
        }
        return result + "= _mm_max_epi" + bitSize + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opMin(String result, String operand, String operand2) {
        int bitSize = element.getBitSize();
        if (bitSize == 64) {
            bitSize = 32;
        }
        return result + "= _mm_min_epi" + bitSize + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opCmpgt(String result, String operand, String operand2) {
        return result + "= _mm_cmpgt_epi" + element.getBitSize() +  "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opCmpEpu(String result, String operand, String operand2, String type) {
        return null;
    }
}

