package org.sduhpcl.bgsa.generator;

import org.sduhpcl.bgsa.arch.SIMDArch;
import org.sduhpcl.bgsa.util.Configuration;
import org.sduhpcl.bgsa.util.FileUtils;
import org.sduhpcl.bgsa.util.GeneratorUtils;

import java.lang.reflect.Array;
import java.util.*;

/**
 * @author zhangjikai
 * @date 16-9-6
 */
public class BitPAlGenerator extends BaseGenerator {
    
    private List<String> dhTmpValue;
    
    private List<String> dhCalcProcess;
    
    private List<String> dvTmpValue;
    
    private List<String> dvBitCalcProcess;
    
    private List<String> dvBitFinalProcess;
    
    private static final String SPLIT_MARK = "_";
    
    public BitPAlGenerator() {
        fileUtils = FileUtils.getInstance();
        initValues();
    }
    
    public BitPAlGenerator(SIMDArch arch) {
        this();
        setArch(arch);
    }
    
    @Override
    public void genKernel() {
        
        if (Configuration.isEdit) {
            if (isCarry) {
                genEditCarry();
            } else {
                genEditCommon();
            }
            return;
        }
        
        if (Configuration.isPacked) {
            if (isCarry) {
                genPackedCarry();
            } else {
                genPackedCommon();
            }
        } else {
            if (isCarry) {
                genUnpackedCarry();
            } else {
                genUnpackedCommon();
            }
        }
    }
    
    
    private void genPackedScore(boolean isCarry) {
        int i;
        fileUtils.setMarignLeft(2);
        for (i = 0; i < maxBitsNum; i++) {
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " vec" + (int) Math.pow(2, i), String.valueOf((int) Math.pow(2, i))));
        }
        fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " vec_" + Math.abs(minValue), String.valueOf(-minValue)));
        if (Configuration.factor != 1) {
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " factor", Configuration.factor + ""));
        }
        fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " score", minValue + " * ref_len"));
        if (Configuration.isSemiGlobal) {
            fileUtils.writeContent(wordType + " max_score = score;");
        }
        fileUtils.blankLine();
        
        
        fileUtils.writeContent("for(j = 0; j < word_num; j++) {");
        fileUtils.setMarignLeft(3);
        {
            
            if (isCarry) {
                fileUtils.writeContent("for(i = j * " + wordStr + "; i < (j + 1) * " + wordStr + " && i < read_len; i++) {");
            } else {
                fileUtils.writeContent("for(i = j * word_sizem1; i < (j + 1) * word_sizem1 && i < read_len; i++) {");
            }
            
            fileUtils.setMarignLeft(4);
            {
                
                for (i = 0; i < maxBitsNum; i++) {
                    fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dvdh_bit" + (int) Math.pow(2, i) + "[j]", "vec1"));
                    fileUtils.writeContent(baseIntrinsics.opMultiply("tmp_value", "vec" + (int) Math.pow(2, i) + ""));
                    if (i == maxBitsNum - 1) {
                        fileUtils.writeContent(baseIntrinsics.opAdd("score", "tmp_value"));
                    } else {
                        fileUtils.writeContent(baseIntrinsics.opSubtract("score", "tmp_value"));
                    }
                    fileUtils.blankLine();
                }
                
                fileUtils.writeContent(baseIntrinsics.opSubtract("score", "vec_" + Math.abs(minValue)));
                for (i = 0; i < maxBitsNum; i++) {
                    fileUtils.writeContent(baseIntrinsics.opRightShift("dvdh_bit" + (int) Math.pow(2, i) + "[j]", "1"));
                }
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent(baseIntrinsics.opMax("max_score", "score", "max_score"));
                }
            }
            fileUtils.setMarignLeft(3);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
        }
        fileUtils.setMarignLeft(2);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
        
        if (Configuration.factor != 1) {
            if (Configuration.isSemiGlobal) {
                fileUtils.writeContent(baseIntrinsics.opMultiply("max_score", "factor"));
            } else {
                fileUtils.writeContent(baseIntrinsics.opMultiply("score", "factor"));
            }
        }
        
        if (Configuration.isSemiGlobal) {
            fileUtils.writeContent("int * vec_dump = ((int *) & max_score);");
        } else {
            fileUtils.writeContent("int * vec_dump = ((int *) & score);");
        }
        
        
        fileUtils.writeContent("int index = result_index * " + vNumStr + ";");
        
        fileUtils.blankLine();
        fileUtils.writeContent("#pragma vector always");
        fileUtils.writeContent("#pragma ivdep");
        fileUtils.writeContent("for(i = 0; i < " + vNumStr + "; i++){");
        fileUtils.writeContent("results[index + i] = vec_dump[i];", 3);
        fileUtils.writeContent("}");
        fileUtils.writeContent("result_index++;");
    }
    
    
    public void genPackedCommon() {
        
        String remainDHMin = getFieldName(minValue, "remain_dh_", "");
        String compDhMin2Mid = "comp_dh_" + getNumberName(minValue) + "_to_" + getNumberName(midValue);
        String dhMin2Mid = "dh_" + getNumberName(minValue) + "_to_" + getNumberName(midValue);
        String dhMaxOrMatch = getFieldName(maxValue, "dh_", "_or_match");
        String dvNotMax2MidOrMatch = "dvnot" + maxValue + "to" + (midValue + 1) + "ormatch";
        String dvMaxShiftOrMatch = getFieldName(maxValue, "dv", "shiftormatch");
        
        
        //isCarry = false;
        calcDhValues();
        calcDvBit();
        
        int i;
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent(declareStr, 0);
        
        fileUtils.setMarignLeft(1);
        {
            
            fileUtils.writeContent("int word_sizem1 = " + wordStr + " - 1;");
            fileUtils.writeContent("int word_sizem2 = " + wordStr + " - 2;");
            fileUtils.writeContent("int i, j, k, m, n;");
            fileUtils.writeContent(bitMaskStr);
            fileUtils.writeContent(wordType + " matches;");
            fileUtils.writeContent(wordType + " not_matches;", 1);
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones", allOnes));
            fileUtils.writeContent(oneStr, 1);
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " carry_bitmask", carryBitmask));
            
            for (i = maxValue; i > midValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv", "shift;"));
            }
            
            for (i = 0; i < dvTmpValue.size(); i++) {
                fileUtils.writeContent(wordType + " " + dvTmpValue.get(i) + ";");
            }
            
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dh_", ";"));
            }
            
            for (i = maxValue; i > midValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", ";"));
            }
            
            for (i = maxValue; i > minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", "_prevbit;"));
            }
            
            for (i = 0; i < maxLength; i++) {
                fileUtils.writeContent(wordType + " overflow" + i + ";");
            }
            
            for (i = maxValue - 1; i > midValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv_", "shift_not_match;"));
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " dhbit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " dhbit" + (int) Math.pow(2, i) + "inverse;");
            }
            
            for (i = 0; i < dhTmpValue.size(); i++) {
                fileUtils.writeContent(wordType + " " + dhTmpValue.get(i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " dv_bit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 1; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " dh_xor_dv_bit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " * dvdh_bit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " bit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " tmpbit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " sumbit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " carry" + i + ";");
            }
            
            fileUtils.writeContent(wordType + " " + remainDHMin + ";");
            fileUtils.writeContent(wordType + " " + dhMin2Mid + ";");
            fileUtils.writeContent(wordType + " " + compDhMin2Mid + ";");
            fileUtils.writeContent(wordType + " " + dvMaxShiftOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvNotMax2MidOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dhMaxOrMatch + ";");
            fileUtils.writeContent(wordType + " dvdh_bit_high_comp;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " high_one", highOne));
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " next_high_one", nextHighOne));
            fileUtils.writeContent(wordType + " init_val;");
            fileUtils.writeContent(wordType + " sum;");
            fileUtils.writeContent(wordType + " tmp_value;");
            fileUtils.writeContent(wordType + " init_tal;");
            fileUtils.writeContent("char * itr;");
            fileUtils.writeContent(readType + " * matchv;");
            fileUtils.writeContent(readType + " * read_temp = read;");
            fileUtils.blankLine();
            
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * word_num * dvdh_len;");
            for (i = 0; i < maxBitsNum; i++) {
                if (i == 0) {
                    fileUtils.writeContent("dvdh_bit" + (int) Math.pow(2, i) + " = & dvdh_bit_mem[start];");
                } else {
                    fileUtils.writeContent("dvdh_bit" + (int) Math.pow(2, i) + " = & dvdh_bit_mem[start + word_num * " + i + "];");
                }
            }
            
            fileUtils.writeContent("for(k = 0; k < chunk_read_num; k++) {");
            fileUtils.blankLine();
            fileUtils.setMarignLeft(2);
            {
                
                fileUtils.writeContent("read =& read_temp[ k * word_num * " + vNumStr + " * CHAR_NUM];");
                
                fileUtils.writeContent("for (i = 0; i < word_num; i++) {");
                List<String> strList = new ArrayList<>();
                if (Configuration.isSemiGlobal) {
                    writeBitInitStr();
                } else {
                    for (i = 0; i < maxBitsNum; i++) {
                        strList.add("dvdh_bit" + (int) Math.pow(2, i) + "[i]");
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"), 3);
                }
                fileUtils.writeContent("}");
                
                strList = new ArrayList<>();
                for (i = maxValue; i > minValue; i--) {
                    strList.add("dh_" + getNumberName(i));
                }
                fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent("dh_zero = all_ones;");
                } else {
                    fileUtils.writeContent("dh_" + getNumberName(minValue) + " = all_ones;");
                }
                fileUtils.blankLine();
                
                fileUtils.writeContent("for(i = 0, itr = ref; i < ref_len; i++, itr++) {");
                fileUtils.blankLine();
                fileUtils.setMarignLeft(3);
                {
                    strList = new ArrayList<>();
                    for (i = 0; i < maxBitsNum; i++) {
                        strList.add("bit" + (int) Math.pow(2, i));
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    
                    strList = new ArrayList<>();
                    for (i = 0; i < maxLength; i++) {
                        strList.add("overflow" + i);
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    
                    strList = new ArrayList<>();
                    for (i = maxValue; i > minValue; i--) {
                        strList.add(getFieldName(i, "init_", "_prevbit"));
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    
                    fileUtils.writeContent("matchv = & read[((int)*itr) * " + vNumStr + " * word_num];");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(j = 0; j < word_num; j++) {");
                    fileUtils.blankLine();
                    fileUtils.setMarignLeft(4);
                    {
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent("dhbit" + (int) Math.pow(2, i) + " = dvdh_bit" + (int) Math.pow(2, i) + "[j];");
                        }
                        
                        
                        fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                        fileUtils.writeContent("matchv += " + vNumStr + ";");
                        fileUtils.writeContent(baseIntrinsics.opNot("not_matches", "matches"));
                        fileUtils.blankLine();
                        
                        for (i = 0; i < dhCalcProcess.size(); i++) {
                            fileUtils.writeContent(dhCalcProcess.get(i));
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue), "matches"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("sum", "init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue)));
                        fileUtils.writeContent(baseIntrinsics.opAdd("sum", "overflow0"));
                        
                        fileUtils.writeContent(baseIntrinsics.opXor("dv" + getNumberName(maxValue) + "shift", "sum", "dh_" + getNumberName(minValue)));
                        fileUtils.writeContent(baseIntrinsics.opXor("dv" + getNumberName(maxValue) + "shift", "init_" + getNumberName(maxValue)));
                        fileUtils.writeContent(baseIntrinsics.opAnd("dv" + getNumberName(maxValue) + "shift", "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("overflow0", "sum", "word_sizem1"));
                        fileUtils.writeContent(baseIntrinsics.opXor(remainDHMin, "dh_" + getNumberName(minValue), "init_" + getNumberName(maxValue)));
                        fileUtils.writeContent(baseIntrinsics.opOr("dv" + getNumberName(maxValue) + "shiftormatch", "dv" + getNumberName(maxValue) + "shift", "matches"));
                        
                        
                        int count = minValue + 1, x, overflowIndex = 1;
                        for (i = maxValue - 1; i > midValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(i), "dh_" + getNumberName(count) + "", dvMaxShiftOrMatch));
                            for (x = 1; x <= maxValue - 1 - i; x++) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_" + getNumberName(count - x) + "", "dv" + getNumberName(maxValue - x) + "shift"));
                                fileUtils.writeContent(baseIntrinsics.opOr("init_" + getNumberName(i), "tmp_value"));
                            }
                            fileUtils.writeContent(baseIntrinsics.opLeftShift("init_val", "init_" + getNumberName(i), "1"));
                            fileUtils.writeContent(baseIntrinsics.opOr("init_val", "init_" + getNumberName(i) + "_prevbit"));
                            fileUtils.writeContent(baseIntrinsics.opRightShift("init_" + getNumberName(i) + "_prevbit", "init_val", "word_sizem1"));
                            fileUtils.writeContent(baseIntrinsics.opAnd("init_val", "carry_bitmask"));
                            fileUtils.writeContent(baseIntrinsics.opAdd("sum", "init_val", remainDHMin));
                            fileUtils.writeContent(baseIntrinsics.opAdd("sum", "overflow" + overflowIndex));
                            fileUtils.writeContent(baseIntrinsics.opXor("dv" + getNumberName(i) + "shift", "sum", remainDHMin));
                            fileUtils.writeContent(baseIntrinsics.opAnd("dv" + getNumberName(i) + "shift", "not_matches"));
                            fileUtils.writeContent(baseIntrinsics.opRightShift("overflow" + overflowIndex, "sum", "word_sizem1"));
                            fileUtils.blankLine();
                            count++;
                            overflowIndex++;
                        }
                        if (maxValue > (midValue + 1)) {
                            fileUtils.writeContent(baseIntrinsics.opOr(dvNotMax2MidOrMatch, dvMaxShiftOrMatch, "dv" + getNumberName(maxValue - 1) + "shift"));
                            for (i = maxValue - 2; i > midValue; i--) {
                                fileUtils.writeContent(baseIntrinsics.opOr(dvNotMax2MidOrMatch, "dv" + getNumberName(i) + "shift"));
                            }
                            fileUtils.writeContent(baseIntrinsics.opNot(dvNotMax2MidOrMatch, dvNotMax2MidOrMatch));
                        } else {
                            fileUtils.writeContent(baseIntrinsics.opNot(dvNotMax2MidOrMatch, dvMaxShiftOrMatch));
                        }
                        
                        
                        for (String s : dvBitCalcProcess) {
                            fileUtils.writeContent(s);
                        }
                        for (String s : dvBitFinalProcess) {
                            fileUtils.writeContent(s);
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("carry0", "dhbit1", "dv_bit1"));
                        for (i = 1; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("carry" + i, "dhbit" + (int) Math.pow(2, i), "dv_bit" + (int) Math.pow(2, i)));
                            fileUtils.writeContent(baseIntrinsics.opXor("dh_xor_dv_bit" + (int) Math.pow(2, i), "dhbit" + (int) Math.pow(2, i), "dv_bit" + (int) Math.pow(2, i)));
                            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_xor_dv_bit" + (int) Math.pow(2, i), "carry" + (i - 1)));
                            fileUtils.writeContent(baseIntrinsics.opOr("carry" + i, "tmp_value"));
                            fileUtils.blankLine();
                        }
                        
                        fileUtils.writeContent(baseIntrinsics.opXor("sumbit1", "dhbit1", "dv_bit1"));
                        for (i = 1; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opXor("sumbit" + (int) Math.pow(2, i), "dh_xor_dv_bit" + (int) Math.pow(2, i), "carry" + (i - 1)));
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opNot("dvdh_bit_high_comp", "sumbit" + (int) Math.pow(2, maxBitsNum - 1)));
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("sumbit" + (int) Math.pow(2, i), "dvdh_bit_high_comp"));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("tmpbit" + (int) Math.pow(2, i), "sumbit" + (int) Math.pow(2, i),
                                    "next_high_one"));
                            fileUtils.writeContent(baseIntrinsics.opRightShift("tmpbit" + (int) Math.pow(2, i), "word_sizem2"));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opLeftShift("sumbit" + (int) Math.pow(2, i), "1"));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opOr("sumbit" + (int) Math.pow(2, i), "bit" + (int) Math.pow(2, i)));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent("bit" + (int) Math.pow(2, i) + " = " + "tmpbit" + (int) Math.pow(2, i) + ";");
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent("dv_bit" + (int) Math.pow(2, i) + " = " + "sumbit" + (int) Math.pow(2, i) + ";");
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opOr(dhMaxOrMatch, "dh_" + getNumberName(maxValue), "matches"));
                        fileUtils.writeContent(baseIntrinsics.opOr(dhMin2Mid, "dh_" + getNumberName(minValue), "dh_" + getNumberName(minValue + 1)));
                        for (i = minValue + 2; i <= midValue; i++) {
                            fileUtils.writeContent(baseIntrinsics.opOr(dhMin2Mid, "dh_" + getNumberName(i)));
                        }
                        fileUtils.writeContent(baseIntrinsics.opAnd(dhMin2Mid, "not_matches"));
                        //fileUtils.writeContent(intrinsics.opXor(compDhMin2Mid, "all_ones", dhMin2Mid));
                        fileUtils.writeContent(baseIntrinsics.opNot(compDhMin2Mid, dhMin2Mid));
                        fileUtils.blankLine();
                        
                        int mark = midValue - minValue - 1;
                        int flags[] = new int[maxBitsNum];
                        for (i = 0; i < maxBitsNum; i++) {
                            flags[i] = 1 << i;
                        }
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            if ((flags[i] & mark) == 0) {
                                fileUtils.writeContent(baseIntrinsics.opOr("dhbit" + (int) Math.pow(2, i), dhMin2Mid));
                            } else {
                                fileUtils.writeContent(baseIntrinsics.opAnd("dhbit" + (int) Math.pow(2, i), compDhMin2Mid));
                            }
                        }
                        fileUtils.blankLine();
                        
                        mark = maxLength - 1;
                        for (i = 0; i < maxBitsNum; i++) {
                            if ((flags[i] & mark) == 0) {
                                fileUtils.writeContent(baseIntrinsics.opOr("dhbit" + (int) Math.pow(2, i), "matches"));
                            } else {
                                fileUtils.writeContent(baseIntrinsics.opAnd("dhbit" + (int) Math.pow(2, i), "not_matches"));
                            }
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("carry0", "dhbit1", "sumbit1"));
                        for (i = 1; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("carry" + i, "dhbit" + (int) Math.pow(2, i), "sumbit" + (int) Math.pow(2, i)));
                            fileUtils.writeContent(baseIntrinsics.opXor("dh_xor_dv_bit" + (int) Math.pow(2, i), "dhbit" + (int) Math.pow(2, i), "sumbit" + (int) Math.pow(2, i)));
                            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_xor_dv_bit" + (int) Math.pow(2, i), "carry" + (i - 1)));
                            fileUtils.writeContent(baseIntrinsics.opOr("carry" + i, "tmp_value"));
                            fileUtils.blankLine();
                        }
                        
                        fileUtils.writeContent(baseIntrinsics.opXor("sumbit1", "dhbit1", "sumbit1"));
                        for (i = 1; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opXor("sumbit" + (int) Math.pow(2, i), "dh_xor_dv_bit" + (int) Math.pow(2, i), "carry" + (i - 1)));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("sumbit" + (int) Math.pow(2, i), "sumbit" + (int) Math.pow(2, maxBitsNum - 1)));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent("dvdh_bit" + (int) Math.pow(2, i) + "[j] = sumbit" + (int) Math.pow(2, i) + ";");
                        }
                    }
                    fileUtils.setMarignLeft(3);
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                genPackedScore(false);
                
            }
            
            fileUtils.setMarignLeft(1);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
            
            
        }
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
    }
    
    private void genPackedCarry() {
        
        String remainDHMin = getFieldName(minValue, "remain_dh_", "");
        String compDhMin2Mid = "comp_dh_" + getNumberName(minValue) + "_to_" + getNumberName(midValue);
        String dhMin2Mid = "dh_" + getNumberName(minValue) + "_to_" + getNumberName(midValue);
        String dhMaxOrMatch = getFieldName(maxValue, "dh_", "_or_match");
        String dvNotMax2MidOrMatch = "dvnot" + maxValue + "to" + (midValue + 1) + "ormatch";
        String dvMaxShiftOrMatch = getFieldName(maxValue, "dv", "shiftormatch");
        
        List<String> strList = new ArrayList<>();
        isCarry = true;
        calcDhValues();
        calcDvBit();
        
        int i;
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent(declareStr, 0);
        
        fileUtils.setMarignLeft(1);
        {
            
            fileUtils.writeContent("int word_sizem1 = " + wordStr + " - 1;");
            fileUtils.writeContent("int i, j, k;");
            fileUtils.writeContent(bitMaskStr);
            fileUtils.writeContent(wordType + " matches;");
            fileUtils.writeContent(wordType + " not_matches;", 1);
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones", allOnes));
            fileUtils.writeContent(oneStr, 1);
            
            for (i = maxValue; i > midValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv", "shift;"));
            }
            
            for (i = 0; i < dvTmpValue.size(); i++) {
                fileUtils.writeContent(wordType + " " + dvTmpValue.get(i) + ";");
            }
            
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dh_", ";"));
            }
            
            for (i = maxValue; i > midValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", ";"));
            }
            
            for (i = maxValue; i > minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", "_prevbit;"));
            }
            
            for (i = maxValue - 1; i > midValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv_", "shift_not_match;"));
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " dhbit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " dhbit" + (int) Math.pow(2, i) + "inverse;");
            }
            
            for (i = 0; i < dhTmpValue.size(); i++) {
                fileUtils.writeContent(wordType + " " + dhTmpValue.get(i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " dv_bit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 1; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " dh_xor_dv_bit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " * dvdh_bit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " bit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " tmpbit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " sumbit" + (int) Math.pow(2, i) + ";");
            }
            
            for (i = 0; i < maxBitsNum; i++) {
                fileUtils.writeContent(wordType + " carry" + i + ";");
            }
            
            for (i = maxValue; i > midValue; i--) {
                fileUtils.writeContent("__mmask16 carry_bit" + i + ";");
            }
            
            for (i = maxValue; i > midValue; i--) {
                fileUtils.writeContent("__mmask16 carry_bit" + i + "_temp;");
            }
            
            fileUtils.writeContent(wordType + " " + remainDHMin + ";");
            fileUtils.writeContent(wordType + " " + dhMin2Mid + ";");
            fileUtils.writeContent(wordType + " " + compDhMin2Mid + ";");
            fileUtils.writeContent(wordType + " " + dvMaxShiftOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvNotMax2MidOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dhMaxOrMatch + ";");
            fileUtils.writeContent(wordType + " dvdh_bit_high_comp;");
            fileUtils.writeContent(wordType + " init_val;");
            fileUtils.writeContent(wordType + " sum;");
            fileUtils.writeContent(wordType + " tmp_value;");
            fileUtils.writeContent(wordType + " init_tal;");
            fileUtils.writeContent("char * itr;");
            fileUtils.writeContent(readType + " * matchv;");
            fileUtils.writeContent(readType + " * read_temp = read;");
            fileUtils.blankLine();
            
            //generateBaMemAlloc();
            //fileUtils.blankLine();
            
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * word_num * dvdh_len;");
            for (i = 0; i < maxBitsNum; i++) {
                if (i == 0) {
                    fileUtils.writeContent("dvdh_bit" + (int) Math.pow(2, i) + " = & dvdh_bit_mem[start];");
                } else {
                    fileUtils.writeContent("dvdh_bit" + (int) Math.pow(2, i) + " = & dvdh_bit_mem[start + word_num * " + i + "];");
                }
            }
            
            fileUtils.writeContent("for(k = 0; k < chunk_read_num; k++) {");
            fileUtils.blankLine();
            fileUtils.setMarignLeft(2);
            {
                
                fileUtils.writeContent("read =& read_temp[ k * word_num * " + vNumStr + " * CHAR_NUM];");
                
                fileUtils.writeContent("for (i = 0; i < word_num; i++) {");
                if (Configuration.isSemiGlobal) {
                    writeBitInitStr();
                } else {
                    for (i = 0; i < maxBitsNum; i++) {
                        strList.add("dvdh_bit" + (int) Math.pow(2, i) + "[i]");
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"), 3);
                }
                
                fileUtils.writeContent("}");
                
                strList = new ArrayList<>();
                for (i = maxValue; i > minValue; i--) {
                    strList.add("dh_" + getNumberName(i));
                }
                fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent("dh_zero = all_ones;");
                } else {
                    fileUtils.writeContent("dh_" + getNumberName(minValue) + " = all_ones;");
                }
                fileUtils.blankLine();
                
                fileUtils.writeContent("for(i = 0, itr = ref; i < ref_len; i++, itr++) {");
                fileUtils.blankLine();
                fileUtils.setMarignLeft(3);
                {
                    strList = new ArrayList<>();
                    for (i = 0; i < maxBitsNum; i++) {
                        strList.add("bit" + (int) Math.pow(2, i));
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    
                    strList = new ArrayList<>();
                    for (i = maxValue; i > minValue; i--) {
                        strList.add(getFieldName(i, "init_", "_prevbit"));
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    
                    fileUtils.writeContent("matchv = & read[((int)*itr) * " + vNumStr + " * word_num];");
                    fileUtils.writeContent("_mm_prefetch((const char *)&read[(int) * (itr + 1) * " + vNumStr + " * word_num], _MM_HINT_T0);");
                    fileUtils.writeContent("_mm_prefetch((const char *)&read[(int) * (itr + 1) * " + vNumStr + " * word_num], _MM_HINT_T1);");
                    fileUtils.blankLine();
                    
                    for (i = maxValue; i > midValue; i--) {
                        fileUtils.writeContent("carry_bit" + i + " = 0;");
                    }
                    fileUtils.blankLine();
                    
                    
                    fileUtils.writeContent("for(j = 0; j < word_num; j++) {");
                    fileUtils.blankLine();
                    fileUtils.setMarignLeft(4);
                    {
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent("dhbit" + (int) Math.pow(2, i) + " = dvdh_bit" + (int) Math.pow(2, i) + "[j];");
                        }
                        
                        
                        fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                        fileUtils.writeContent("matchv += " + vNumStr + ";");
                        fileUtils.writeContent(baseIntrinsics.opNot("not_matches", "matches"));
                        fileUtils.blankLine();
                        
                        for (i = 0; i < dhCalcProcess.size(); i++) {
                            fileUtils.writeContent(dhCalcProcess.get(i));
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent("#ifdef __MIC__");
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue), "matches"));
                        fileUtils.writeContent(baseIntrinsics.opAdc("sum", "init_" + getNumberName(maxValue), " carry_bit" + maxValue, " dh_" + getNumberName(minValue), " & carry_bit" + maxValue + "_temp"));
                        fileUtils.writeContent("carry_bit" + maxValue + " = " + "carry_bit" + maxValue + "_temp;");
                        //fileUtils.writeContent(intrinsics.opAdd("sum", "init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue)));
                        //fileUtils.writeContent(intrinsics.opAdd("sum", "overflow0"));
                        
                        fileUtils.writeContent(baseIntrinsics.opXor("dv" + getNumberName(maxValue) + "shift", "sum", "dh_" + getNumberName(minValue)));
                        fileUtils.writeContent(baseIntrinsics.opXor("dv" + getNumberName(maxValue) + "shift", "init_" + getNumberName(maxValue)));
                        //fileUtils.writeContent(intrinsics.opAnd("dv" + getNumberName(maxValue) + "shift", "carry_bitmask"));
                        //fileUtils.writeContent(intrinsics.opRightShift("overflow0", "sum", "word_sizem1"));
                        fileUtils.writeContent(baseIntrinsics.opXor(remainDHMin, "dh_" + getNumberName(minValue), "init_" + getNumberName(maxValue)));
                        fileUtils.writeContent(baseIntrinsics.opOr("dv" + getNumberName(maxValue) + "shiftormatch", "dv" + getNumberName(maxValue) + "shift", "matches"));
                        
                        
                        int count = minValue + 1, x, overflowIndex = 1;
                        for (i = maxValue - 1; i > midValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(i), "dh_" + getNumberName(count) + "", dvMaxShiftOrMatch));
                            for (x = 1; x <= maxValue - 1 - i; x++) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_" + getNumberName(count - x) + "", "dv" + getNumberName(maxValue - x) + "shift"));
                                fileUtils.writeContent(baseIntrinsics.opOr("init_" + getNumberName(i), "tmp_value"));
                            }
                            fileUtils.writeContent(baseIntrinsics.opRightShift("tmp_value", "init_pos" + i, "word_sizem1"));
                            fileUtils.writeContent(baseIntrinsics.opLeftShift("init_val", "init_" + getNumberName(i), "1"));
                            fileUtils.writeContent(baseIntrinsics.opOr("init_val", "init_" + getNumberName(i) + "_prevbit"));
                            //fileUtils.writeContent(intrinsics.opRightShift("init_" + getNumberName(i) + "_prevbit", "init_val", "word_sizem1"));
                            fileUtils.writeContent("init_pos" + i + "_prevbit = tmp_value;");
                            //fileUtils.writeContent(intrinsics.opAnd("init_val", "carry_bitmask"));
                            //fileUtils.writeContent(intrinsics.opAdd("sum", "init_val", remainDHMin));
                            //fileUtils.writeContent(intrinsics.opAdd("sum", "overflow" + overflowIndex));
                            fileUtils.writeContent(baseIntrinsics.opAdc("sum", "init_val", "carry_bit" + i, remainDHMin, "& carry_bit" + i + "_temp"));
                            fileUtils.writeContent("carry_bit" + i + " = " + "carry_bit" + i + "_temp;");
                            fileUtils.writeContent(baseIntrinsics.opXor("dv" + getNumberName(i) + "shift", "sum", remainDHMin));
                            fileUtils.writeContent(baseIntrinsics.opAnd("dv" + getNumberName(i) + "shift", "not_matches"));
                            //fileUtils.writeContent(intrinsics.opRightShift("overflow" + overflowIndex, "sum", "word_sizem1"));
                            fileUtils.blankLine();
                            count++;
                            //overflowIndex++;
                        }
                        
                        fileUtils.writeContent("#endif");
                        fileUtils.blankLine();
                        
                        if (maxValue > (midValue + 1)) {
                            fileUtils.writeContent(baseIntrinsics.opOr(dvNotMax2MidOrMatch, dvMaxShiftOrMatch, "dv" + getNumberName(maxValue - 1) + "shift"));
                            for (i = maxValue - 2; i > midValue; i--) {
                                fileUtils.writeContent(baseIntrinsics.opOr(dvNotMax2MidOrMatch, "dv" + getNumberName(i) + "shift"));
                            }
                            fileUtils.writeContent(baseIntrinsics.opNot(dvNotMax2MidOrMatch, dvNotMax2MidOrMatch));
                        } else {
                            fileUtils.writeContent(baseIntrinsics.opNot(dvNotMax2MidOrMatch, dvMaxShiftOrMatch));
                        }
                        
                        
                        for (String s : dvBitCalcProcess) {
                            fileUtils.writeContent(s);
                        }
                        for (String s : dvBitFinalProcess) {
                            fileUtils.writeContent(s);
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("carry0", "dhbit1", "dv_bit1"));
                        for (i = 1; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("carry" + i, "dhbit" + (int) Math.pow(2, i), "dv_bit" + (int) Math.pow(2, i)));
                            fileUtils.writeContent(baseIntrinsics.opXor("dh_xor_dv_bit" + (int) Math.pow(2, i), "dhbit" + (int) Math.pow(2, i), "dv_bit" + (int) Math.pow(2, i)));
                            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_xor_dv_bit" + (int) Math.pow(2, i), "carry" + (i - 1)));
                            fileUtils.writeContent(baseIntrinsics.opOr("carry" + i, "tmp_value"));
                            fileUtils.blankLine();
                        }
                        
                        fileUtils.writeContent(baseIntrinsics.opXor("sumbit1", "dhbit1", "dv_bit1"));
                        for (i = 1; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opXor("sumbit" + (int) Math.pow(2, i), "dh_xor_dv_bit" + (int) Math.pow(2, i), "carry" + (i - 1)));
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opNot("dvdh_bit_high_comp", "sumbit" + (int) Math.pow(2, maxBitsNum - 1)));
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("sumbit" + (int) Math.pow(2, i), "dvdh_bit_high_comp"));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            //fileUtils.writeContent(intrinsics.opAnd("tmpbit" + (int) Math.pow(2, i), "sumbit" + (int) Math.pow(2, i),"next_high_one"));
                            fileUtils.writeContent(baseIntrinsics.opRightShift("tmpbit" + (int) Math.pow(2, i), "sumbit" + (int) Math.pow(2, i), "word_sizem1"));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opLeftShift("sumbit" + (int) Math.pow(2, i), "1"));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opOr("sumbit" + (int) Math.pow(2, i), "bit" + (int) Math.pow(2, i)));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent("bit" + (int) Math.pow(2, i) + " = " + "tmpbit" + (int) Math.pow(2, i) + ";");
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent("dv_bit" + (int) Math.pow(2, i) + " = " + "sumbit" + (int) Math.pow(2, i) + ";");
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opOr(dhMaxOrMatch, "dh_" + getNumberName(maxValue), "matches"));
                        fileUtils.writeContent(baseIntrinsics.opOr(dhMin2Mid, "dh_" + getNumberName(minValue), "dh_" + getNumberName(minValue + 1)));
                        for (i = minValue + 2; i <= midValue; i++) {
                            fileUtils.writeContent(baseIntrinsics.opOr(dhMin2Mid, "dh_" + getNumberName(i)));
                        }
                        fileUtils.writeContent(baseIntrinsics.opAnd(dhMin2Mid, "not_matches"));
                        fileUtils.writeContent(baseIntrinsics.opXor(compDhMin2Mid, "all_ones", dhMin2Mid));
                        fileUtils.blankLine();
                        
                        int mark = midValue - minValue - 1;
                        int flags[] = new int[maxBitsNum];
                        for (i = 0; i < maxBitsNum; i++) {
                            flags[i] = 1 << i;
                        }
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            if ((flags[i] & mark) == 0) {
                                fileUtils.writeContent(baseIntrinsics.opOr("dhbit" + (int) Math.pow(2, i), dhMin2Mid));
                            } else {
                                fileUtils.writeContent(baseIntrinsics.opAnd("dhbit" + (int) Math.pow(2, i), compDhMin2Mid));
                            }
                        }
                        fileUtils.blankLine();
                        
                        mark = maxLength - 1;
                        for (i = 0; i < maxBitsNum; i++) {
                            if ((flags[i] & mark) == 0) {
                                fileUtils.writeContent(baseIntrinsics.opOr("dhbit" + (int) Math.pow(2, i), "matches"));
                            } else {
                                fileUtils.writeContent(baseIntrinsics.opAnd("dhbit" + (int) Math.pow(2, i), "not_matches"));
                            }
                        }
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("carry0", "dhbit1", "sumbit1"));
                        for (i = 1; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("carry" + i, "dhbit" + (int) Math.pow(2, i), "sumbit" + (int) Math.pow(2, i)));
                            fileUtils.writeContent(baseIntrinsics.opXor("dh_xor_dv_bit" + (int) Math.pow(2, i), "dhbit" + (int) Math.pow(2, i), "sumbit" + (int) Math.pow(2, i)));
                            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_xor_dv_bit" + (int) Math.pow(2, i), "carry" + (i - 1)));
                            fileUtils.writeContent(baseIntrinsics.opOr("carry" + i, "tmp_value"));
                            fileUtils.blankLine();
                        }
                        
                        fileUtils.writeContent(baseIntrinsics.opXor("sumbit1", "dhbit1", "sumbit1"));
                        for (i = 1; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opXor("sumbit" + (int) Math.pow(2, i), "dh_xor_dv_bit" + (int) Math.pow(2, i), "carry" + (i - 1)));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("sumbit" + (int) Math.pow(2, i), "sumbit" + (int) Math.pow(2, maxBitsNum - 1)));
                        }
                        fileUtils.blankLine();
                        
                        for (i = 0; i < maxBitsNum; i++) {
                            fileUtils.writeContent("dvdh_bit" + (int) Math.pow(2, i) + "[j] = sumbit" + (int) Math.pow(2, i) + ";");
                        }
                    }
                    fileUtils.setMarignLeft(3);
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                genPackedScore(true);
                //generateBaStoreScoreMulti();
                
            }
            
            fileUtils.setMarignLeft(1);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
            
            
        }
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
    }
    
    
    private void genUnpackedScore(boolean isCarry) {
        
        fileUtils.setMarignLeft(2);
        for (int i = 0; i < maxBitsNum; i++) {
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " vec" + (int) Math.pow(2, i), String.valueOf((int) Math.pow(2, i))));
        }
        
        fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " vec_" + Math.abs(minValue), String.valueOf(-minValue)));
        if (Configuration.factor != 1) {
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " factor", Configuration.factor + ""));
        }
        fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " score", minValue + " * ref_len"));
        if (Configuration.isSemiGlobal) {
            fileUtils.writeContent(wordType + " max_score = score;");
        }
        for (int i = 0; i < maxBitsNum; i++) {
            fileUtils.writeContent(wordType + " add" + (int) Math.pow(2, i) + ";");
        }
        fileUtils.blankLine();
        
        fileUtils.writeContent("for(j = 0; j < word_num; j++) {");
        fileUtils.setMarignLeft(3);
        fileUtils.blankLine();
        {
            for (int i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(getFieldName(i, "dh_", "_temp") + " = " + getFieldName(i, "dh_", "[j]") + ";");
            }
            
            int count = 0;
            int flags[] = new int[maxBitsNum];
            for (int i = 0; i < maxBitsNum; i++) {
                flags[i] = 1 << i;
            }
            int numBinary[][] = new int[maxLength + 1][maxBitsNum];
            for (int i = 0; i <= maxLength; i++) {
                for (int j = 0; j < maxBitsNum; j++) {
                    numBinary[i][j] = ((count & flags[j]) != 0) ? 1 : 0;
                }
                count++;
            }
            
            
            int num = 0;
            String tmp_str = "";
            for (int i = 0; i < maxBitsNum; i++) {
                num = 0;
                for (int j = 0; j <= maxLength; j++) {
                    if (numBinary[j][i] == 1) {
                        if (num == 0) {
                            num = 1;
                            tmp_str = getFieldName(j + minValue, "dh_", "_temp");
                        } else if (num == 1) {
                            num = 2;
                            fileUtils.writeContent(baseIntrinsics.opOr("add" + (int) Math.pow(2, i), tmp_str, getFieldName(j + minValue, "dh_", "_temp")));
                        } else if (num == 2) {
                            fileUtils.writeContent(baseIntrinsics.opOr("add" + (int) Math.pow(2, i), getFieldName(j + minValue, "dh_", "_temp")));
                        }
                    }
                }
                
                if (num == 0) {
                    fileUtils.writeContent(baseIntrinsics.opSetValue("add" + (int) Math.pow(2, i), "0"));
                } else if (num == 1) {
                    fileUtils.writeContent(baseIntrinsics.opEqualValue("add" + (int) Math.pow(2, i), tmp_str));
                    
                }
                
            }
            fileUtils.blankLine();
            
            if (isCarry) {
                fileUtils.writeContent("for(i = j * " + wordStr + "; i < (j + 1) * " + wordStr + " && i < read_len; i++) {");
            } else {
                fileUtils.writeContent("for(i = j * word_sizem1; i < (j + 1) * word_sizem1 && i < read_len; i++) {");
            }
            
            fileUtils.setMarignLeft(4);
            fileUtils.blankLine();
            {
                for (int i = 0; i < maxBitsNum; i++) {
                    
                    
                    fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "add" + (int) Math.pow(2, i), "vec1"));
                    fileUtils.writeContent(baseIntrinsics.opMultiply("tmp_value", "vec" + (int) Math.pow(2, i) + ""));
                    fileUtils.writeContent(baseIntrinsics.opAdd("score", "tmp_value"));
                    fileUtils.blankLine();
                }
                
                fileUtils.writeContent(baseIntrinsics.opSubtract("score", "vec_" + Math.abs(minValue)));
                
                for (int i = 0; i < maxBitsNum; i++) {
                    fileUtils.writeContent(baseIntrinsics.opRightShift("add" + (int) Math.pow(2, i), "1"));
                }
                
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent(baseIntrinsics.opMax("max_score", "score", "max_score"));
                }
                fileUtils.blankLine();
            }
            fileUtils.setMarignLeft(3);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
        }
        fileUtils.setMarignLeft(2);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
        
        if (Configuration.isSemiGlobal) {
            fileUtils.writeContent("score = max_score;");
        }
        if (Configuration.factor != 1) {
            fileUtils.writeContent(baseIntrinsics.opMultiply("score", "factor"));
        }
        fileUtils.writeContent("int * vec_dump = ((int *) & score);");
        fileUtils.writeContent("int index = result_index * " + vNumStr + ";");
        fileUtils.blankLine();
        fileUtils.writeContent("#pragma vector always");
        fileUtils.writeContent("#pragma ivdep");
        fileUtils.writeContent("for(i = 0; i < " + vNumStr + "; i++){");
        fileUtils.writeContent("results[index + i] = vec_dump[i];", 3);
        fileUtils.writeContent("}");
        fileUtils.writeContent("result_index++;");
    }
    
    public void genUnpackedCarry() {
        
        int i, j;
        String remainDHMin = getFieldName(minValue, "remain_dh", "");
        String dhMin2Mid = "dh" + getNumberName(minValue) + "to" + getNumberName(midValue);
        String dhMaxOrMatch = getFieldName(maxValue, "dh", "ormatch");
        String dvNotMax2MidOrMatch = "dvnot" + maxValue + "to" + (midValue) + "ormatch";
        String dvMaxShiftOrMatch = getFieldName(maxValue, "dv", "shiftormatch");
        List<String> strList = new ArrayList<>();
        int index = 1;
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent(declareStr);
        fileUtils.blankLine();
        
        fileUtils.setMarignLeft(1);
        {
            
            fileUtils.writeContent("int word_sizem1 = " + wordStr + " - 1;");
            fileUtils.writeContent("int i, j, k;");
            fileUtils.writeContent(bitMaskStr);
            
            fileUtils.writeContent(wordType + " matches;");
            fileUtils.writeContent(wordType + " not_matches;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones", allOnes));
            fileUtils.writeContent(oneStr, 1);
            //fileUtils.writeContent(intrinsics.opSetValue(wordType + " carry_bitmask", carryBitmask));
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv_", "_shift;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dh_", "_temp;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dh_", "_temp_1;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "* dh_", ";"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", ";"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", "_prevbit;"));
            }
            
            for (i = maxValue - 1; i > midValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv_", "shift_not_match;"));
            }
            
            fileUtils.writeContent(wordType + " " + remainDHMin + ";");
            fileUtils.writeContent(wordType + " " + dhMin2Mid + ";");
            fileUtils.writeContent(wordType + " " + dhMaxOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvMaxShiftOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvNotMax2MidOrMatch + ";");
            fileUtils.writeContent(wordType + " sum;");
            fileUtils.writeContent(wordType + " tmp_value;");
            fileUtils.writeContent(wordType + " init_val;");
            //fileUtils.writeContent(intrinsics.opSetValue(wordType + " high_one", highOne));
            for (i = maxValue; i > midValue; i--) {
                fileUtils.writeContent("__mmask16 carry_bit" + i + ";");
            }
            
            for (i = maxValue; i > midValue; i--) {
                fileUtils.writeContent("__mmask16 carry_bit" + i + "_temp;");
            }
            fileUtils.writeContent("char * itr;");
            fileUtils.writeContent(readType + " * matchv;");
            fileUtils.writeContent(readType + " * read_temp = read;");
            
            fileUtils.blankLine();
            
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * word_num * dvdh_len;");
            
            for (i = maxValue; i >= minValue; i--) {
                if (i == maxValue) {
                    fileUtils.writeContent(getFieldName(i, "dh_", "") + " = & dvdh_bit_mem[start];");
                } else {
                    fileUtils.writeContent(getFieldName(i, "dh_", "") + " = & dvdh_bit_mem[start + word_num * " + index + "];");
                    index++;
                }
            }
            fileUtils.blankLine();
            
            fileUtils.writeContent("for(k = 0; k < chunk_read_num; k++) {");
            fileUtils.setMarignLeft(2);
            fileUtils.blankLine();
            {
                fileUtils.writeContent("read =& read_temp[ k * word_num * " + vNumStr + " * CHAR_NUM];");
                fileUtils.writeContent("for (i = 0; i < word_num; i++) {");
                for (i = maxValue; i >= minValue; i--) {
                    strList.add(getFieldName(i, "dh_", "[i]"));
                }
                fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"), 3);
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent(baseIntrinsics.opSetValue(getFieldName(0, "dh_", "[i]"), allOnes), 3);
                } else {
                    fileUtils.writeContent(baseIntrinsics.opSetValue(getFieldName(minValue, "dh_", "[i]"), allOnes), 3);
                }
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                fileUtils.writeContent("for(i = 0, itr = ref; i < ref_len; i++, itr++) {");
                fileUtils.setMarignLeft(3);
                fileUtils.blankLine();
                {
                    /*strList = new ArrayList<>();
                    for (i = 0; i < maxLength; i++) {
                        strList.add("overflow" + i);
                    }
                    fileUtils.writeContent(intrinsics.opSetValue(strList, "0"));*/
                    
                    strList = new ArrayList<>();
                    for (i = maxValue; i > minValue; i--) {
                        strList.add(getFieldName(i, "init_", "_prevbit"));
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    
                    fileUtils.writeContent("matchv = & read[((int)*itr) * " + vNumStr + " * word_num];");
                    for (i = maxValue; i > midValue; i--) {
                        fileUtils.writeContent("carry_bit" + i + " = 0x0000;");
                    }
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(j = 0; j < word_num; j++) {");
                    fileUtils.setMarignLeft(4);
                    fileUtils.blankLine();
                    {
                        for (i = maxValue; i >= minValue; i--) {
                            fileUtils.writeContent(getFieldName(i, "dh_", "_temp") + " = " + getFieldName(i, "dh_", "[j];"));
                        }
                        fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                        fileUtils.writeContent("matchv += " + vNumStr + ";");
                        fileUtils.writeContent(baseIntrinsics.opNot("not_matches", "matches"));
                        fileUtils.writeContent("#ifdef __MIC__");
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp", "matches"));
                        //fileUtils.writeContent(intrinsics.opAdd("sum", "init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp"));
                        //fileUtils.writeContent(intrinsics.opAdd("sum", "overflow0"));
                        fileUtils.writeContent(baseIntrinsics.opAdc("sum", "init_" + getNumberName(maxValue), " carry_bit" + maxValue, "dh_" + getNumberName(minValue) + "_temp", " & carry_bit" + maxValue + "_temp"));
                        fileUtils.writeContent("carry_bit" + maxValue + " = carry_bit" + maxValue + "_temp;");
                        fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(maxValue) + "_shift", "sum", "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(maxValue) + "_shift", "init_" + getNumberName(maxValue)));
                        //fileUtils.writeContent(intrinsics.opAnd("dv_" + getNumberName(maxValue) + "_shift", "carry_bitmask"));
                        //fileUtils.writeContent(intrinsics.opRightShift("overflow0", "sum", "word_sizem1"));
                        //fileUtils.writeContent(intrinsics.opAnd(remainDHMin, "init_" + getNumberName(maxValue), "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opXor(remainDHMin, "init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opOr("dv" + getNumberName(maxValue) + "shiftormatch", "dv_" + getNumberName(maxValue) + "_shift", "matches"));
                        
                        //fileUtils.blankLine();
                        
                        int count = minValue + 1, x, overflowIndex = 1;
                        //System.out.println(count);
                        for (i = maxValue - 1; i > midValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(i), "dh_" + getNumberName(count) + "_temp", dvMaxShiftOrMatch));
                            
                            for (x = 1; x <= maxValue - 1 - i; x++) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_" + getNumberName(count - x) + "_temp", "dv_" + getNumberName(maxValue - x) + "shift_not_match"));
                                fileUtils.writeContent(baseIntrinsics.opOr("init_" + getNumberName(i), "tmp_value"));
                            }
                            fileUtils.writeContent(baseIntrinsics.opRightShift("tmp_value", "init_" + getNumberName(i), "word_sizem1"));
                            fileUtils.writeContent(baseIntrinsics.opLeftShift("init_val", "init_" + getNumberName(i), "1"));
                            fileUtils.writeContent(baseIntrinsics.opOr("init_val", "init_" + getNumberName(i) + "_prevbit"));
                            fileUtils.writeContent("init_" + getNumberName(i) + "_prevbit = tmp_value;");
                            //fileUtils.writeContent(intrinsics.opRightShift("init_" + getNumberName(i) + "_prevbit", "init_val", "word_sizem1"));
                            //fileUtils.writeContent(intrinsics.opAnd("init_val", "carry_bitmask"));
                            //fileUtils.writeContent(intrinsics.opAdd("sum", "init_val", remainDHMin));
                            //fileUtils.writeContent(intrinsics.opAdd("sum", "overflow" + overflowIndex));
                            fileUtils.writeContent(baseIntrinsics.opAdc("sum", "init_val", " carry_bit" + i, remainDHMin, " & carry_bit" + i + "_temp"));
                            fileUtils.writeContent("carry_bit" + i + " = carry_bit" + i + "_temp;");
                            fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(i) + "_shift", "sum", remainDHMin));
                            fileUtils.writeContent(baseIntrinsics.opAnd("dv_" + getNumberName(i) + "shift_not_match", "dv_" + getNumberName(i) + "_shift", "not_matches"));
                            // fileUtils.writeContent(intrinsics.opRightShift("overflow" + overflowIndex, "sum", "word_sizem1"));
                            fileUtils.blankLine();
                            count++;
                            overflowIndex++;
                        }
                        
                        
                        if (maxValue > (midValue + 1)) {
                            fileUtils.writeContent(baseIntrinsics.opOr(dvNotMax2MidOrMatch, dvMaxShiftOrMatch, "dv_" + getNumberName(maxValue - 1) + "_shift"));
                            for (i = maxValue - 2; i > midValue; i--) {
                                fileUtils.writeContent(baseIntrinsics.opOr(dvNotMax2MidOrMatch, "dv_" + getNumberName(i) + "_shift"));
                            }
                            fileUtils.writeContent(baseIntrinsics.opNot(dvNotMax2MidOrMatch, dvNotMax2MidOrMatch));
                        } else {
                            fileUtils.writeContent(baseIntrinsics.opNot(dvNotMax2MidOrMatch, dvMaxShiftOrMatch));
                        }
                        
                        fileUtils.blankLine();
                        
                        index = minValue + matchScore - mismatchScore;
                        int dh_index = 0;
                        
                        for (i = midValue; i > minValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(i, "init_", ""), getFieldName(index, "dh_", "_temp"), getFieldName(maxValue, "dv", "shiftormatch")));
                            dh_index = index - 1;
                            for (j = maxValue - 1; j > midValue; j--) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(dh_index, "dh_", "_temp"), getFieldName(j, "dv_", "shift_not_match")));
                                fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "init_", ""), "tmp_value"));
                                dh_index--;
                            }
                            
                            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(dh_index, "dh_", "_temp"), dvNotMax2MidOrMatch));
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "init_", ""), "tmp_value"));
                            
                            fileUtils.writeContent(baseIntrinsics.opRightShift("tmp_value", getFieldName(i, "init_", ""), "word_sizem1"));
                            fileUtils.writeContent(baseIntrinsics.opLeftShift(getFieldName(i, "dv_", "_shift"), getFieldName(i, "init_", ""), "1"));
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "dv_", "_shift"), getFieldName(i, "init_", "_prevbit")));
                            fileUtils.writeContent(getFieldName(i, "init_", "_prevbit") + " = tmp_value;");
                            //fileUtils.writeContent(intrinsics.opAnd(getFieldName(i, "init_", "_prevbit"), getFieldName(i, "init_", ""), "carry_bitmask"));
                            //fileUtils.writeContent(intrinsics.opRightShift(getFieldName(i, "init_", "_prevbit"), "(word_sizem1-1)"));
                            index++;
                            fileUtils.blankLine();
                        }
                        fileUtils.writeContent("#endif");
                        fileUtils.blankLine();
                        
                        if (maxValue - 1 > minValue) {
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(minValue, "dv_", "_shift"), getFieldName(maxValue, "dv_", "_shift"), getFieldName(maxValue - 1, "dv_", "_shift")));
                        } else {
                            fileUtils.writeContent(baseIntrinsics.opEqualValue(getFieldName(minValue, "dv_", "_shift"), getFieldName(maxValue, "dv_", "_shift")));
                        }
                        for (i = maxValue - 2; i > minValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(minValue, "dv_", "_shift"), getFieldName(i, "dv_", "_shift")));
                        }
                        
                        //fileUtils.writeContent(intrinsics.opXor(getFieldName(minValue, "dv_", "_shift"), "all_ones"));
                        fileUtils.writeContent(baseIntrinsics.opNot(getFieldName(minValue, "dv_", "_shift"), getFieldName(minValue, "dv_", "_shift")));
                        
                        for (i = midValue + 1; i < maxValue; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(i, "dh_", "_temp"), "not_matches"));
                        }
                        
                        fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(maxValue, "dh", "ormatch"), getFieldName(maxValue, "dh_", "_temp"), "matches"));
                        //fileUtils.writeContent(intrinsics.op);
                        if (maxValue - 1 > midValue) {
                            fileUtils.writeContent(baseIntrinsics.opOr(dhMin2Mid, getFieldName(maxValue, "dh", "ormatch"), getFieldName(maxValue - 1, "dh_", "_temp")));
                            for (i = maxValue - 2; i > midValue; i--) {
                                fileUtils.writeContent(baseIntrinsics.opOr(dhMin2Mid, getFieldName(i, "dh_", "_temp")));
                            }
                            //fileUtils.writeContent(intrinsics.opXor(dhMin2Mid, "all_ones"));
                            fileUtils.writeContent(baseIntrinsics.opNot(dhMin2Mid, dhMin2Mid));
                            
                        } else {
                            //fileUtils.writeContent(intrinsics.opXor(dhMin2Mid, getFieldName(maxValue, "dh", "ormatch"), "all_ones"));
                            fileUtils.writeContent(baseIntrinsics.opNot(dhMin2Mid, getFieldName(maxValue, "dh", "ormatch")));
                            
                        }
                        
                        index = maxValue - 1;
                        
                        
                        for (i = minValue + 1; i <= midValue; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(i, "dh_", "_temp_1"), getFieldName(index, "dv_", "_shift"), getFieldName(maxValue, "dh", "ormatch")));
                            dh_index = maxValue - 1;
                            for (j = 1; j < maxSubtractMid; j++) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(index - j, "dv_", "_shift"), getFieldName(dh_index, "dh_", "_temp")));
                                fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "dh_", "_temp_1"), "tmp_value"));
                                dh_index--;
                            }
                            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(index - j, "dv_", "_shift"), dhMin2Mid));
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "dh_", "_temp"), getFieldName(i, "dh_", "_temp_1"), "tmp_value"));
                            index--;
                            fileUtils.blankLine();
                        }
                        
                        int value = maxSubtractMid;
                        for (i = midValue + 1; i <= maxValue; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(i, "dh_", "_temp_1"), getFieldName(index, "dv_", "_shift"), getFieldName(maxValue, "dh", "ormatch")));
                            dh_index = maxValue - 1;
                            for (j = 1; j < value; j++) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(index - j, "dv_", "_shift"), getFieldName(dh_index, "dh_", "_temp")));
                                fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "dh_", "_temp_1"), "tmp_value"));
                                dh_index--;
                            }
                            fileUtils.writeContent(baseIntrinsics.opEqualValue(getFieldName(i, "dh_", "_temp"), getFieldName(i, "dh_", "_temp_1")));
                            value--;
                            index--;
                            fileUtils.blankLine();
                        }
                        
                        
                        fileUtils.writeContent(baseIntrinsics.opOr("tmp_value", getFieldName(maxValue, "dh_", "_temp"), getFieldName(maxValue - 1, "dh_", "_temp")));
                        for (i = maxValue - 2; i > minValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opOr("tmp_value", getFieldName(i, "dh_", "_temp")));
                        }
                        
                        fileUtils.writeContent(baseIntrinsics.opXor(getFieldName(minValue, "dh_", "_temp"), "tmp_value", "all_ones"));
                        //fileUtils.writeContent(intrinsics.opAnd(getFieldName(minValue, "dh_", "_temp"), "carry_bitmask"));
                        for (i = maxValue; i >= minValue; i--) {
                            fileUtils.writeContent(getFieldName(i, "dh_", "[j]") + " = " + getFieldName(i, "dh_", "_temp") + ";");
                        }
                        
                    }
                    
                    fileUtils.setMarignLeft(3);
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                //genPackedScore();
                //generateUnpackScoreMulti();
                genUnpackedScore(true);
            }
            fileUtils.setMarignLeft(1);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
            
        }
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
        
        
    }
    
    public void genUnpackedCommon() {
        
        int i, j;
        String remainDHMin = getFieldName(minValue, "remain_dh", "");
        String dhMin2Mid = "dh" + getNumberName(minValue) + "to" + getNumberName(midValue);
        String dhMaxOrMatch = getFieldName(maxValue, "dh", "ormatch");
        String dvNotMax2MidOrMatch = "dvnot" + maxValue + "to" + (midValue) + "ormatch";
        String dvMaxShiftOrMatch = getFieldName(maxValue, "dv", "shiftormatch");
        List<String> strList = new ArrayList<>();
        int index = 1;
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent(declareStr);
        fileUtils.blankLine();
        
        fileUtils.setMarignLeft(1);
        {
            
            
            fileUtils.writeContent("int word_sizem1 = " + wordStr + " - 1;");
            fileUtils.writeContent("int i, j, k;");
            fileUtils.writeContent(bitMaskStr);
            
            fileUtils.writeContent(wordType + " matches;");
            fileUtils.writeContent(wordType + " not_matches;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones", allOnes));
            fileUtils.writeContent(oneStr, 1);
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " carry_bitmask", carryBitmask));
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv_", "_shift;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dh_", "_temp;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dh_", "_temp_1;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "* dh_", ";"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", ";"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", "_prevbit;"));
            }
            
            for (i = 0; i < maxLength; i++) {
                fileUtils.writeContent(wordType + " overflow" + i + ";");
            }
            
            for (i = maxValue - 1; i > midValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv_", "shift_not_match;"));
            }
            
            fileUtils.writeContent(wordType + " " + remainDHMin + ";");
            fileUtils.writeContent(wordType + " " + dhMin2Mid + ";");
            fileUtils.writeContent(wordType + " " + dhMaxOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvMaxShiftOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvNotMax2MidOrMatch + ";");
            fileUtils.writeContent(wordType + " sum;");
            fileUtils.writeContent(wordType + " tmp_value;");
            fileUtils.writeContent(wordType + " init_val;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " high_one", highOne));
            fileUtils.writeContent("char * itr;");
            fileUtils.writeContent(readType + " * matchv;");
            fileUtils.writeContent(readType + " * read_temp = read;");
            
            fileUtils.blankLine();
            
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * word_num * dvdh_len;");
            
            for (i = maxValue; i >= minValue; i--) {
                if (i == maxValue) {
                    fileUtils.writeContent(getFieldName(i, "dh_", "") + " = & dvdh_bit_mem[start];");
                } else {
                    fileUtils.writeContent(getFieldName(i, "dh_", "") + " = & dvdh_bit_mem[start + word_num * " + index + "];");
                    index++;
                }
            }
            fileUtils.blankLine();
            
            fileUtils.writeContent("for(k = 0; k < chunk_read_num; k++) {");
            fileUtils.setMarignLeft(2);
            fileUtils.blankLine();
            {
                fileUtils.writeContent("read =& read_temp[ k * word_num * " + vNumStr + " * CHAR_NUM];");
                fileUtils.writeContent("for (i = 0; i < word_num; i++) {");
                for (i = maxValue; i >= minValue; i--) {
                    strList.add(getFieldName(i, "dh_", "[i]"));
                }
                fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"), 3);
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent(baseIntrinsics.opSetValue(getFieldName(0, "dh_", "[i]"), carryBitmask), 3);
                } else {
                    fileUtils.writeContent(baseIntrinsics.opSetValue(getFieldName(minValue, "dh_", "[i]"), carryBitmask), 3);
                }
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                fileUtils.writeContent("for(i = 0, itr = ref; i < ref_len; i++, itr++) {");
                fileUtils.setMarignLeft(3);
                fileUtils.blankLine();
                {
                    strList = new ArrayList<>();
                    for (i = 0; i < maxLength; i++) {
                        strList.add("overflow" + i);
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    
                    strList = new ArrayList<>();
                    for (i = maxValue; i > minValue; i--) {
                        strList.add(getFieldName(i, "init_", "_prevbit"));
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    
                    fileUtils.writeContent("matchv = & read[((int)*itr) * " + vNumStr + " * word_num];");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(j = 0; j < word_num; j++) {");
                    fileUtils.setMarignLeft(4);
                    fileUtils.blankLine();
                    {
                        for (i = maxValue; i >= minValue; i--) {
                            fileUtils.writeContent(getFieldName(i, "dh_", "_temp") + " = " + getFieldName(i, "dh_", "[j];"));
                        }
                        fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                        fileUtils.writeContent("matchv += " + vNumStr + ";");
                        fileUtils.writeContent(baseIntrinsics.opNot("not_matches", "matches"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp", "matches"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("sum", "init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("sum", "overflow0"));
                        fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(maxValue) + "_shift", "sum", "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(maxValue) + "_shift", "init_" + getNumberName(maxValue)));
                        fileUtils.writeContent(baseIntrinsics.opAnd("dv_" + getNumberName(maxValue) + "_shift", "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("overflow0", "sum", "word_sizem1"));
                        fileUtils.writeContent(baseIntrinsics.opAnd(remainDHMin, "init_" + getNumberName(maxValue), "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opXor(remainDHMin, "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opOr("dv" + getNumberName(maxValue) + "shiftormatch", "dv_" + getNumberName(maxValue) + "_shift", "matches"));
                        
                        //fileUtils.blankLine();
                        
                        int count = minValue + 1, x, overflowIndex = 1;
                        //System.out.println(count);
                        for (i = maxValue - 1; i > midValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(i), "dh_" + getNumberName(count) + "_temp", dvMaxShiftOrMatch));
                            for (x = 1; x <= maxValue - 1 - i; x++) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_" + getNumberName(count - x) + "_temp", "dv_" + getNumberName(maxValue - x) + "shift_not_match"));
                                fileUtils.writeContent(baseIntrinsics.opOr("init_" + getNumberName(i), "tmp_value"));
                            }
                            fileUtils.writeContent(baseIntrinsics.opLeftShift("init_val", "init_" + getNumberName(i), "1"));
                            fileUtils.writeContent(baseIntrinsics.opOr("init_val", "init_" + getNumberName(i) + "_prevbit"));
                            fileUtils.writeContent(baseIntrinsics.opRightShift("init_" + getNumberName(i) + "_prevbit", "init_val", "word_sizem1"));
                            fileUtils.writeContent(baseIntrinsics.opAnd("init_val", "carry_bitmask"));
                            fileUtils.writeContent(baseIntrinsics.opAdd("sum", "init_val", remainDHMin));
                            fileUtils.writeContent(baseIntrinsics.opAdd("sum", "overflow" + overflowIndex));
                            fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(i) + "_shift", "sum", remainDHMin));
                            fileUtils.writeContent(baseIntrinsics.opAnd("dv_" + getNumberName(i) + "shift_not_match", "dv_" + getNumberName(i) + "_shift", "not_matches"));
                            fileUtils.writeContent(baseIntrinsics.opRightShift("overflow" + overflowIndex, "sum", "word_sizem1"));
                            fileUtils.blankLine();
                            count++;
                            overflowIndex++;
                        }
                        
                        
                        if (maxValue > (midValue + 1)) {
                            fileUtils.writeContent(baseIntrinsics.opOr(dvNotMax2MidOrMatch, dvMaxShiftOrMatch, "dv_" + getNumberName(maxValue - 1) + "_shift"));
                            for (i = maxValue - 2; i > midValue; i--) {
                                fileUtils.writeContent(baseIntrinsics.opOr(dvNotMax2MidOrMatch, "dv_" + getNumberName(i) + "_shift"));
                            }
                            fileUtils.writeContent(baseIntrinsics.opNot(dvNotMax2MidOrMatch, dvNotMax2MidOrMatch));
                        } else {
                            fileUtils.writeContent(baseIntrinsics.opNot(dvNotMax2MidOrMatch, dvMaxShiftOrMatch));
                        }
                        
                        fileUtils.blankLine();
                        
                        index = minValue + matchScore - mismatchScore;
                        int dh_index = 0;
                        
                        for (i = midValue; i > minValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(i, "init_", ""), getFieldName(index, "dh_", "_temp"), getFieldName(maxValue, "dv", "shiftormatch")));
                            dh_index = index - 1;
                            for (j = maxValue - 1; j > midValue; j--) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(dh_index, "dh_", "_temp"), getFieldName(j, "dv_", "shift_not_match")));
                                fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "init_", ""), "tmp_value"));
                                dh_index--;
                            }
                            
                            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(dh_index, "dh_", "_temp"), dvNotMax2MidOrMatch));
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "init_", ""), "tmp_value"));
                            
                            fileUtils.writeContent(baseIntrinsics.opLeftShift(getFieldName(i, "dv_", "_shift"), getFieldName(i, "init_", ""), "1"));
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "dv_", "_shift"), getFieldName(i, "init_", "_prevbit")));
                            fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(i, "init_", "_prevbit"), getFieldName(i, "init_", ""), "carry_bitmask"));
                            fileUtils.writeContent(baseIntrinsics.opRightShift(getFieldName(i, "init_", "_prevbit"), "(word_sizem1-1)"));
                            index++;
                            fileUtils.blankLine();
                        }
                        
                        if (maxValue - 1 > minValue) {
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(minValue, "dv_", "_shift"), getFieldName(maxValue, "dv_", "_shift"), getFieldName(maxValue - 1, "dv_", "_shift")));
                        } else {
                            fileUtils.writeContent(baseIntrinsics.opEqualValue(getFieldName(minValue, "dv_", "_shift"), getFieldName(maxValue, "dv_", "_shift")));
                        }
                        for (i = maxValue - 2; i > minValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(minValue, "dv_", "_shift"), getFieldName(i, "dv_", "_shift")));
                        }
                        
                        //fileUtils.writeContent(intrinsics.opXor(getFieldName(minValue, "dv_", "_shift"), "all_ones"));
                        fileUtils.writeContent(baseIntrinsics.opNot(getFieldName(minValue, "dv_", "_shift"), getFieldName(minValue, "dv_", "_shift")));
                        
                        for (i = midValue + 1; i < maxValue; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(i, "dh_", "_temp"), "not_matches"));
                        }
                        
                        fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(maxValue, "dh", "ormatch"), getFieldName(maxValue, "dh_", "_temp"), "matches"));
                        //fileUtils.writeContent(intrinsics.op);
                        if (maxValue - 1 > midValue) {
                            fileUtils.writeContent(baseIntrinsics.opOr(dhMin2Mid, getFieldName(maxValue, "dh", "ormatch"), getFieldName(maxValue - 1, "dh_", "_temp")));
                            for (i = maxValue - 2; i > midValue; i--) {
                                fileUtils.writeContent(baseIntrinsics.opOr(dhMin2Mid, getFieldName(i, "dh_", "_temp")));
                            }
                            //fileUtils.writeContent(intrinsics.opXor(dhMin2Mid, "all_ones"));
                            fileUtils.writeContent(baseIntrinsics.opNot(dhMin2Mid, dhMin2Mid));
                        } else {
                            //fileUtils.writeContent(intrinsics.opXor(dhMin2Mid, getFieldName(maxValue, "dh", "ormatch"), "all_ones"));
                            fileUtils.writeContent(baseIntrinsics.opNot(dhMin2Mid, getFieldName(maxValue, "dh", "ormatch")));
                        }
                        
                        index = maxValue - 1;
                        
                        
                        for (i = minValue + 1; i <= midValue; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(i, "dh_", "_temp_1"), getFieldName(index, "dv_", "_shift"), getFieldName(maxValue, "dh", "ormatch")));
                            dh_index = maxValue - 1;
                            for (j = 1; j < maxSubtractMid; j++) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(index - j, "dv_", "_shift"), getFieldName(dh_index, "dh_", "_temp")));
                                fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "dh_", "_temp_1"), "tmp_value"));
                                dh_index--;
                            }
                            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(index - j, "dv_", "_shift"), dhMin2Mid));
                            fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "dh_", "_temp"), getFieldName(i, "dh_", "_temp_1"), "tmp_value"));
                            index--;
                            fileUtils.blankLine();
                        }
                        
                        int value = maxSubtractMid;
                        for (i = midValue + 1; i <= maxValue; i++) {
                            fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(i, "dh_", "_temp_1"), getFieldName(index, "dv_", "_shift"), getFieldName(maxValue, "dh", "ormatch")));
                            dh_index = maxValue - 1;
                            for (j = 1; j < value; j++) {
                                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", getFieldName(index - j, "dv_", "_shift"), getFieldName(dh_index, "dh_", "_temp")));
                                fileUtils.writeContent(baseIntrinsics.opOr(getFieldName(i, "dh_", "_temp_1"), "tmp_value"));
                                dh_index--;
                            }
                            fileUtils.writeContent(baseIntrinsics.opEqualValue(getFieldName(i, "dh_", "_temp"), getFieldName(i, "dh_", "_temp_1")));
                            value--;
                            index--;
                            fileUtils.blankLine();
                        }
                        
                        
                        fileUtils.writeContent(baseIntrinsics.opOr("tmp_value", getFieldName(maxValue, "dh_", "_temp"), getFieldName(maxValue - 1, "dh_", "_temp")));
                        for (i = maxValue - 2; i > minValue; i--) {
                            fileUtils.writeContent(baseIntrinsics.opOr("tmp_value", getFieldName(i, "dh_", "_temp")));
                        }
                        
                        //fileUtils.writeContent(intrinsics.opXor(getFieldName(minValue, "dh_", "_temp"), "tmp_value", "all_ones"));
                        fileUtils.writeContent(baseIntrinsics.opNot(getFieldName(minValue, "dh_", "_temp"), "tmp_value"));
                        fileUtils.writeContent(baseIntrinsics.opAnd(getFieldName(minValue, "dh_", "_temp"), "carry_bitmask"));
                        for (i = maxValue; i >= minValue; i--) {
                            fileUtils.writeContent(getFieldName(i, "dh_", "[j]") + " = " + getFieldName(i, "dh_", "_temp") + ";");
                        }
                        
                    }
                    
                    fileUtils.setMarignLeft(3);
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                
                genUnpackedScore(false);
                //generateUnpackScoreMulti();
            }
            fileUtils.setMarignLeft(1);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
            
        }
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
        
        
    }
    
    
    private void genEditScore(boolean isCarry) {
        fileUtils.setMarignLeft(2);
        if (Configuration.factor != 1) {
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " factor", Configuration.factor + ""));
        }
        
        fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " score", minValue + " * ref_len"));
        if (Configuration.isSemiGlobal) {
            fileUtils.writeContent(wordType + " max_score = score;");
        }
        for (int i = 0; i < maxBitsNum; i++) {
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " vec" + (int) Math.pow(2, i), String.valueOf((int) Math.pow(2, i))));
        }
        fileUtils.blankLine();
        fileUtils.writeContent("for(j = 0; j < word_num; j++) {");
        fileUtils.setMarignLeft(3);
        {
            if (isCarry) {
                fileUtils.writeContent("for(i = j * " + wordStr + "; i < (j + 1) * " + wordStr + " && i < read_len; i++) {");
            } else {
                fileUtils.writeContent("for(i = j * word_sizem1; i < (j + 1) * word_sizem1 && i < read_len; i++) {");
            }
            
            fileUtils.setMarignLeft(4);
            {
                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_zero[j]", "vec1"));
                fileUtils.writeContent(baseIntrinsics.opAdd("score", "tmp_value"));
                fileUtils.blankLine();
                
                fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_pos1[j]", "vec1"));
                fileUtils.writeContent(baseIntrinsics.opMultiply("tmp_value", "vec2"));
                fileUtils.writeContent(baseIntrinsics.opAdd("score", "tmp_value"));
                fileUtils.writeContent(baseIntrinsics.opSubtract("score", "vec" + Math.abs(minValue)));
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent(baseIntrinsics.opMax("max_score", "score", "max_score"));
                }
                fileUtils.blankLine();
                
                fileUtils.writeContent(baseIntrinsics.opRightShift("dh_zero[j]", "1"));
                fileUtils.writeContent(baseIntrinsics.opRightShift("dh_pos1[j]", "1"));
            }
            fileUtils.setMarignLeft(3);
            fileUtils.writeContent("}");
            
        }
        fileUtils.setMarignLeft(2);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
        
        if (Configuration.isSemiGlobal) {
            fileUtils.writeContent("score = max_score;");
        }
        if (Configuration.factor != 1) {
            fileUtils.writeContent(baseIntrinsics.opMultiply("score", "factor"));
        }
        fileUtils.writeContent("int * vec_dump = ((int *) & score);");
        fileUtils.writeContent("int index = result_index * " + vNumStr + ";");
        fileUtils.blankLine();
        
        fileUtils.writeContent("#pragma vector always");
        fileUtils.writeContent("#pragma ivdep");
        fileUtils.writeContent("for(i = 0; i < " + vNumStr + "; i++){");
        fileUtils.writeContent("results[index + i] = vec_dump[i];", 3);
        fileUtils.writeContent("}");
        fileUtils.writeContent("result_index++;");
    }
    
    
    public void genEditCarry() {
        int i, j;
        String remainDHMin = getFieldName(minValue, "remain_dh", "");
        String dhMin2Mid = "dh" + getNumberName(minValue) + "to" + getNumberName(midValue);
        String dhMaxOrMatch = getFieldName(maxValue, "dh", "ormatch");
        String dvNotMax2MidOrMatch = "dvnot" + maxValue + "to" + (midValue) + "ormatch";
        String dvMaxShiftOrMatch = getFieldName(maxValue, "dv", "shiftormatch");
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent(declareStr);
        fileUtils.setMarignLeft(1);
        {
            fileUtils.writeContent("int word_sizem1 = " + wordStr + " - 1;");
            //fileUtils.writeContent("int word_sizem2 = " + wordStr + " - 2;");
            fileUtils.writeContent("int i, j, k;");
            fileUtils.writeContent(bitMaskStr);
            
            fileUtils.writeContent(wordType + " matches;");
            fileUtils.writeContent(wordType + " not_matches;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones", allOnes));
            fileUtils.writeContent(oneStr);
            //fileUtils.writeContent(intrinsics.opSetValue(wordType + " carry_bitmask", carryBitmask));
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv_", "_shift;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dh_", "_temp;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "* dh_", ";"));
            }
            
            for (i = maxValue; i > minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", ";"));
            }
            
            for (i = maxValue; i > minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", "_prevbit;"));
            }
            
            fileUtils.writeContent(wordType + " " + remainDHMin + ";");
            fileUtils.writeContent(wordType + " " + dhMin2Mid + ";");
            fileUtils.writeContent(wordType + " " + dhMaxOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvMaxShiftOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvNotMax2MidOrMatch + ";");
            fileUtils.writeContent(wordType + " sum;");
            fileUtils.writeContent(wordType + " tmp_value;");
            fileUtils.writeContent(wordType + " init_val;");
            fileUtils.writeContent("__mmask16 carry = 0x0000;");
            fileUtils.writeContent("__mmask16 carry_temp = 0x0000;");
            fileUtils.writeContent("char * itr;");
            fileUtils.writeContent(readType + " * matchv;");
            fileUtils.writeContent(readType + " * read_temp = read;");
            fileUtils.blankLine();
            
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * word_num * dvdh_len;");
            int index = 1;
            for (i = maxValue; i >= minValue; i--) {
                if (i == maxValue) {
                    fileUtils.writeContent(getFieldName(i, "dh_", "") + " = & dvdh_bit_mem[start];");
                } else {
                    fileUtils.writeContent(getFieldName(i, "dh_", "") + " = & dvdh_bit_mem[start + word_num * " + index + "];");
                    index++;
                }
            }
            fileUtils.blankLine();
            
            fileUtils.writeContent("for(k = 0; k < chunk_read_num; k++) {");
            fileUtils.setMarignLeft(2);
            fileUtils.blankLine();
            {
                fileUtils.writeContent("read =& read_temp[ k * word_num * " + vNumStr + " * CHAR_NUM];");
                List<String> strList = new ArrayList<>();
                fileUtils.writeContent("for (i = 0; i < word_num; i++) {");
                for (i = maxValue; i > minValue; i--) {
                    strList.add(getFieldName(i, "dh_", "[i]"));
                }
                fileUtils.setMarignLeft(3);
                {
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    if (Configuration.isSemiGlobal) {
                        fileUtils.writeContent(baseIntrinsics.opSetValue(getFieldName(0, "dh_", "[i]"), allOnes));
                    } else {
                        fileUtils.writeContent(baseIntrinsics.opSetValue(getFieldName(minValue, "dh_", "[i]"), allOnes));
                    }
                    
                    
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                fileUtils.writeContent("for(i = 0, itr = ref; i < ref_len; i++, itr++) {");
                fileUtils.setMarignLeft(3);
                fileUtils.blankLine();
                {
                    /*strList = new ArrayList<>();
                    for (i = 0; i < maxLength; i++) {
                        strList.add("overflow" + i);
                    }
                    fileUtils.writeContent(intrinsics.opSetValue(strList, "0"));*/
                    strList = new ArrayList<>();
                    for (i = maxValue; i > minValue; i--) {
                        strList.add(getFieldName(i, "init_", "_prevbit"));
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    fileUtils.writeContent("matchv = & read[((int)*itr) * " + vNumStr + " * word_num];");
                    fileUtils.writeContent("carry = 0x0000;");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(j = 0; j < word_num; j++) {");
                    
                    fileUtils.setMarignLeft(4);
                    {
                        for (i = maxValue; i >= minValue; i--) {
                            fileUtils.writeContent(getFieldName(i, "dh_", "_temp") + " = " + getFieldName(i, "dh_", "[j];"));
                        }
                        
                        fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                        fileUtils.writeContent("matchv += " + vNumStr + ";");
                        fileUtils.writeContent(baseIntrinsics.opNot("not_matches", "matches"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp", "matches"));
                        //fileUtils.writeContent(intrinsics.opAdd("sum", "init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp"));
                        //fileUtils.writeContent(intrinsics.opAdd("sum", "overflow0"));
                        fileUtils.writeContent("#ifdef __MIC__");
                        fileUtils.writeContent(baseIntrinsics.opAdc("sum", "init_" + getNumberName(maxValue), "carry", "dh_" + getNumberName(minValue) + "_temp", "&carry_temp"));
                        fileUtils.writeContent("carry = carry_temp;");
                        fileUtils.writeContent("#endif");
                        fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(maxValue) + "_shift", "sum", "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(maxValue) + "_shift", "init_" + getNumberName(maxValue)));
                        //fileUtils.writeContent(intrinsics.opAnd("dv_" + getNumberName(maxValue) + "_shift", "carry_bitmask"));
                        //fileUtils.writeContent(intrinsics.opRightShift("overflow0", "sum", "word_sizem1"));
                        //fileUtils.writeContent(intrinsics.opAnd(remainDHMin, "init_" + getNumberName(maxValue), "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opXor(remainDHMin, "init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opOr(dvMaxShiftOrMatch, "dv_" + getNumberName(maxValue) + "_shift", "matches"));
                        fileUtils.writeContent(baseIntrinsics.opXor(dvNotMax2MidOrMatch, "dv" + getNumberName(maxValue) + "shiftormatch", "all_ones"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("init_zero", "dh_zero_temp", dvMaxShiftOrMatch));
                        fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_neg1_temp", dvNotMax2MidOrMatch));
                        fileUtils.writeContent(baseIntrinsics.opOr("init_zero", "tmp_value"));
                        
                        fileUtils.writeContent(baseIntrinsics.opLeftShift("dv_zero_shift", "init_zero", "1"));
                        fileUtils.writeContent(baseIntrinsics.opOr("dv_zero_shift", "init_zero_prevbit"));
                        //fileUtils.writeContent(intrinsics.opAnd("init_zero_prevbit", "init_zero", "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("init_zero_prevbit", "init_zero", "word_sizem1"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opOr("dv_neg1_shift", "dv_pos1_shift", "dv_zero_shift"));
                        fileUtils.writeContent(baseIntrinsics.opXor("dv_neg1_shift", "all_ones"));
                        fileUtils.writeContent(baseIntrinsics.opOr(dhMaxOrMatch, "dh_pos1_temp", "matches"));
                        fileUtils.writeContent(baseIntrinsics.opXor(dhMin2Mid, dhMaxOrMatch, "all_ones"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("dh_zero_temp", "dv_zero_shift", dhMaxOrMatch));
                        fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dv_neg1_shift", dhMin2Mid));
                        fileUtils.writeContent(baseIntrinsics.opOr("dh_zero_temp", "tmp_value"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("dh_pos1_temp", "dv_neg1_shift", dhMaxOrMatch));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opOr("dh_neg1_temp", "dh_zero_temp", "dh_pos1_temp"));
                        fileUtils.writeContent(baseIntrinsics.opXor("dh_neg1_temp", "all_ones"));
                        //fileUtils.writeContent(intrinsics.opAnd("dh_neg1_temp", "carry_bitmask"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent("dh_pos1[j] = dh_pos1_temp;");
                        fileUtils.writeContent("dh_zero[j] = dh_zero_temp;");
                        fileUtils.writeContent("dh_neg1[j] = dh_neg1_temp;");
                        
                    }
                    fileUtils.setMarignLeft(3);
                    fileUtils.writeContent("}");
                    
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                //generateEditScoreMulti();
                genEditScore(true);
            }
            fileUtils.setMarignLeft(1);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
            
            
        }
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
        
    }
    
    
    public void genEditCommon() {
        int i, j;
        String remainDHMin = getFieldName(minValue, "remain_dh", "");
        String dhMin2Mid = "dh" + getNumberName(minValue) + "to" + getNumberName(midValue);
        String dhMaxOrMatch = getFieldName(maxValue, "dh", "ormatch");
        String dvNotMax2MidOrMatch = "dvnot" + maxValue + "to" + (midValue) + "ormatch";
        String dvMaxShiftOrMatch = getFieldName(maxValue, "dv", "shiftormatch");
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent(declareStr);
        fileUtils.setMarignLeft(1);
        {
            fileUtils.writeContent("int word_sizem1 = " + wordStr + " - 1;");
            fileUtils.writeContent("int word_sizem2 = " + wordStr + " - 2;");
            fileUtils.writeContent("int i, j, k;");
            fileUtils.writeContent(bitMaskStr);
            
            fileUtils.writeContent(wordType + " matches;");
            fileUtils.writeContent(wordType + " not_matches;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones", allOnes));
            fileUtils.writeContent(oneStr);
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " carry_bitmask", carryBitmask));
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dv_", "_shift;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "dh_", "_temp;"));
            }
            
            for (i = maxValue; i >= minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "* dh_", ";"));
            }
            
            for (i = maxValue; i > minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", ";"));
            }
            
            for (i = maxValue; i > minValue; i--) {
                fileUtils.writeContent(wordType + " " + getFieldName(i, "init_", "_prevbit;"));
            }
            
            for (i = 0; i < maxLength; i++) {
                fileUtils.writeContent(wordType + " overflow" + i + ";");
            }
            
            fileUtils.writeContent(wordType + " " + remainDHMin + ";");
            fileUtils.writeContent(wordType + " " + dhMin2Mid + ";");
            fileUtils.writeContent(wordType + " " + dhMaxOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvMaxShiftOrMatch + ";");
            fileUtils.writeContent(wordType + " " + dvNotMax2MidOrMatch + ";");
            fileUtils.writeContent(wordType + " sum;");
            fileUtils.writeContent(wordType + " tmp_value;");
            fileUtils.writeContent(wordType + " init_val;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " high_one", highOne));
            fileUtils.writeContent("char * itr;");
            fileUtils.writeContent(readType + " * matchv;");
            fileUtils.writeContent(readType + " * read_temp = read;");
            fileUtils.blankLine();
            
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * word_num * dvdh_len;");
            int index = 1;
            for (i = maxValue; i >= minValue; i--) {
                if (i == maxValue) {
                    fileUtils.writeContent(getFieldName(i, "dh_", "") + " = & dvdh_bit_mem[start];");
                } else {
                    fileUtils.writeContent(getFieldName(i, "dh_", "") + " = & dvdh_bit_mem[start + word_num * " + index + "];");
                    index++;
                }
            }
            fileUtils.blankLine();
            
            fileUtils.writeContent("for(k = 0; k < chunk_read_num; k++) {");
            fileUtils.setMarignLeft(2);
            fileUtils.blankLine();
            {
                fileUtils.writeContent("read =& read_temp[ k * word_num * " + vNumStr + " * CHAR_NUM];");
                List<String> strList = new ArrayList<>();
                fileUtils.writeContent("for (i = 0; i < word_num; i++) {");
                for (i = maxValue; i >= minValue; i--) {
                    strList.add(getFieldName(i, "dh_", "[i]"));
                }
                fileUtils.setMarignLeft(3);
                {
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    if (Configuration.isSemiGlobal) {
                        fileUtils.writeContent(baseIntrinsics.opSetValue(getFieldName(0, "dh_", "[i]"), carryBitmask));
                    } else {
                        fileUtils.writeContent(baseIntrinsics.opSetValue(getFieldName(minValue, "dh_", "[i]"), carryBitmask));
                    }
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                fileUtils.writeContent("for(i = 0, itr = ref; i < ref_len; i++, itr++) {");
                fileUtils.setMarignLeft(3);
                fileUtils.blankLine();
                {
                    strList = new ArrayList<>();
                    for (i = 0; i < maxLength; i++) {
                        strList.add("overflow" + i);
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    strList = new ArrayList<>();
                    for (i = maxValue; i > minValue; i--) {
                        strList.add(getFieldName(i, "init_", "_prevbit"));
                    }
                    fileUtils.writeContent(baseIntrinsics.opSetValue(strList, "0"));
                    fileUtils.writeContent("matchv = & read[((int)*itr) * " + vNumStr + " * word_num];");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(j = 0; j < word_num; j++) {");
                    
                    fileUtils.setMarignLeft(4);
                    {
                        for (i = maxValue; i >= minValue; i--) {
                            fileUtils.writeContent(getFieldName(i, "dh_", "_temp") + " = " + getFieldName(i, "dh_", "[j];"));
                        }
                        
                        fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                        fileUtils.writeContent("matchv += " + vNumStr + ";");
                        fileUtils.writeContent(baseIntrinsics.opNot("not_matches", "matches"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp", "matches"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("sum", "init_" + getNumberName(maxValue), "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("sum", "overflow0"));
                        fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(maxValue) + "_shift", "sum", "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opXor("dv_" + getNumberName(maxValue) + "_shift", "init_" + getNumberName(maxValue)));
                        fileUtils.writeContent(baseIntrinsics.opAnd("dv_" + getNumberName(maxValue) + "_shift", "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("overflow0", "sum", "word_sizem1"));
                        fileUtils.writeContent(baseIntrinsics.opAnd(remainDHMin, "init_" + getNumberName(maxValue), "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opXor(remainDHMin, "dh_" + getNumberName(minValue) + "_temp"));
                        fileUtils.writeContent(baseIntrinsics.opOr(dvMaxShiftOrMatch, "dv_" + getNumberName(maxValue) + "_shift", "matches"));
                        //fileUtils.writeContent(intrinsics.opXor(dvNotMax2MidOrMatch, "dv" + getNumberName(maxValue) + "shiftormatch", "all_ones"));
                        fileUtils.writeContent(baseIntrinsics.opNot(dvNotMax2MidOrMatch, "dv" + getNumberName(maxValue) + "shiftormatch"));
                        
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("init_zero", "dh_zero_temp", dvMaxShiftOrMatch));
                        fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dh_neg1_temp", dvNotMax2MidOrMatch));
                        fileUtils.writeContent(baseIntrinsics.opOr("init_zero", "tmp_value"));
                        fileUtils.writeContent(baseIntrinsics.opLeftShift("dv_zero_shift", "init_zero", "1"));
                        fileUtils.writeContent(baseIntrinsics.opOr("dv_zero_shift", "init_zero_prevbit"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("init_zero_prevbit", "init_zero", "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("init_zero_prevbit", "word_sizem2"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opOr("dv_neg1_shift", "dv_pos1_shift", "dv_zero_shift"));
                        //fileUtils.writeContent(intrinsics.opXor("dv_neg1_shift", "all_ones"));
                        fileUtils.writeContent(baseIntrinsics.opNot("dv_neg1_shift", "dv_neg1_shift"));
                        fileUtils.writeContent(baseIntrinsics.opOr(dhMaxOrMatch, "dh_pos1_temp", "matches"));
                        fileUtils.writeContent(baseIntrinsics.opNot(dhMin2Mid, dhMaxOrMatch));
                        //fileUtils.writeContent(intrinsics.opXor(dhMin2Mid, dhMaxOrMatch, "all_ones"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("dh_zero_temp", "dv_zero_shift", dhMaxOrMatch));
                        fileUtils.writeContent(baseIntrinsics.opAnd("tmp_value", "dv_neg1_shift", dhMin2Mid));
                        fileUtils.writeContent(baseIntrinsics.opOr("dh_zero_temp", "tmp_value"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("dh_pos1_temp", "dv_neg1_shift", dhMaxOrMatch));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opOr("dh_neg1_temp", "dh_zero_temp", "dh_pos1_temp"));
                        //fileUtils.writeContent(intrinsics.opXor("dh_neg1_temp", "all_ones"));
                        fileUtils.writeContent(baseIntrinsics.opNot("dh_neg1_temp", "dh_neg1_temp"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("dh_neg1_temp", "carry_bitmask"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent("dh_pos1[j] = dh_pos1_temp;");
                        fileUtils.writeContent("dh_zero[j] = dh_zero_temp;");
                        fileUtils.writeContent("dh_neg1[j] = dh_neg1_temp;");
                        
                    }
                    fileUtils.setMarignLeft(3);
                    fileUtils.writeContent("}");
                    
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                genEditScore(false);
            }
            fileUtils.setMarignLeft(1);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
            
            
        }
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
        
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
    
    private String getDHStr(int numValue, int bitPos) {
        String dhStr = "";
        if (numValue == 0) {
            dhStr = "dhbit" + (int) Math.pow(2, bitPos) + "inverse";
        } else {
            dhStr = "dhbit" + (int) Math.pow(2, bitPos);
        }
        return dhStr;
    }
    
    private String getNumberName(int num) {
        String fieldName = "";
        if (num > 0) {
            fieldName = NumberType.POSITIVE.toString() + num;
        } else if (num < 0) {
            fieldName = NumberType.NEGATIVE.toString() + (-num);
        } else {
            fieldName = NumberType.ZERO.toString();
        }
        return fieldName;
    }
    
    private void writeBitInitStr() {
        StringBuilder result = new StringBuilder();
        String minComplementStr = GeneratorUtils.convertIntToComplementCode(minValue);
        List<String> oneList = new ArrayList<>();
        List<String> zeroList = new ArrayList<>();
        for (int i = minComplementStr.length() - 1, index = 0; index < maxBitsNum; index++, i--) {
            char c = minComplementStr.charAt(i);
            if (c == '1') {
                oneList.add("dvdh_bit" + (int) Math.pow(2, index) + "[i]");
            } else {
                zeroList.add("dvdh_bit" + (int) Math.pow(2, index) + "[i]");
            }
        }
        
        fileUtils.writeContent(baseIntrinsics.opSetValue(oneList, allOnes), 3);
        fileUtils.writeContent(baseIntrinsics.opSetValue(zeroList, "0"), 3);
        // return intrinsics.opSetValue(oneList, "all_ones") + "\n" + intrinsics.opSetValue(zeroList, "0");
    }
    
    private String getFieldName(int num, String prefixStr, String suffixStr) {
        String fieldName = "";
        if (num > 0) {
            fieldName = prefixStr + NumberType.POSITIVE.toString() + num + suffixStr;
        } else if (num < 0) {
            fieldName = prefixStr + NumberType.NEGATIVE.toString() + -num + suffixStr;
        } else {
            fieldName = prefixStr + NumberType.ZERO.toString() + suffixStr;
        }
        return fieldName;
    }
    
    
    /**
     * 根据dhbit计算dh的值,
     * <p>
     * dh将 min-max 映射到 0-(-(max-min)) , 根据dh映射后的补码值, 将dhbit进行and操作, 从而获得dh的值
     * and操作的规则为: 如果补码值的某位上为0, 和与该位对应的dhbitinverse进行and, 如果为1, 和dhbit进行and.
     * <p>
     * 在计算过程中保留中间计算结果, 以减少位操作的次数, 具体方式为定义一个map, 键值格式为 {位索引:值 位索引:值 ...}
     * 执行时从高位到低位依次and, 每次与低位and之前首先检查一下map中是否有已经and的结果, 如果有继续下一个, 如果没有进行and
     * 并将结果和对应的键值保存到map中.
     * <p>
     * dhTmpValue: 保存中间结果的变量名, 例如 dhbit32_dhbit16_dhbit8
     * dhCalcProcess: 保存算式 例如 dhbit32_dhbit16_dhbit8 = dhbit32_dhbit16 & dhbit8;
     */
    private void calcDhValues() {
        //System.out.println(Integer.toBinaryString());
        int i, j, count = 0;
        int flags[] = new int[maxBitsNum];
        for (i = 0; i < maxBitsNum; i++) {
            flags[i] = 1 << i;
        }
        
        int numBinary[][] = new int[mid2minLength][maxBitsNum];
        for (i = 0; i < mid2minLength; i++) {
            for (j = 0; j < maxBitsNum; j++) {
                numBinary[i][j] = ((count & flags[j]) != 0) ? 1 : 0;
            }
            count--;
        }
        
        dhTmpValue = new LinkedList<>();
        dhCalcProcess = new LinkedList<>();
        for (i = 0; i < maxBitsNum; i++) {
            dhCalcProcess.add(baseIntrinsics.opNot("dhbit" + (int) Math.pow(2, i) +
                    "inverse", "dhbit" + (int) Math.pow(2, i)));
        }
        Map<String, String> calcProcess = new LinkedHashMap<>();
        Map<String, String> valStore = new LinkedHashMap<>();
        String key = "";
        String value = "";
        String tmpValue = "";
        String tmpStr = "", tmpStr2 = "";
        count = minValue;
        for (i = 0; i < mid2minLength; i++) {
            
            key = (maxBitsNum - 1) + ":" + numBinary[i][maxBitsNum - 1] + " " + (maxBitsNum - 2) + ":" +
                    numBinary[i][maxBitsNum - 2] + " ";
            
            if (calcProcess.containsKey(key)) {
                tmpValue = valStore.get(key);
            } else {
                tmpStr = getDHStr(numBinary[i][maxBitsNum - 1], maxBitsNum - 1);
                tmpStr2 = getDHStr(numBinary[i][maxBitsNum - 2], maxBitsNum - 2);
                value = baseIntrinsics.opAnd(tmpStr + "_" + tmpStr2, tmpStr, tmpStr2);
                calcProcess.put(key, value);
                tmpValue = tmpStr + "_" + tmpStr2;
                valStore.put(key, tmpValue);
            }
            
            for (j = maxBitsNum - 3; j >= 0; j--) {
                key += (j + ":" + numBinary[i][j] + " ");
                if (calcProcess.containsKey(key)) {
                    tmpValue = valStore.get(key);
                } else {
                    tmpStr = getDHStr(numBinary[i][j], j);
                    if (j == 0) {
                        value = baseIntrinsics.opAnd("dh_" + getNumberName(count), tmpValue, tmpStr);
                    } else {
                        value = baseIntrinsics.opAnd(tmpValue + "_" + tmpStr, tmpValue, tmpStr);
                        tmpValue = tmpValue + "_" + tmpStr;
                        valStore.put(key, tmpValue);
                    }
                    calcProcess.put(key, value);
                }
            }
            count++;
        }
        
        for (String s : valStore.values()) {
            dhTmpValue.add(s);
        }
        
        
        for (String s : calcProcess.values()) {
            if (!s.startsWith("dh_")) {
                dhCalcProcess.add(s);
            }
        }
        
        List<String> tmpCache = new ArrayList<>();
        
        for (String s : calcProcess.values()) {
            if (s.startsWith("dh_")) {
                tmpCache.add(s);
            }
        }
        
        for (i = tmpCache.size() - 1; i >= 0; i--) {
            dhCalcProcess.add(tmpCache.get(i));
        }
        if (!isCarry) {
            dhCalcProcess.add(baseIntrinsics.opAnd("dh_" + getNumberName(minValue), "carry_bitmask"));
        }
    }
    
    private String getDvName(int value) {
        String string = null;
        if (value == midValue) {
            string = "dvnot" + maxValue + "to" + (midValue + 1) + "ormatch";
        } else if (value == maxValue) {
            string = "dv" + getNumberName(value) + "shiftormatch";
        } else {
            string = "dv" + getNumberName(value) + "shift";
        }
        return string;
        
    }
    
    /**
     * 将key按照dv值进行排序, 例如3_2_4 会返回2_3_4
     *
     * @param keyStr
     * @return
     */
    private String sortKeyStr(String keyStr) {
        String string = "";
        String[] numStr = keyStr.split(SPLIT_MARK);
        int[] numVal = new int[numStr.length];
        for (int i = 0; i < numStr.length; i++) {
            numVal[i] = Integer.parseInt(numStr[i]);
        }
        Arrays.sort(numVal);
        for (int i = 0; i < numVal.length; i++) {
            string += numVal[i] + SPLIT_MARK;
        }
        return string;
    }
    
    /**
     * 根据dv计算dvbit
     * <p>
     * dv将 min-max 映射到 0 - (max -min), 根据映射后的dv的补码值(和原码值一样)进行or操作, dvbit1表示将所有最低位为1(即2^0)的dv值进行or操作,
     * dvbit2表示将第2位(从低位开始查, 即2^1)为1的dv进行or操作, dvbit4表示将第3位(即2^3)为1的dv进行or操作, 以此类推. 有一个比较特殊值dvnotmax2mid, 该变量是
     * 按照 mid 的映射值进行or操作的.
     * <p>
     * 为了共用中间结果, 采取的策略是每个位上 or 时, 先按组 or , 然后将每组的结果 or 起来, 最低位是1个一组, 第二位是2个一组, 第三位是4个一组,
     * 以此类推, 数据结构还是使用的map, key的格式为{dv值1_dv值2...}, 过程和dh计算时类似
     */
    private void calcDvBit() {
        int i, j, k, count = 0;
        int flags[] = new int[maxBitsNum];
        for (i = 0; i < maxBitsNum; i++) {
            flags[i] = 1 << i;
        }
        
        int numBinary[][] = new int[maxSubtractMid + 1][maxBitsNum];
        count = midValue - minValue;
        
        //System.out.println("mid - min value is " + (midValue));
        for (i = 0; i <= maxSubtractMid; i++) {
            for (j = 0; j < maxBitsNum; j++) {
                numBinary[i][j] = ((count & flags[j]) != 0) ? 1 : 0;
            }
            count++;
        }
        
        String key = "";
        String value = "";
        String tmpStr = "", tmpStr2 = "";
        String tmpBitStr = null;
        String bitStr[] = new String[maxBitsNum];
        
        List<Integer> bitList[] = (List<Integer>[]) Array.newInstance(new LinkedList<Integer>().getClass(), maxBitsNum);
        
        for (i = 0; i < maxBitsNum; i++) {
            bitList[i] = new LinkedList<>();
        }
        
        for (i = maxBitsNum - 1; i >= 0; i--) {
            bitStr[i] = "";
            for (j = 0; j <= maxSubtractMid; j++) {
                if (numBinary[j][i] == 1) {
                    bitStr[i] += (j + midValue + "_");
                    bitList[i].add(midValue + j);
                }
            }
        }
        
        //        for (i = 0; i < maxBitsNum; i++) {
        //            System.out.println(bitStr[i]);
        //        }
        
        String calcStr[] = new String[maxBitsNum];
        
        Map<String, String> bitAllProcess = new LinkedHashMap<>();
        Map<String, String> bitAllValStore = new LinkedHashMap<>();
        
        
        List<Integer> tmpList;
        int lastIndex = 0;
        List<String> keyList = null;
        int remainNum[] = null;
        
        
        calcStr[0] = "";
        if (bitList[0].size() == 0) {
            //calcStr[0] = "dv_bit" + (int) Math.pow(2, 0) + " = 0;";
            calcStr[0] = baseIntrinsics.opSetValue("dv_bit" + (int) Math.pow(2, 0), "0");
        } else if (bitList[0].size() == 1) {
            //calcStr[0] = "dv_bit" + (int) Math.pow(2, 0) + " = " + getDvName(bitList[0].get(0)) + ";";
            calcStr[0] = baseIntrinsics.opEqualValue("dv_bit" + (int) Math.pow(2, 0), getDvName(bitList[0].get(0)));
        } else {
            calcStr[0] = baseIntrinsics.opOr("dv_bit" + (int) Math.pow(2, 0), getDvName(bitList[0].get(0)), getDvName
                    (bitList[0].get(1))) + " @";
            for (i = 2; i < bitList[0].size(); i++) {
                calcStr[0] += baseIntrinsics.opOr("dv_bit" + (int) Math.pow(2, 0), getDvName(bitList[0].get(i))) + " @";
            }
        }
        
        
        for (i = 1; i < bitList.length; i++) {
            tmpList = bitList[i];
            if (tmpList.size() == 0) {
                //calcStr[i] = "dv_bit" + (int) Math.pow(2, i) + " = 0;";
                calcStr[i] = baseIntrinsics.opSetValue("dv_bit" + (int) Math.pow(2, i), "0");
            } else if (tmpList.size() == 1) {
                //calcStr[i] = "dv_bit" + (int) Math.pow(2, i) + " = " + getDvName(tmpList.get(0)) + ";";
                calcStr[i] = baseIntrinsics.opEqualValue("dv_bit" + (int) Math.pow(2, i), getDvName(tmpList.get(0)));
            } else {
                calcStr[i] = "";
                List<Integer> splitIndex = new ArrayList<>();
                for (j = 0; j < tmpList.size() - 2; j++) {
                    if (tmpList.get(j + 1) - tmpList.get(j) == ((int) Math.pow(2, i) + 1)) {
                        splitIndex.add(j + 1);
                    }
                }
                
                Map<String, String> localValStore = new LinkedHashMap<>();
                
                if (splitIndex.size() > 0) {
                    int start, end;
                    String segStr = null;
                    
                    tmpBitStr = bitStr[i];
                    String strArr[] = tmpBitStr.split(SPLIT_MARK);
                    for (j = 0; j < splitIndex.size(); j++) {
                        
                        Map<String, String> segValueStore = new LinkedHashMap<>();
                        start = splitIndex.get(j);
                        if (j == splitIndex.size() - 1) {
                            end = tmpList.size();
                        } else {
                            end = start + (int) Math.pow(2, i);
                        }
                        
                        segStr = "";
                        for (k = start; k < end; k++) {
                            segStr += strArr[k] + SPLIT_MARK;
                        }
                        //segStr = bitStr[i].substring(start, end);
                        bitStr[i] = bitStr[i].replaceFirst(segStr, "");
                        
                        keyList = new ArrayList<>();
                        TreeSet<String> treeSet = new TreeSet<>(new SplitSizeFirstComparator<String>());
                        treeSet.addAll(bitAllProcess.keySet());
                        Iterator<String> iterator = treeSet.iterator();
                        while (iterator.hasNext()) {
                            keyList.add(iterator.next());
                        }
                        
                        for (k = 0; k < keyList.size(); k++) {
                            key = keyList.get(k);
                            if (segStr.contains(key)) {
                                value = bitAllValStore.get(key);
                                segValueStore.put(key, value);
                                segStr = segStr.replaceFirst(key, "");
                            }
                        }
                        
                        String numStr[];
                        if (segStr.trim().equals("")) {
                            numStr = new String[0];
                        } else {
                            numStr = segStr.split(SPLIT_MARK);
                        }
                        
                        remainNum = new int[numStr.length];
                        for (k = 0; k < numStr.length; k++) {
                            remainNum[k] = Integer.parseInt(numStr[k]);
                        }
                        
                        for (k = 0; k < remainNum.length; k += 2) {
                            
                            if (k + 1 >= remainNum.length) {
                                break;
                            }
                            key = remainNum[k] + SPLIT_MARK + remainNum[k + 1] + SPLIT_MARK;
                            
                            tmpStr = getDvName(remainNum[k]);
                            tmpStr2 = getDvName(remainNum[k + 1]);
                            
                            value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                            segValueStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                            bitAllProcess.put(key, value);
                            bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                        }
                        
                        lastIndex = k;
                        keyList = new ArrayList<>(segValueStore.keySet());
                        if (keyList.size() == 1) {
                            if (lastIndex >= remainNum.length) {
                                key = keyList.get(0);
                                value = segValueStore.get(key);
                                
                                localValStore.put(key, value);
                            } else {
                                key = sortKeyStr(keyList.get(0) + remainNum[remainNum.length - 1] + SPLIT_MARK);
                                tmpStr = segValueStore.get(keyList.get(0));
                                tmpStr2 = getDvName(remainNum[remainNum.length - 1]);
                                value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                                bitAllProcess.put(key, value);
                                bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                                localValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                            }
                        } else {
                            key = sortKeyStr(keyList.get(0) + keyList.get(1));
                            tmpStr = segValueStore.get(keyList.get(0));
                            tmpStr2 = segValueStore.get(keyList.get(1));
                            value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                            bitAllProcess.put(key, value);
                            bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                            tmpStr = tmpStr + SPLIT_MARK + tmpStr2;
                            
                            for (k = 2; k < keyList.size(); k++) {
                                key = sortKeyStr(key + keyList.get(k));
                                tmpStr2 = segValueStore.get(keyList.get(k));
                                value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                                bitAllProcess.put(key, value);
                                bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                                tmpStr = tmpStr + SPLIT_MARK + tmpStr2;
                            }
                            
                            if (lastIndex < remainNum.length) {
                                key = sortKeyStr(key + remainNum[remainNum.length - 1] + SPLIT_MARK);
                                tmpStr2 = getDvName(remainNum[remainNum.length - 1]);
                                value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                                bitAllProcess.put(key, value);
                                bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                                tmpStr = tmpStr + SPLIT_MARK + tmpStr2;
                            }
                            localValStore.put(key, tmpStr);
                        }
                    }
                }
                
                if (bitStr[i].length() != 0) {
                    keyList = new ArrayList<>();
                    TreeSet<String> treeSet = new TreeSet<>(new SplitSizeFirstComparator<String>());
                    treeSet.addAll(bitAllProcess.keySet());
                    Iterator<String> iterator = treeSet.iterator();
                    while (iterator.hasNext()) {
                        keyList.add(iterator.next());
                    }
                    
                    for (j = 0; j < keyList.size(); j++) {
                        key = keyList.get(j);
                        if (bitStr[i].contains(key)) {
                            value = bitAllValStore.get(key);
                            localValStore.put(key, value);
                            bitStr[i] = bitStr[i].replaceFirst(key, "");
                        }
                    }
                    
                    String numStr[] = null;
                    if (bitStr[i].trim().equals("")) {
                        numStr = new String[0];
                    } else {
                        numStr = bitStr[i].split(SPLIT_MARK);
                    }
                    
                    remainNum = new int[numStr.length];
                    for (j = 0; j < numStr.length; j++) {
                        remainNum[j] = Integer.parseInt(numStr[j]);
                    }
                    
                    for (j = 0; j < remainNum.length; j += 2) {
                        if (j + 1 >= remainNum.length) {
                            break;
                        }
                        key = remainNum[j] + SPLIT_MARK + remainNum[j + 1] + SPLIT_MARK;
                        tmpStr = getDvName(remainNum[j]);
                        tmpStr2 = getDvName(remainNum[j + 1]);
                        value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                        localValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                        bitAllProcess.put(key, value);
                        bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                    }
                    lastIndex = j;
                } else {
                    remainNum = new int[0];
                    lastIndex = 0;
                }
                
                keyList = new ArrayList<>(localValStore.keySet());
                if (keyList.size() == 1) {
                    if (lastIndex >= remainNum.length) {
                        calcStr[i] = "dv_bit" + (int) Math.pow(2, i) + " = " + localValStore.get(keyList.get(0)) + ";";
                    } else {
                        key = sortKeyStr(keyList.get(0) + remainNum[remainNum.length - 1] + SPLIT_MARK);
                        tmpStr = localValStore.get(keyList.get(0));
                        tmpStr2 = getDvName(remainNum[remainNum.length - 1]);
                        value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                        bitAllProcess.put(key, value);
                        bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                        calcStr[i] = "dv_bit" + (int) Math.pow(2, i) + " = " + tmpStr + SPLIT_MARK + tmpStr2 + ";";
                    }
                } else {
                    calcStr[i] = baseIntrinsics.opOr("dv_bit" + (int) Math.pow(2, i), localValStore.get(keyList.get(0)
                    ), localValStore.get(keyList.get(1))) + "\n";
                    
                    key = sortKeyStr(keyList.get(0) + keyList.get(1));
                    tmpStr = localValStore.get(keyList.get(0));
                    tmpStr2 = localValStore.get(keyList.get(1));
                    value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                    bitAllProcess.put(key, value);
                    bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                    tmpStr = tmpStr + SPLIT_MARK + tmpStr2;
                    
                    for (j = 2; j < keyList.size(); j++) {
                        key = sortKeyStr(key + keyList.get(j));
                        tmpStr2 = localValStore.get(keyList.get(j));
                        
                        value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                        bitAllProcess.put(key, value);
                        bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                        tmpStr = tmpStr + SPLIT_MARK + tmpStr2;
                    }
                    
                    if (lastIndex < remainNum.length) {
                        key = sortKeyStr(key + remainNum[remainNum.length - 1] + SPLIT_MARK);
                        tmpStr2 = getDvName(remainNum[remainNum.length - 1]);
                        value = baseIntrinsics.opOr(tmpStr + SPLIT_MARK + tmpStr2, tmpStr, tmpStr2);
                        bitAllProcess.put(key, value);
                        bitAllValStore.put(key, tmpStr + SPLIT_MARK + tmpStr2);
                        tmpStr = tmpStr + SPLIT_MARK + tmpStr2;
                        
                    }
                    calcStr[i] = "dv_bit" + (int) Math.pow(2, i) + " = " + tmpStr + ";";
                }
                
            }
            
            
        }
        
        dvTmpValue = new ArrayList<>();
        for (String s : bitAllValStore.values()) {
            dvTmpValue.add(s);
        }
        
        dvBitCalcProcess = new ArrayList<>();
        for (String s : bitAllProcess.values()) {
            dvBitCalcProcess.add(s);
        }
        
        dvBitFinalProcess = new ArrayList<>();
        String[] strArr;
        for (String s : calcStr) {
            strArr = s.split("@");
            for (String s1 : strArr) {
                dvBitFinalProcess.add(s1);
            }
            
        }
    }
    
    
    /**
     * 整数前缀, 正数-pos, 负数-neg, 0-zero
     */
    private enum NumberType {
        POSITIVE("pos"), NEGATIVE("neg"), ZERO("zero");
        
        private String value;
        
        private NumberType(String value) {
            this.value = value;
        }
        
        @Override
        public String toString() {
            return value;
        }
    }
    
    /**
     * 自定义比较器, 使用SPLIT_MARK作为分割符分割 o1 和 o2, 得到的String数组长度大的在前面
     *
     * @param <T>
     */
    private class SplitSizeFirstComparator<T> implements Comparator<T> {
        
        @Override
        public int compare(T o1, T o2) {
            String s1 = (String) o1;
            String s2 = (String) o2;
            if (s1 == null || s2 == null || s1.equals("") || s2.equals("")) {
                return s1.compareTo(s2);
            }
            int num = s1.split(SPLIT_MARK).length - s2.split(SPLIT_MARK).length;
            
            if (num == 0) {
                return s1.compareTo(s2);
            }
            return -num;
        }
    }
    
    
}
