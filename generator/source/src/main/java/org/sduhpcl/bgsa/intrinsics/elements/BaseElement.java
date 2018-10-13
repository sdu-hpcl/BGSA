package org.sduhpcl.bgsa.intrinsics.elements;

import lombok.Getter;
import lombok.Setter;

/**
 * @author Jikai Zhang
 * @date 2018-04-04
 */
@Setter
@Getter
public abstract class BaseElement {
    protected int bitSize;
    protected String allOnes;
    protected String carryBitMask;
    protected String highOne;
    protected String nextHighOne;
    protected String bitMask;
    protected String one;
}
