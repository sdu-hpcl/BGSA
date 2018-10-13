package org.sduhpcl.bgsa.intrinsics;

import java.util.List;

/**
 * @author zhangjikai
 * @date 16-9-6
 */
public class CPUIntrinsics extends BaseIntrinsics {
    
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
        return result + " = " + operand + " & " + operand2 + ";";
    }
    
    @Override
    public String opAnd(String result, String operand) {
        countOp();
        return result + " &= " + operand + ";";
    }
    
    @Override
    public String opXor(String result, String operand, String operand2) {
        countOp();
        return result + " = " + operand + " ^ " + operand2 + ";";
    }
    
    @Override
    public String opXor(String result, String operand) {
        countOp();
        return result + " ^= " + operand + ";";
    }
    
    @Override
    public String opOr(String result, String operand, String operand2) {
        countOp();
        return result + " = " + operand + " | " + operand2 + ";";
    }
    
    @Override
    public String opOr(String result, String operand) {
        countOp();
        return result + " |= " + operand + ";";
    }
    
    @Override
    public String opNot(String result, String operand) {
        return result + " = ~" + operand + ";";
    }
    
    @Override
    public String opRightShift(String result, String operand, String shiftLen) {
        countOp();
        return result + " = " + operand + " >> " + shiftLen + ";";
    }
    
    @Override
    public String opRightShift(String operand, String shiftLen) {
        countOp();
        return operand + " >>= " + shiftLen + ";";
    }
    
    @Override
    public String opLeftShift(String result, String operand, String shiftLen) {
        countOp();
        return result + " = " + operand + " << " + shiftLen + ";";
    }
    
    @Override
    public String opLeftShift(String operand, String shiftLen) {
        countOp();
        return operand + " <<= " + shiftLen + ";";
    }
    
    @Override
    public String opAdd(String result, String operand, String operand2) {
        countOp();
        return result + " = " + operand + " + " + operand2 + ";";
    }
    
    @Override
    public String opAdc(String result, String operand, String carryBit, String operand2, String carryStore) {
        return null;
    }
    
    @Override
    public String opAdd(String result, String operand) {
        countOp();
        return result + " += " + operand + ";";
    }
    
    @Override
    public String opMultiply(String result, String operand, String operand2) {
        return result + " = " + operand + " * " + operand2 + ";";
    }
    
    @Override
    public String opMultiply(String result, String operand) {
        return result + " *= " + operand + ";";
    }
    
    @Override
    public String opSubtract(String result, String operand, String operand2) {
        return result + " = " + operand + " - " + operand2 + ";";
    }
    
    @Override
    public String opSubtract(String result, String operand) {
        return result + " -= " + operand + ";";
    }
    
    @Override
    public String opSetValue(String result, String value) {
        return result + " = " + value + ";";
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
        builder.append(value);
        builder.append(";");
        return builder.toString();
    }
    
    @Override
    public String opLoad(String result, String operand) {
        return result + " = *" + operand + ";";
    }
    
    @Override
    public String opMax(String result, String operand, String operand2) {
        return result + " = " + operand + " > " + operand2 + " ? " + operand + " : " + operand2 + ";";
    }
    
    @Override
    public String opMin(String result, String operand, String operand2) {
        return result + " = " + operand + " < " + operand2 + " ? " + operand + " : " + operand2 + ";";
    }
    
    @Override
    public String opCmpgt(String result, String operand, String operand2) {
        return result + " = " + operand + " > " + operand2 + " ? " + operand + " : " + operand2 + ";";
    }
    
    @Override
    public String opCmpEpu(String result, String operand, String operand2, String type) {
        return null;
    }
    
    
}
