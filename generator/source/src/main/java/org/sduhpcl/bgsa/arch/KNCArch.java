package org.sduhpcl.bgsa.arch;

import org.sduhpcl.bgsa.intrinsics.AVX512Intrinsics;
import org.sduhpcl.bgsa.generator.BaseGenerator;
import org.sduhpcl.bgsa.intrinsics.elements.BaseElement;
import org.sduhpcl.bgsa.util.Constants;
import org.sduhpcl.bgsa.util.FileUtils;

import java.util.HashMap;

/**
 * @author zhangjikai
 * @date 16-9-6
 */
public class KNCArch extends SIMDArch {
    
    
    public KNCArch() {
        declarePrefix = "__ONMIC__ ";
        archName = "mic";
        baseIntrinsics = new AVX512Intrinsics();
    }
    
    
    @Override
    public void initValues(BaseGenerator generator) {
        BaseElement element = baseIntrinsics.getElement();
        HashMap<String, Object> propMap = new HashMap<String, Object>();
        propMap.put("wordType", "mic_data_t");
        propMap.put("readType", "mic_read_t");
        propMap.put("writeType", "mic_write_t");
        propMap.put("resultType", "mic_result_t");
        propMap.put("allOnes", element.getAllOnes());
        propMap.put("carryBitmask", element.getCarryBitMask());
        propMap.put("highOne", element.getHighOne());
        propMap.put("nextHighOne", element.getNextHighOne());
        propMap.put("declareStr", declarePrefix + "void align_mic(char * ref, mic_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, mic_write_t * results, mic_data_t * dvdh_bit_mem) {");
        propMap.put("bitMask", element.getOne());
        propMap.put("bitMaskStr", "sse_read_t bitmask;");
        propMap.put("vNumStr", "MIC_V_NUM");
        propMap.put("wordStr", "MIC_WORD_SIZE");
        propMap.put("oneStr", "sse_read_t one = " + element.getOne() + ";");
        propMap.put("isMic", true);
        propMap.put("isCarry", true);
        fillFields(generator, propMap);
    }
    
    
    @Override
    public void archSpecificOperation(int operationType) {
        switch (operationType) {
            case Constants.MYERS_CAL:
                myersCal();
                break;
            case Constants.EARLY_TERMINATE_JUDGE:
                checkEarlyBandedMyers();
            default:
                break;
        }
    }
    
    
    private void checkEarlyBandedMyers() {
        FileUtils fileUtils = FileUtils.getInstance();
        fileUtils.writeContent(baseIntrinsics.opCmpEpu("cmp_result", "err", "max_err", "_MM_CMPINT_GT"));
        fileUtils.writeContent("if(cmp_result == result_mask) {");
        {
            fileUtils.setMarignLeft(fileUtils.getMarginLeft() + 1);
            fileUtils.writeContent("for(int i = 0; i < MIC_V_NUM; i++) {");
            {
                fileUtils.setMarignLeft(fileUtils.getMarginLeft() + 1);
                fileUtils.writeContent("score[score_index + i] = MAX_ERROR;");
                fileUtils.setMarignLeft(fileUtils.getMarginLeft() - 1);
            }
            fileUtils.writeContent("}");
            fileUtils.blankLine();
            fileUtils.writeContent("goto end;");
            fileUtils.setMarignLeft(fileUtils.getMarginLeft() - 1);
            
        }
        fileUtils.writeContent("}");
        
    }
    
    private void myersCal() {
        System.out.println(111);
        FileUtils fileUtils = FileUtils.getInstance();
        fileUtils.writeContent("tmp = _mm512_and_epi32(HP, maskh);");
        fileUtils.writeContent("m1=_mm512_cmp_epu32_mask(tmp,maskh ,_MM_CMPINT_EQ);");
        fileUtils.writeContent("score= _mm512_mask_add_epi32(score, m1, score, one);");
        fileUtils.writeContent("tmp = _mm512_and_epi32(HN, maskh);");
        fileUtils.writeContent("m1=_mm512_cmp_epu32_mask(tmp,maskh ,_MM_CMPINT_EQ);");
        fileUtils.writeContent("score= _mm512_mask_sub_epi32(score, m1, score, one);");
    }
}
