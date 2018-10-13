package org.sduhpcl.bgsa.arch;

import lombok.Data;
import lombok.Getter;
import lombok.Setter;
import org.sduhpcl.bgsa.intrinsics.BaseIntrinsics;
import org.sduhpcl.bgsa.generator.BaseGenerator;

import java.lang.reflect.Field;
import java.util.Map;

/**
 *
 * @author zhangjikai
 * @date 16-9-6
 */
@Getter
@Setter
public abstract class SIMDArch {

    protected BaseIntrinsics baseIntrinsics;
    
    protected String archName;
    
    protected String declarePrefix = "";

    public abstract void initValues(BaseGenerator generator);

    public abstract void archSpecificOperation(int operationType);

    public BaseIntrinsics getBaseIntrinsics() {
        return baseIntrinsics;
    }

    void fillFields(BaseGenerator generator, Map<String, Object> propMaps) {
        Class<?> clazz = BaseGenerator.class;
        Field field;

        try {

            for (Map.Entry<String, Object> entry : propMaps.entrySet()) {
                field = clazz.getDeclaredField(entry.getKey());
                field.setAccessible(true);
                field.set(generator, entry.getValue());
            }

        } catch (NoSuchFieldException | IllegalAccessException e) {
            e.printStackTrace();
        }
    }

}
