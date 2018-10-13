package org.sduhpcl.bgsa.intrinsics.elements;

/**
 * @author Jikai Zhang
 * @date 2018-04-04
 */
public class Element64 extends BaseElement {
    public Element64() {
        bitSize = 64;
        allOnes = "0xffffffffffffffff";
        carryBitMask = "0x7fffffffffffffff";
        highOne = "0x8000000000000000";
        nextHighOne = "0x4000000000000000";
        one = "0x0000000000000001";
    }
}
