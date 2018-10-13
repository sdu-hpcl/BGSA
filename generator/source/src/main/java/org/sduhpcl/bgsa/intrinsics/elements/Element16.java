package org.sduhpcl.bgsa.intrinsics.elements;

/**
 * @author Jikai Zhang
 * @date 2018-04-04
 */
public class Element16 extends BaseElement {
    public Element16() {
        bitSize = 16;
        allOnes = "0xffff";
        carryBitMask = "0x7fff";
        highOne = "0x8000";
        nextHighOne = "0x4000";
        one = "0x0001";
    }
}
