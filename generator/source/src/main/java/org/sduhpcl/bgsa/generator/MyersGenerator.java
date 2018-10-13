package org.sduhpcl.bgsa.generator;

import org.sduhpcl.bgsa.arch.SIMDArch;
import org.sduhpcl.bgsa.util.Configuration;
import org.sduhpcl.bgsa.util.Constants;
import org.sduhpcl.bgsa.util.FileUtils;

/**
 * @author zhangjikai
 * @date 16-9-6
 */
public class MyersGenerator extends BaseGenerator {
    
    public MyersGenerator() {
        fileUtils = FileUtils.getInstance();
        initValues();
    }
    
    public MyersGenerator(SIMDArch arch) {
        this();
        setArch(arch);
    }
    
    
    @Override
    public void genKernel() {
        if (Configuration.isSemiGlobal) {
            genSemiGlobal();
            return;
        }
        if (isCarry) {
            genMyersCarry();
        } else {
            genMyersCommon();
        }
    }
    
    
    private void genMyersScore() {
        if (Configuration.isSemiGlobal) {
            fileUtils.writeContent("score = min_score;");
        }
        if (Configuration.factor != 1) {
            fileUtils.writeContent(baseIntrinsics.opMultiply("score", "factor"));
        }
        fileUtils.writeContent("int index = result_index * " + vNumStr + ";");
        fileUtils.writeContent("int * vec_dump = ((int *) & score);");
        fileUtils.writeContent("#pragma vector always");
        fileUtils.writeContent("#pragma ivdep");
        fileUtils.writeContent("for(i = 0; i < " + vNumStr + "; i++){");
        fileUtils.writeContent("results[index + i] = vec_dump[i];", 3);
        fileUtils.writeContent("}");
        fileUtils.writeContent("result_index++;");
    }
    
    private void genSemiGlobal() {
        int margin = 0;
        fileUtils.setMarignLeft(margin);
        fileUtils.writeContent(declareStr);
        fileUtils.setMarignLeft(++margin);
        fileUtils.blankLine();
        {
            fileUtils.writeContent("int i, j, k;");
            fileUtils.writeContent("int word_size = " + wordStr + ";");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " one", "1"));
            fileUtils.writeContent(wordType + " pv;");
            fileUtils.writeContent(wordType + " mv;");
            fileUtils.writeContent(wordType + " eq;");
            fileUtils.writeContent(wordType + " xh;");
            fileUtils.writeContent(wordType + " xv;");
            fileUtils.writeContent(wordType + " ph;");
            fileUtils.writeContent(wordType + " mh;");
            fileUtils.writeContent(wordType + " ph_tmp;");
            fileUtils.writeContent(wordType + " mh_tmp;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones", allOnes));
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " last_maskh",
                    bitMask + " << ((read_len - 1) % word_size)"));
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " common_maskh",
                    bitMask + " << (word_size - 1)"));
            fileUtils.writeContent("int last_shift_size = (read_len - 1) % word_size;");
            fileUtils.writeContent("int common_shift_size = word_size - 1;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " h_in", "0"));
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " h_in_tmp", "0"));
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " h_out", "0"));
            fileUtils.writeContent(wordType + " factor;");
            fileUtils.writeContent(wordType + " score;");
            if (Configuration.isSemiGlobal) {
                fileUtils.writeContent(wordType + " min_score;");
            }
            fileUtils.writeContent(readType + " * matchv;");
            fileUtils.writeContent(readType + " * read_temp = read;");
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * word_num * dvdh_len;");
            fileUtils.writeContent("char * itr;");
            fileUtils.writeContent(wordType + " * pv_arr = & dvdh_bit_mem[start];");
            fileUtils.writeContent(wordType + " * mv_arr = & dvdh_bit_mem[start + word_num * 1];");
            if (Configuration.factor != 1) {
                fileUtils.writeContent(baseIntrinsics.opSetValue("factor", Configuration.factor + ""));
            }
            fileUtils.blankLine();
            fileUtils.writeContent("for(k = 0; k < chunk_read_num; k++) {");
            fileUtils.blankLine();
            {
                fileUtils.setMarignLeft(++margin);
                fileUtils.writeContent("read =& read_temp[ k * word_num * " + vNumStr + " * CHAR_NUM];");
                fileUtils.blankLine();
                fileUtils.writeContent("for (i = 0; i < word_num; i++) {");
                {
                    fileUtils.writeContent(baseIntrinsics.opSetValue("mv_arr[i]", "0"), 3);
                    fileUtils.writeContent(baseIntrinsics.opSetValue("pv_arr[i]", allOnes), 3);
                }
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                fileUtils.writeContent(baseIntrinsics.opSetValue("score", "read_len"));
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent("min_score = score;");
                }
                
                fileUtils.writeContent("for(i = 0, itr = ref; i < ref_len; i++, itr++) {");
                {
                    fileUtils.setMarignLeft(++margin);
                    fileUtils.writeContent("matchv = & read[((int)*itr) * " + vNumStr + " * word_num];");
                    fileUtils.writeContent(baseIntrinsics.opSetValue("h_in", "0"));
                    if (Configuration.isSemiGlobal) {
                        fileUtils.writeContent(baseIntrinsics.opSetValue("h_out", "0"));
                    } else {
                        fileUtils.writeContent(baseIntrinsics.opSetValue("h_out", "1"));
                    }
                    fileUtils.writeContent("for(j = 0; j < word_num - 1; j++) {");
                    {
                        fileUtils.setMarignLeft(++margin);
                        fileUtils.writeContent("h_in = h_out;");
                        fileUtils.writeContent(baseIntrinsics.opRightShift("h_in_tmp", "h_in", "1"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("h_in_tmp", "one"));
                        fileUtils.writeContent("pv = pv_arr[j];");
                        fileUtils.writeContent("mv = mv_arr[j];");
                        fileUtils.writeContent(baseIntrinsics.opLoad("eq", "matchv"));
                        fileUtils.writeContent("matchv += " + vNumStr + ";");
                        fileUtils.writeContent(baseIntrinsics.opOr("xv", "eq", "mv"));
                        fileUtils.writeContent(baseIntrinsics.opOr("eq", "h_in_tmp"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("xh", "eq", "pv"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("xh", "pv"));
                        fileUtils.writeContent(baseIntrinsics.opXor("xh", "pv"));
                        fileUtils.writeContent(baseIntrinsics.opOr("xh", "eq"));
                        fileUtils.writeContent(baseIntrinsics.opOr("ph", "xh", "pv"));
                        fileUtils.writeContent(baseIntrinsics.opNot("ph", "ph"));
                        fileUtils.writeContent(baseIntrinsics.opOr("ph", "mv"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("mh", "pv", "xh"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("ph_tmp", "ph", "common_maskh"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("h_out", "ph_tmp", "common_shift_size"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("mh_tmp", "mh", "common_maskh"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("mh_tmp", "common_shift_size"));
                        fileUtils.writeContent(baseIntrinsics.opSubtract("h_out", "mh_tmp"));
                        fileUtils.writeContent(baseIntrinsics.opLeftShift("ph", "1"));
                        fileUtils.writeContent(baseIntrinsics.opLeftShift("mh", "1"));
                        fileUtils.writeContent(baseIntrinsics.opOr("mh", "h_in_tmp"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("h_in", "one"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("h_in", "1"));
                        fileUtils.writeContent(baseIntrinsics.opOr("ph", "h_in"));
                        fileUtils.writeContent(baseIntrinsics.opOr("pv", "xv", "ph"));
                        fileUtils.writeContent(baseIntrinsics.opNot("pv", "pv"));
                        fileUtils.writeContent(baseIntrinsics.opOr("pv", "mh"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("mv", "ph", "xv"));
                        fileUtils.writeContent("pv_arr[j] = pv;");
                        fileUtils.writeContent("mv_arr[j] = mv;");
                        fileUtils.setMarignLeft(--margin);
                        fileUtils.writeContent("}");
                    }
                    
                    fileUtils.writeContent("h_in = h_out;");
                    fileUtils.writeContent(baseIntrinsics.opRightShift("h_in_tmp", "h_in", "1"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("h_in_tmp", "one"));
                    fileUtils.writeContent("pv = pv_arr[j];");
                    fileUtils.writeContent("mv = mv_arr[j];");
                    fileUtils.writeContent(baseIntrinsics.opLoad("eq", "matchv"));
                    fileUtils.writeContent("matchv += " + vNumStr + ";");
                    fileUtils.writeContent(baseIntrinsics.opOr("xv", "eq", "mv"));
                    fileUtils.writeContent(baseIntrinsics.opOr("eq", "h_in_tmp"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("xh", "eq", "pv"));
                    fileUtils.writeContent(baseIntrinsics.opAdd("xh", "pv"));
                    fileUtils.writeContent(baseIntrinsics.opXor("xh", "pv"));
                    fileUtils.writeContent(baseIntrinsics.opOr("xh", "eq"));
                    fileUtils.writeContent(baseIntrinsics.opOr("ph", "xh", "pv"));
                    fileUtils.writeContent(baseIntrinsics.opNot("ph", "ph"));
                    fileUtils.writeContent(baseIntrinsics.opOr("ph", "mv"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("mh", "pv", "xh"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("ph_tmp", "ph", "last_maskh"));
                    fileUtils.writeContent(baseIntrinsics.opRightShift("h_out", "ph_tmp", "last_shift_size"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("mh_tmp", "mh", "last_maskh"));
                    fileUtils.writeContent(baseIntrinsics.opRightShift("mh_tmp", "last_shift_size"));
                    fileUtils.writeContent(baseIntrinsics.opSubtract("h_out", "mh_tmp"));
                    fileUtils.writeContent(baseIntrinsics.opLeftShift("ph", "1"));
                    fileUtils.writeContent(baseIntrinsics.opLeftShift("mh", "1"));
                    fileUtils.writeContent(baseIntrinsics.opOr("mh", "h_in_tmp"));
                    fileUtils.writeContent(baseIntrinsics.opAdd("h_in", "one"));
                    fileUtils.writeContent(baseIntrinsics.opRightShift("h_in", "1"));
                    fileUtils.writeContent(baseIntrinsics.opOr("ph", "h_in"));
                    fileUtils.writeContent(baseIntrinsics.opOr("pv", "xv", "ph"));
                    fileUtils.writeContent(baseIntrinsics.opNot("pv", "pv"));
                    fileUtils.writeContent(baseIntrinsics.opOr("pv", "mh"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("mv", "ph", "xv"));
                    fileUtils.writeContent("pv_arr[j] = pv;");
                    fileUtils.writeContent("mv_arr[j] = mv;");
                    fileUtils.writeContent(baseIntrinsics.opAdd("score", "h_out"));
                    if (Configuration.isSemiGlobal) {
                        fileUtils.writeContent(baseIntrinsics.opMin("min_score", "min_score", "score"));
                    }
                    fileUtils.setMarignLeft(--margin);
                    fileUtils.writeContent("}");
                }
                
                fileUtils.blankLine();
                genMyersScore();
                fileUtils.setMarignLeft(--margin);
                fileUtils.writeContent("}");
                
            }
            fileUtils.setMarignLeft(--margin);
            fileUtils.writeContent("}");
        }
        
    }
    
    private void genMyersCommon() {
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent(declareStr);
        fileUtils.setMarignLeft(1);
        fileUtils.blankLine();
        {
            fileUtils.writeContent("int i, j, k;");
            fileUtils.writeContent("int word_size = " + wordStr + " - 1;");
            fileUtils.writeContent(wordType + " * VN;");
            fileUtils.writeContent(wordType + " * VP;");
            fileUtils.writeContent(wordType + " VN_temp;");
            fileUtils.writeContent(wordType + " VP_temp;");
            fileUtils.writeContent(wordType + " PM;");
            fileUtils.writeContent(wordType + " D0;");
            fileUtils.writeContent(wordType + " HP;");
            fileUtils.writeContent(wordType + " HN;");
            fileUtils.writeContent(wordType + " HP_shift;");
            fileUtils.writeContent(wordType + " HN_shift;");
            fileUtils.writeContent(wordType + " sum;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones", allOnes));
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " carry_bitmask", carryBitmask));
            fileUtils.writeContent(wordType + " maskh;");
            fileUtils.writeContent(wordType + " factor;");
            
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " one", "1"));
            fileUtils.writeContent(wordType + " score;");
            fileUtils.writeContent(wordType + " matches;");
            fileUtils.writeContent(wordType + " tmp;");
            if (isMic) {
                fileUtils.writeContent("__mmask16 m1;");
            } else {
                fileUtils.writeContent(wordType + " m1;");
            }
            
            fileUtils.writeContent(baseIntrinsics.opSetValue("maskh", bitMask + " << ((read_len - 1) % word_size)"));
            if (Configuration.factor != 1) {
                fileUtils.writeContent(baseIntrinsics.opSetValue("factor", Configuration.factor + ""));
            }
            
            fileUtils.writeContent("char * itr;");
            fileUtils.writeContent(readType + " * matchv;");
            fileUtils.writeContent(readType + " * read_temp = read;");
            fileUtils.blankLine();
            
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * word_num * dvdh_len;");
            fileUtils.writeContent("VN = & dvdh_bit_mem[start];");
            fileUtils.writeContent("VP = & dvdh_bit_mem[start + word_num * 1];");
            fileUtils.blankLine();
            
            fileUtils.writeContent("for(k = 0; k < chunk_read_num; k++) {");
            {
                fileUtils.blankLine();
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("read =& read_temp[ k * word_num * " + vNumStr + " * CHAR_NUM];");
                fileUtils.blankLine();
                
                fileUtils.writeContent("for (i = 0; i < word_num; i++) {");
                fileUtils.writeContent(baseIntrinsics.opSetValue("VN[i]", "0"), 3);
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent(baseIntrinsics.opSetValue("VP[i]", "0"), 3);
                } else {
                    fileUtils.writeContent(baseIntrinsics.opSetValue("VP[i]", carryBitmask), 3);
                }
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                fileUtils.writeContent(baseIntrinsics.opSetValue("score", "read_len"));
                if (Configuration.isSemiGlobal) {
                    fileUtils.writeContent(wordType + " max_score = score;");
                }
                fileUtils.blankLine();
                
                fileUtils.writeContent("for(i = 0, itr = ref; i < ref_len; i++, itr++) {");
                fileUtils.setMarignLeft(3);
                fileUtils.blankLine();
                {
                    fileUtils.writeContent("matchv = & read[((int)*itr) * " + vNumStr + " * word_num];");
                    fileUtils.writeContent(baseIntrinsics.opSetValue("HP_shift", "1"));
                    fileUtils.writeContent(baseIntrinsics.opSetValue("HN_shift", "0"));
                    fileUtils.writeContent(baseIntrinsics.opSetValue("sum", "0"));
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(j = 0; j < word_num-1; j++) {");
                    fileUtils.setMarignLeft(4);
                    fileUtils.blankLine();
                    {
                        fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                        fileUtils.writeContent("matchv += " + vNumStr + ";");
                        fileUtils.writeContent("VN_temp = VN[j];");
                        fileUtils.writeContent("VP_temp = VP[j];");
                        fileUtils.writeContent(baseIntrinsics.opOr("PM", "matches", "VN_temp"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("tmp", "sum", "word_size"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("sum", "VP_temp", "PM"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("sum", "VP_temp"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("sum", "tmp"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("D0", "sum", "carry_bitmask"));
                        fileUtils.writeContent(baseIntrinsics.opXor("D0", "VP_temp"));
                        fileUtils.writeContent(baseIntrinsics.opOr("D0", "PM"));
                        fileUtils.writeContent(baseIntrinsics.opOr("HP", "D0", "VP_temp"));
                        fileUtils.writeContent(baseIntrinsics.opNot("HP", "HP"));
                        fileUtils.writeContent(baseIntrinsics.opOr("HP", "VN_temp"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("HN", "D0", "VP_temp"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opLeftShift("HP", "1"));
                        fileUtils.writeContent(baseIntrinsics.opOr("HP", "HP_shift"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("HP_shift", "HP", "word_size"));
                        
                        fileUtils.writeContent(baseIntrinsics.opLeftShift("HN", "1"));
                        fileUtils.writeContent(baseIntrinsics.opOr("HN", "HN_shift"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("HN_shift", "HN", "word_size"));
                        fileUtils.writeContent(baseIntrinsics.opOr("VP[j]", "D0", "HP"));
                        fileUtils.writeContent(baseIntrinsics.opNot("VP[j]", "VP[j]"));
                        fileUtils.writeContent(baseIntrinsics.opOr("VP[j]", "HN"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("VP[j]", "carry_bitmask"));
                        
                        fileUtils.writeContent(baseIntrinsics.opAnd("VN[j]", "D0", "HP"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("VN[j]", "carry_bitmask"));
                    }
                    fileUtils.setMarignLeft(3);
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                    fileUtils.writeContent("VN_temp = VN[word_num - 1];");
                    fileUtils.writeContent("VP_temp = VP[word_num - 1];");
                    fileUtils.writeContent(baseIntrinsics.opOr("PM", "matches", "VN_temp"));
                    fileUtils.writeContent(baseIntrinsics.opRightShift("tmp", "sum", "word_size"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("sum", "VP_temp", "PM"));
                    fileUtils.writeContent(baseIntrinsics.opAdd("sum", "VP_temp"));
                    fileUtils.writeContent(baseIntrinsics.opAdd("sum", "tmp"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("D0", "sum", "carry_bitmask"));
                    fileUtils.writeContent(baseIntrinsics.opXor("D0", "VP_temp"));
                    fileUtils.writeContent(baseIntrinsics.opOr("D0", "PM"));
                    fileUtils.writeContent(baseIntrinsics.opOr("HP", "D0", "VP_temp"));
                    fileUtils.writeContent(baseIntrinsics.opNot("HP", "HP"));
                    fileUtils.writeContent(baseIntrinsics.opOr("HP", "VN_temp"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("HN", "D0", "VP_temp"));
                    fileUtils.blankLine();
                    
                    arch.archSpecificOperation(Constants.MYERS_CAL);
                    
                    if (Configuration.isSemiGlobal) {
                        fileUtils.writeContent(baseIntrinsics.opMax("max_score", "score", "max_score"));
                    }
                    fileUtils.writeContent(baseIntrinsics.opLeftShift("HP", "1"));
                    fileUtils.writeContent(baseIntrinsics.opOr("HP", "HP_shift"));
                    
                    fileUtils.writeContent(baseIntrinsics.opLeftShift("HN", "1"));
                    fileUtils.writeContent(baseIntrinsics.opOr("HN", "HN_shift"));
                    fileUtils.writeContent(baseIntrinsics.opOr("VP[word_num - 1]", "D0", "HP"));
                    fileUtils.writeContent(baseIntrinsics.opNot("VP[word_num - 1]", "VP[word_num - 1]"));
                    fileUtils.writeContent(baseIntrinsics.opOr("VP[word_num - 1]", "HN"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("VP[word_num - 1]", "carry_bitmask"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("VN[word_num - 1]", "D0", "HP"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("VN[word_num - 1]", "carry_bitmask"));
                    
                    
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                //generateMyersScore();
                genMyersScore();
            }
            fileUtils.setMarignLeft(1);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
        }
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
    }
    
    
    private void genMyersCarry() {
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent(declareStr);
        fileUtils.setMarignLeft(1);
        fileUtils.blankLine();
        {
            fileUtils.writeContent("int i, j, k;");
            fileUtils.writeContent("int word_size = " + wordStr + " - 1;");
            fileUtils.writeContent(wordType + " * VN;");
            fileUtils.writeContent(wordType + " * VP;");
            fileUtils.writeContent(wordType + " VN_temp;");
            fileUtils.writeContent(wordType + " VP_temp;");
            fileUtils.writeContent(wordType + " PM;");
            fileUtils.writeContent(wordType + " D0;");
            fileUtils.writeContent(wordType + " HP;");
            fileUtils.writeContent(wordType + " HN;");
            fileUtils.writeContent(wordType + " HP_shift;");
            fileUtils.writeContent(wordType + " HN_shift;");
            fileUtils.writeContent(wordType + " sum;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones", allOnes));
            
            fileUtils.writeContent(wordType + " maskh;");
            fileUtils.writeContent(wordType + " factor;");
            fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " one", "1"));
            fileUtils.writeContent("__mmask16 carry = 0x0000;");
            fileUtils.writeContent("__mmask16 carry_temp = 0x0000;");
            fileUtils.writeContent("__mmask16 m1;");
            fileUtils.writeContent(wordType + " score;");
            fileUtils.writeContent(wordType + " matches;");
            fileUtils.writeContent(wordType + " tmp;");
            
            
            fileUtils.writeContent(baseIntrinsics.opSetValue("maskh", bitMask + " << ((read_len - 1) % MIC_WORD_SIZE)"));
            if (Configuration.factor != 1) {
                fileUtils.writeContent(baseIntrinsics.opSetValue("factor", Configuration.factor + ""));
            }
            fileUtils.blankLine();
            
            fileUtils.writeContent("char * itr;");
            fileUtils.writeContent(readType + " * matchv;");
            fileUtils.writeContent(readType + " * read_temp = read;");
            fileUtils.blankLine();
            
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * word_num * dvdh_len;");
            fileUtils.writeContent("VN = & dvdh_bit_mem[start];");
            fileUtils.writeContent("VP = & dvdh_bit_mem[start + word_num * 1];");
            fileUtils.blankLine();
            
            fileUtils.writeContent("for(k = 0; k < chunk_read_num; k++) {");
            fileUtils.blankLine();
            fileUtils.setMarignLeft(2);
            {
                fileUtils.writeContent("read =& read_temp[ k * word_num * " + vNumStr + " * CHAR_NUM];");
                fileUtils.blankLine();
                
                fileUtils.writeContent("for (i = 0; i < word_num; i++) {");
                fileUtils.writeContent(baseIntrinsics.opSetValue("VN[i]", "0"), 3);
                fileUtils.writeContent(baseIntrinsics.opSetValue("VP[i]", allOnes), 3);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                fileUtils.writeContent(baseIntrinsics.opSetValue("score", "read_len"));
                fileUtils.blankLine();
                
                fileUtils.writeContent("for(i = 0, itr = ref; i < ref_len; i++, itr++) {");
                fileUtils.setMarignLeft(3);
                fileUtils.blankLine();
                {
                    fileUtils.writeContent("matchv = & read[((int)*itr) * " + vNumStr + " * word_num];");
                    fileUtils.writeContent(baseIntrinsics.opSetValue("HP_shift", "1"));
                    fileUtils.writeContent(baseIntrinsics.opSetValue("HN_shift", "0"));
                    fileUtils.writeContent(baseIntrinsics.opSetValue("sum", "0"));
                    fileUtils.writeContent(baseIntrinsics.opEqualValue("carry", "0x0000"));
                    fileUtils.writeContent(baseIntrinsics.opEqualValue("carry_temp", "0x0000"));
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(j = 0; j < word_num-1; j++) {");
                    fileUtils.setMarignLeft(4);
                    fileUtils.blankLine();
                    {
                        fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                        fileUtils.writeContent("matchv += " + vNumStr + ";");
                        fileUtils.writeContent("VN_temp = VN[j];");
                        fileUtils.writeContent("VP_temp = VP[j];");
                        fileUtils.writeContent(baseIntrinsics.opOr("PM", "matches", "VN_temp"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("sum", "VP_temp", "PM"));
                        fileUtils.writeContent("#ifdef __MIC__");
                        fileUtils.writeContent(baseIntrinsics.opAdc("sum", "sum", "carry", "VP_temp", "&carry_temp"));
                        fileUtils.writeContent("#endif");
                        fileUtils.writeContent("carry = carry_temp;");
                        
                        fileUtils.writeContent(baseIntrinsics.opXor("D0", "sum", "VP_temp"));
                        fileUtils.writeContent(baseIntrinsics.opOr("D0", "PM"));
                        fileUtils.writeContent(baseIntrinsics.opOr("HP", "D0", "VP_temp"));
                        fileUtils.writeContent(baseIntrinsics.opNot("HP", "HP"));
                        fileUtils.writeContent(baseIntrinsics.opOr("HP", "VN_temp"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("HN", "D0", "VP_temp"));
                        fileUtils.blankLine();
                        
                        fileUtils.writeContent(baseIntrinsics.opRightShift("tmp", "HP", "word_size"));
                        fileUtils.writeContent(baseIntrinsics.opLeftShift("HP", "1"));
                        fileUtils.writeContent(baseIntrinsics.opOr("HP", "HP_shift"));
                        fileUtils.writeContent("HP_shift = tmp;");
                        
                        fileUtils.writeContent(baseIntrinsics.opRightShift("tmp", "HN", "word_size"));
                        fileUtils.writeContent(baseIntrinsics.opLeftShift("HN", "1"));
                        fileUtils.writeContent(baseIntrinsics.opOr("HN", "HN_shift"));
                        fileUtils.writeContent("HN_shift = tmp;");
                        
                        fileUtils.writeContent(baseIntrinsics.opOr("VP[j]", "D0", "HP"));
                        fileUtils.writeContent(baseIntrinsics.opNot("VP[j]", "VP[j]"));
                        fileUtils.writeContent(baseIntrinsics.opOr("VP[j]", "HN"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("VN[j]", "D0", "HP"));
                    }
                    fileUtils.setMarignLeft(3);
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent(baseIntrinsics.opLoad("matches", "matchv"));
                    fileUtils.writeContent("matchv += " + vNumStr + ";");
                    fileUtils.writeContent("VN_temp = VN[j];");
                    fileUtils.writeContent("VP_temp = VP[j];");
                    fileUtils.writeContent(baseIntrinsics.opOr("PM", "matches", "VN_temp"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("sum", "VP_temp", "PM"));
                    fileUtils.writeContent("#ifdef __MIC__");
                    fileUtils.writeContent(baseIntrinsics.opAdc("sum", "sum", "carry", "VP_temp", "&carry_temp"));
                    fileUtils.writeContent("#endif");
                    fileUtils.writeContent("carry = carry_temp;");
                    
                    fileUtils.writeContent(baseIntrinsics.opXor("D0", "sum", "VP_temp"));
                    fileUtils.writeContent(baseIntrinsics.opOr("D0", "PM"));
                    fileUtils.writeContent(baseIntrinsics.opOr("HP", "D0", "VP_temp"));
                    fileUtils.writeContent(baseIntrinsics.opNot("HP", "HP"));
                    fileUtils.writeContent(baseIntrinsics.opOr("HP", "VN_temp"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("HN", "D0", "VP_temp"));
                    fileUtils.blankLine();
                    
                    //generateMyersCal();
                    arch.archSpecificOperation(Constants.MYERS_CAL);
                    
                    
                    fileUtils.writeContent(baseIntrinsics.opRightShift("tmp", "HP", "word_size"));
                    fileUtils.writeContent(baseIntrinsics.opLeftShift("HP", "1"));
                    fileUtils.writeContent(baseIntrinsics.opOr("HP", "HP_shift"));
                    fileUtils.writeContent("HP_shift = tmp;");
                    
                    fileUtils.writeContent(baseIntrinsics.opRightShift("tmp", "HN", "word_size"));
                    fileUtils.writeContent(baseIntrinsics.opLeftShift("HN", "1"));
                    fileUtils.writeContent(baseIntrinsics.opOr("HN", "HN_shift"));
                    fileUtils.writeContent("HN_shift = tmp;");
                    
                    fileUtils.writeContent(baseIntrinsics.opOr("VP[j]", "D0", "HP"));
                    fileUtils.writeContent(baseIntrinsics.opNot("VP[j]", "VP[j]"));
                    fileUtils.writeContent(baseIntrinsics.opOr("VP[j]", "HN"));
                    fileUtils.writeContent(baseIntrinsics.opAnd("VN[j]", "D0", "HP"));
                    
                    
                }
                fileUtils.setMarignLeft(2);
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                genMyersScore();
            }
            fileUtils.setMarignLeft(1);
            fileUtils.writeContent("}");
            fileUtils.blankLine();
        }
        
        fileUtils.setMarignLeft(0);
        fileUtils.writeContent("}");
        fileUtils.blankLine();
    }
    
    
}
