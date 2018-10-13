package org.sduhpcl.bgsa.intrinsics.elements;

/**
 * @author Jikai Zhang
 * @date 2018-04-04
 */
public class Element8 extends BaseElement {
    public Element8() {
        bitSize = 8;
        allOnes = "0xff";
        carryBitMask = "0x7f";
        highOne = "0x80";
        nextHighOne = "0x40";
        one = "0x01";
    }
}
