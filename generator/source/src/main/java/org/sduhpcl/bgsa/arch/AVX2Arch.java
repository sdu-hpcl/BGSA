package org.sduhpcl.bgsa.arch;

import org.sduhpcl.bgsa.intrinsics.AVX2Intrinsics;
import org.sduhpcl.bgsa.generator.BaseGenerator;
import org.sduhpcl.bgsa.intrinsics.elements.BaseElement;
import org.sduhpcl.bgsa.util.Constants;
import org.sduhpcl.bgsa.util.FileUtils;

import java.util.HashMap;
import java.util.Map;

/**
 * @author zhangjikai
 * @date 17-3-6
 */
public class AVX2Arch extends SIMDArch {
    
    public AVX2Arch() {
        archName = "avx";
        baseIntrinsics = new AVX2Intrinsics();
    }
    
    @Override
    public void initValues(BaseGenerator generator) {
        BaseElement element = baseIntrinsics.getElement();
        Map<String, Object> propMap = new HashMap<>();
        propMap.put("wordType", "avx_data_t");
        propMap.put("readType", "avx_read_t");
        propMap.put("writeType", "avx_write_t");
        propMap.put("resultType", "avx_result_t");
        propMap.put("allOnes", element.getAllOnes());
        propMap.put("carryBitmask", element.getCarryBitMask());
        propMap.put("highOne", element.getHighOne());
        propMap.put("nextHighOne", element.getNextHighOne());
        propMap.put("declareStr", "void align_avx(char * ref, avx_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, avx_write_t * results, avx_data_t * dvdh_bit_mem) {");
        propMap.put("bitMask", element.getOne());
        propMap.put("bitMaskStr", "avx_read_t bitmask;");
        propMap.put("vNumStr", "AVX_V_NUM");
        propMap.put("wordStr", "AVX_WORD_SIZE");
        propMap.put("oneStr", "avx_read_t one = " + element.getOne() + ";");
        propMap.put("isMic", false);
        propMap.put("isCarry", false);
        fillFields(generator, propMap);
        
    }
    
    
    private void myersCal() {
        FileUtils fileUtils = FileUtils.getInstance();
        fileUtils.writeContent("tmp = _mm256_and_si256(HP, maskh);");
        fileUtils.writeContent("m1=_mm256_cmpeq_epi32(tmp, maskh);");
        fileUtils.writeContent("m1 = _mm256_srli_epi32(m1, word_size);");
        fileUtils.writeContent("score= _mm256_add_epi32(score, m1);");
        
        fileUtils.writeContent("tmp = _mm256_and_si256(HN, maskh);");
        fileUtils.writeContent("m1=_mm256_cmpeq_epi32(tmp, maskh);");
        fileUtils.writeContent("m1 = _mm256_srli_epi32(m1, word_size);");
        fileUtils.writeContent("score= _mm256_sub_epi32(score, m1);");
        
    }
    
    private void bandedMyersEarlyJudge() {
        FileUtils fileUtils = FileUtils.getInstance();
        fileUtils.writeContent(baseIntrinsics.opCmpgt("tmp_vector", "err", "max_err"));
        fileUtils.writeContent("cmp_result = _mm256_movemask_epi8(tmp_vector);");
        fileUtils.writeContent("if(cmp_result == result_mask) {");
        {
            fileUtils.setMarignLeft(fileUtils.getMarginLeft() + 1);
            fileUtils.writeContent("for(int i = 0; i < AVX_V_NUM; i++) {");
            {
                fileUtils.setMarignLeft(fileUtils.getMarginLeft() + 1);
                fileUtils.writeContent("score[score_index + i] = MAX_ERROR;");
                fileUtils.setMarignLeft(fileUtils.getMarginLeft() - 1);
            }
            fileUtils.writeContent("}");
            fileUtils.writeContent("goto end;");
            fileUtils.blankLine();
            fileUtils.setMarignLeft(fileUtils.getMarginLeft() - 1);
        }
        fileUtils.writeContent("}");
    }
    
    @Override
    public void archSpecificOperation(int operationType) {
        switch (operationType) {
            case Constants.EARLY_TERMINATE_JUDGE:
                bandedMyersEarlyJudge();
                break;
            case Constants.MYERS_CAL:
                myersCal();
                break;
            default:
                break;
        }
    }
}
