package org.sduhpcl.bgsa.arch;

import org.sduhpcl.bgsa.intrinsics.CPUIntrinsics;
import org.sduhpcl.bgsa.generator.BaseGenerator;
import org.sduhpcl.bgsa.intrinsics.elements.BaseElement;
import org.sduhpcl.bgsa.util.Constants;
import org.sduhpcl.bgsa.util.FileUtils;

import java.util.HashMap;

/**
 * @author zhangjikai
 * @date 16-9-6
 */
public class CPUArch extends SIMDArch {
    
    public CPUArch() {
        archName = "cpu";
        baseIntrinsics = new CPUIntrinsics();
    }
    
    @Override
    public void initValues(BaseGenerator generator) {
        BaseElement element = baseIntrinsics.getElement();
        HashMap<String, Object> propMap = new HashMap<String, Object>();
        propMap.put("wordType", "cpu_data_t");
        propMap.put("readType", "cpu_read_t");
        propMap.put("writeType", "cpu_write_t");
        propMap.put("resultType", "cpu_result_t");
        propMap.put("allOnes", element.getAllOnes());
        propMap.put("carryBitmask", element.getCarryBitMask());
        propMap.put("highOne", element.getHighOne());
        propMap.put("nextHighOne", element.getNextHighOne());
        propMap.put("declareStr", "void align_cpu(char * ref, cpu_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, cpu_write_t * results, cpu_data_t * dvdh_bit_mem) {");
        propMap.put("bitMask", element.getOne());
        propMap.put("bitMaskStr", "cpu_read_t bitmask;");
        propMap.put("vNumStr", "CPU_V_NUM");
        propMap.put("wordStr", "CPU_WORD_SIZE");
        propMap.put("oneStr", "cpu_read_t one = " + element.getOne() + ";");
        propMap.put("isCarry", false);
        propMap.put("isMic", false);
        fillFields(generator, propMap);
        
    }
    
    @Override
    public void archSpecificOperation(int operationType) {
        switch (operationType) {
            case Constants.MYERS_CAL:
                myersCal();
                break;
            default:
                break;
        }
    }
    
    private void myersCal() {
        FileUtils fileUtils = FileUtils.getInstance();
        fileUtils.writeContent("if(HN & maskh)");
        fileUtils.writeContent("score--;", 4);
        fileUtils.writeContent("else if(HP & maskh)");
        fileUtils.writeContent("score++;", 4);
        
    }
}
