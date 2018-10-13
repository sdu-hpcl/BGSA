package org.sduhpcl.bgsa.intrinsics;

import lombok.Getter;
import lombok.Setter;
import org.sduhpcl.bgsa.intrinsics.elements.BaseElement;
import org.sduhpcl.bgsa.intrinsics.elements.ElementFactory;
import org.sduhpcl.bgsa.util.Configuration;

import java.util.List;

/**
 * @author zhangjikai
 * @date 16-9-6
 */
@Getter
@Setter
public abstract class BaseIntrinsics {
    
    BaseElement element = ElementFactory.newElement(Configuration.elementType);
    
    /**
     * result = operand & operand2
     *
     * @param result   结果
     * @param operand  操作数1
     * @param operand2 操作数2
     * @return 拼接后的字符串
     */
    public abstract String opAnd(String result, String operand, String operand2);
    
    /**
     * result &= operand
     *
     * @param result  结果
     * @param operand 操作数
     * @return 拼接后的字符串
     */
    public abstract String opAnd(String result, String operand);
    
    /**
     * result = operand ^ operand2
     *
     * @param result   结果
     * @param operand  操作数 1
     * @param operand2 操作数 2
     * @return 拼接后的字符串
     */
    public abstract String opXor(String result, String operand, String operand2);
    
    /**
     * result ^= operand
     *
     * @param result  结果
     * @param operand 操作数
     * @return 拼接后的字符串
     */
    public abstract String opXor(String result, String operand);
    
    /**
     * result = operand | operand2
     *
     * @param result   结果
     * @param operand  操作数1
     * @param operand2 操作数2
     * @return 拼接字符串
     */
    public abstract String opOr(String result, String operand, String operand2);
    
    /**
     * result |= operand
     *
     * @param result  结果
     * @param operand 操作数1
     * @return 拼接字符串
     */
    public abstract String opOr(String result, String operand);
    
    /**
     * result = !operand
     *
     * @param result  结果
     * @param operand 操作数
     * @return 拼接字符串
     */
    public abstract String opNot(String result, String operand);
    
    /**
     * result = operand >> shiftLen
     *
     * @param result   结果
     * @param operand  操作数
     * @param shiftLen 移动位数
     * @return 拼接字符串
     */
    public abstract String opRightShift(String result, String operand, String shiftLen);
    
    /**
     * operand >>= shiftLen
     *
     * @param operand  操作数
     * @param shiftLen 移动位数
     * @return 拼接字符串
     */
    public abstract String opRightShift(String operand, String shiftLen);
    
    
    public abstract String opLeftShift(String result, String operand, String shiftLen);
    
    public abstract String opLeftShift(String operand, String shiftLen);
    
    public abstract String opAdd(String result, String operand, String operand2);
    
    /**
     * only for KNC
     */
    public abstract String opAdc(String result, String operand, String carryBit, String operand2, String carryStore);
    
    public abstract String opAdd(String result, String operand);
    
    public abstract String opMultiply(String result, String operand, String operand2);
    
    public abstract String opMultiply(String result, String operand);
    
    public abstract String opSubtract(String result, String operand, String operand2);
    
    public abstract String opSubtract(String result, String operand);
    
    public abstract String opSetValue(String result, String value);
    
    public abstract String opEqualValue(String result, String value);
    
    public abstract String opSetValue(List<String> results, String value);
    
    public abstract String opLoad(String result, String operand);
    
    public abstract String opMax(String result, String operand, String operand2);
    
    public abstract String opMin(String result, String operand, String operand2);
    
    public abstract String opCmpgt(String result, String operand, String operand2);
    
    public abstract String opCmpEpu(String result, String operand, String operand2, String type);
    
    
}
