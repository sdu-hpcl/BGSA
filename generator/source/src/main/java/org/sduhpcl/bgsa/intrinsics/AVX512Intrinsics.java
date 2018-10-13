package org.sduhpcl.bgsa.intrinsics;

import java.util.List;

/**
 * Created by zhangjikai on 16-9-6.
 */
public class AVX512Intrinsics extends BaseIntrinsics {
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
        return result + " = _mm512_and_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opAnd(String result, String operand) {
        countOp();
        return result + " = _mm512_and_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opXor(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm512_xor_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opXor(String result, String operand) {
        countOp();
        return result + " = _mm512_xor_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opOr(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm512_or_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opOr(String result, String operand) {
        countOp();
        return result + " = _mm512_or_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opNot(String result, String operand) {
        countOp();
        return result + " = _mm512_xor_epi" + element.getBitSize() + "(" + operand + ", all_ones);";
    }
    
    @Override
    public String opRightShift(String result, String operand, String shiftLen) {
        countOp();
        return result + " = _mm512_srli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opRightShift(String operand, String shiftLen) {
        countOp();
        return operand + " = _mm512_srli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opLeftShift(String result, String operand, String shiftLen) {
        countOp();
        return result + " = _mm512_slli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opLeftShift(String operand, String shiftLen) {
        countOp();
        return operand + " = _mm512_slli_epi" + element.getBitSize() + "(" + operand + ", " + shiftLen + ");";
    }
    
    @Override
    public String opAdd(String result, String operand, String operand2) {
        countOp();
        return result + " = _mm512_add_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opAdc(String result, String operand, String carryBit, String operand2, String carryStore) {
        countOp();
        return result + " = _mm512_adc_epi" + element.getBitSize() + "(" + operand + "," + carryBit + "," + operand2 + "," + carryStore + ");";
    }
    
    @Override
    public String opAdd(String result, String operand) {
        countOp();
        return result + " = _mm512_add_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opMultiply(String result, String operand, String operand2) {
        return result + " = _mm512_mullo_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opMultiply(String result, String operand) {
        return result + " = _mm512_mullo_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opSubtract(String result, String operand, String operand2) {
        return result + " = _mm512_sub_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opSubtract(String result, String operand) {
        return result + " = _mm512_sub_epi" + element.getBitSize() + "(" + result + ", " + operand + ");";
    }
    
    @Override
    public String opSetValue(String result, String value) {
        return result + " = _mm512_set1_epi" + element.getBitSize() + "(" + value + ");";
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
        builder.append("_mm512_set1_epi" + element.getBitSize() + "(");
        builder.append(value);
        builder.append(");");
        return builder.toString();
    }
    
    @Override
    public String opLoad(String result, String operand) {
        return result + " = _mm512_load_epi" + element.getBitSize() + "(" + operand + ");";
    }
    
    @Override
    public String opMax(String result, String operand, String operand2) {
        return result + " = _mm512_max_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opMin(String result, String operand, String operand2) {
        return result + " = _mm512_min_epi" + element.getBitSize() + "(" + operand + ", " + operand2 + ");";
    }
    
    @Override
    public String opCmpgt(String result, String operand, String operand2) {
        return null;
    }
    
    @Override
    public String opCmpEpu(String result, String operand, String operand2, String type) {
        
        return String.format("%s = _mm512_cmp_epu%s_mask(%s, %s, %s);", result, element.getBitSize(),
                operand, operand2, type);
    }
}
