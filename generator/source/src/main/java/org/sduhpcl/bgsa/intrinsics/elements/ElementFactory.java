package org.sduhpcl.bgsa.intrinsics.elements;

import org.sduhpcl.bgsa.util.Configuration;

/**
 * @author Jikai Zhang
 * @date 2018-04-04
 */
public class ElementFactory {
    public static BaseElement newElement(Configuration.ElementType elementType) {
        switch (elementType) {
            case E8:
                return new Element8();
            case E16:
                return new Element16();
            case E64:
                return new Element64();
            default:
                return new Element32();
        }
    }
}
