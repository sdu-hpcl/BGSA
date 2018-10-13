package org.sduhpcl.bgsa.arch;

import org.sduhpcl.bgsa.intrinsics.AVX512Intrinsics;
import org.sduhpcl.bgsa.generator.BaseGenerator;

import java.util.HashMap;

/**
 *
 * @author zhangjikai
 * @date 16-9-6
 */
public class KNLArch extends KNCArch {

    public KNLArch() {
        declarePrefix = "";
        archName = "mic";
        baseIntrinsics = new AVX512Intrinsics();
    }

    @Override
    public void initValues(BaseGenerator generator) {

        super.initValues(generator);
        HashMap<String, Object> propMap = new HashMap<String, Object>();
        propMap.put("isCarry", false);
        fillFields(generator, propMap);
    }
}
