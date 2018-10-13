package org.sduhpcl.bgsa.arch;

import org.sduhpcl.bgsa.intrinsics.SSEIntrinsics;
import org.sduhpcl.bgsa.generator.BaseGenerator;
import org.sduhpcl.bgsa.intrinsics.elements.BaseElement;
import org.sduhpcl.bgsa.util.Constants;
import org.sduhpcl.bgsa.util.FileUtils;

import java.util.HashMap;

/**
 * @author zhangjikai
 * @date 16-9-6
 */
public class SSEArch extends SIMDArch {
    
    public SSEArch() {
        archName = "sse";
        baseIntrinsics = new SSEIntrinsics();
    }
    
    @Override
    public void initValues(BaseGenerator generator) {
        BaseElement element = baseIntrinsics.getElement();
        HashMap<String, Object> propMap = new HashMap<>();
        propMap.put("wordType", "sse_data_t");
        propMap.put("readType", "sse_read_t");
        propMap.put("writeType", "sse_write_t");
        propMap.put("resultType", "sse_result_t");
        propMap.put("allOnes", element.getAllOnes());
        propMap.put("carryBitmask", element.getCarryBitMask());
        propMap.put("highOne", element.getHighOne());
        propMap.put("nextHighOne", element.getNextHighOne());
        propMap.put("declareStr", "void align_sse(char * ref, sse_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, sse_write_t * results, sse_data_t * dvdh_bit_mem) {");
        propMap.put("bitMask", element.getOne());
        propMap.put("bitMaskStr", "sse_read_t bitmask;");
        propMap.put("vNumStr", "SSE_V_NUM");
        propMap.put("wordStr", "SSE_WORD_SIZE");
        propMap.put("oneStr", "sse_read_t one = " + element.getOne() + ";");
        propMap.put("isMic", false);
        propMap.put("isCarry", false);
        fillFields(generator, propMap);
    }
    
    private void myersCal() {
        FileUtils fileUtils = FileUtils.getInstance();
        
        fileUtils.writeContent("tmp = _mm_and_si128(HP, maskh);");
        fileUtils.writeContent("m1=_mm_cmpeq_epi32(tmp, maskh);");
        fileUtils.writeContent("m1 = _mm_srli_epi32(m1, word_size);");
        fileUtils.writeContent("score= _mm_add_epi32(score, m1);");
        
        fileUtils.writeContent("tmp = _mm_and_si128(HN, maskh);");
        fileUtils.writeContent("m1=_mm_cmpeq_epi32(tmp, maskh);");
        fileUtils.writeContent("m1 = _mm_srli_epi32(m1, word_size);");
        fileUtils.writeContent("score= _mm_sub_epi32(score, m1);");
    }
    
    private void bandedMyersEarlyJudge() {
        FileUtils fileUtils = FileUtils.getInstance();
        fileUtils.writeContent(baseIntrinsics.opCmpgt("tmp_vector", "err", "max_err"));
        fileUtils.writeContent("cmp_result = _mm_movemask_epi8(tmp_vector);");
        fileUtils.writeContent("if(cmp_result == result_mask) {");
        {
            fileUtils.setMarignLeft(fileUtils.getMarginLeft() + 1);
            fileUtils.writeContent("for(int i = 0; i < SSE_V_NUM; i++) {");
            {
                fileUtils.setMarignLeft(fileUtils.getMarginLeft() + 1);
                fileUtils.writeContent("score[score_index + i] = MAX_ERROR;");
                fileUtils.setMarignLeft(fileUtils.getMarginLeft() - 1);
            }
            fileUtils.writeContent("}");
            fileUtils.blankLine();
            fileUtils.setMarignLeft(fileUtils.getMarginLeft() - 1);
        }
        fileUtils.writeContent("}");
    }
    
    @Override
    public void archSpecificOperation(int operationType) {
        switch (operationType) {
            
            case Constants.MYERS_CAL:
                myersCal();
                break;
            case Constants.EARLY_TERMINATE_JUDGE:
                bandedMyersEarlyJudge();
            default:
                break;
        }
    }
}
