package org.sduhpcl.bgsa.intrinsics.elements;

/**
 * @author Jikai Zhang
 * @date 2018-04-04
 */
public class Element32 extends BaseElement {
    public Element32() {
        bitSize = 32;
        allOnes = "0xffffffff";
        carryBitMask = "0x7fffffff";
        highOne = "0x80000000";
        nextHighOne = "0x40000000";
        one = "0x00000001";
    }
}
