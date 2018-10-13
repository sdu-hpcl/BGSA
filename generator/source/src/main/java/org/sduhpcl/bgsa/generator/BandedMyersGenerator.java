package org.sduhpcl.bgsa.generator;

import org.sduhpcl.bgsa.arch.SIMDArch;
import org.sduhpcl.bgsa.arch.SSEArch;
import org.sduhpcl.bgsa.util.Constants;
import org.sduhpcl.bgsa.util.FileUtils;
import sun.reflect.misc.FieldUtil;

/**
 * @author Jikai Zhang
 * @date 2018-04-04
 */
public class BandedMyersGenerator extends BaseGenerator {
    
    public BandedMyersGenerator() {
        fileUtils = FileUtils.getInstance();
        initValues();
    }
    
    public BandedMyersGenerator(SIMDArch arch) {
        this();
        setArch(arch);
    }
    
    @Override
    public void genKernel() {
        genDefine();
        genContent();
    }
    
    private void genDefine() {
        int startMargin = 0;
        fileUtils.setMarignLeft(startMargin);
        fileUtils.writeContent("#define cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c) \\");
        {
            fileUtils.setMarignLeft(++startMargin);
            fileUtils.writeContent(baseIntrinsics.opOr("X", "peq[c]", "VN") + " \\");
            fileUtils.writeContent(baseIntrinsics.opAnd("D0", "X", "VP") + " \\");
            fileUtils.writeContent(baseIntrinsics.opAdd("D0", "D0", "VP") + " \\");
            fileUtils.writeContent(baseIntrinsics.opXor("D0", "D0", "VP") + " \\");
            fileUtils.writeContent(baseIntrinsics.opOr("D0", "D0", "X") + " \\");
            fileUtils.writeContent(baseIntrinsics.opAnd("HN", "D0", "VP") + " \\");
            fileUtils.writeContent(baseIntrinsics.opOr("HP", "D0", "VP") + " \\");
            fileUtils.writeContent(baseIntrinsics.opXor("HP", "HP", "all_ones_mask") + " \\");
            fileUtils.writeContent(baseIntrinsics.opOr("HP", "HP", "VN") + " \\");
            fileUtils.writeContent(baseIntrinsics.opRightShift("X", "D0", "1") + " \\");
            fileUtils.writeContent(baseIntrinsics.opAnd("VN", "X", "HP") + " \\");
            fileUtils.writeContent(baseIntrinsics.opOr("VP", "HP", "X") + " \\");
            fileUtils.writeContent(baseIntrinsics.opXor("VP", "VP", "all_ones_mask") + " \\");
            fileUtils.writeContent(baseIntrinsics.opOr("VP", "VP", "HN"));
            fileUtils.setMarignLeft(--startMargin);
        }
        fileUtils.blankLine();
        
        fileUtils.writeContent("#define move_peq(peq) \\");
        {
            fileUtils.setMarignLeft(++startMargin);
            fileUtils.writeContent(baseIntrinsics.opRightShift("peq[0]", "peq[0]", "1") + " \\");
            fileUtils.writeContent(baseIntrinsics.opRightShift("peq[1]", "peq[1]", "1") + " \\");
            fileUtils.writeContent(baseIntrinsics.opRightShift("peq[2]", "peq[2]", "1") + " \\");
            fileUtils.writeContent(baseIntrinsics.opRightShift("peq[3]", "peq[3]", "1") + " \\");
            fileUtils.setMarignLeft(--startMargin);
        }
        fileUtils.blankLine();
        
        fileUtils.writeContent("#define or_peq(tmp_peq, peq, bit_index, one, band_down) \\");
        {
            fileUtils.setMarignLeft(++startMargin);
            fileUtils.writeContent(baseIntrinsics.opRightShift("tmp_vector", "tmp_peq[0]", "bit_index") + " \\");
            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_vector", "tmp_vector", "one") + " \\");
            fileUtils.writeContent(baseIntrinsics.opLeftShift("tmp_vector", "tmp_vector", "band_down") + " \\");
            fileUtils.writeContent(baseIntrinsics.opOr("peq[0]", "peq[0]", "tmp_vector") + " \\");
            fileUtils.writeContent(baseIntrinsics.opRightShift("tmp_vector", "tmp_peq[1]", "bit_index") + " \\");
            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_vector", "tmp_vector", "one") + " \\");
            fileUtils.writeContent(baseIntrinsics.opLeftShift("tmp_vector", "tmp_vector", "band_down") + " \\");
            fileUtils.writeContent(baseIntrinsics.opOr("peq[1]", "peq[1]", "tmp_vector") + " \\");
            fileUtils.writeContent(baseIntrinsics.opRightShift("tmp_vector", "tmp_peq[2]", "bit_index") + " \\");
            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_vector", "tmp_vector", "one") + " \\");
            fileUtils.writeContent(baseIntrinsics.opLeftShift("tmp_vector", "tmp_vector", "band_down") + " \\");
            fileUtils.writeContent(baseIntrinsics.opOr("peq[2]", "peq[2]", "tmp_vector") + " \\");
            fileUtils.writeContent(baseIntrinsics.opRightShift("tmp_vector", "tmp_peq[3]", "bit_index") + " \\");
            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_vector", "tmp_vector", "one") + " \\");
            fileUtils.writeContent(baseIntrinsics.opLeftShift("tmp_vector", "tmp_vector", "band_down") + " \\");
            fileUtils.writeContent(baseIntrinsics.opOr("peq[3]", "peq[3]", "tmp_vector"));
            fileUtils.setMarignLeft(--startMargin);
        }
        fileUtils.blankLine();
        
        fileUtils.writeContent("#define cal_score(tmp_vector, D0, one, err) \\");
        {
            fileUtils.setMarignLeft(++startMargin);
            fileUtils.writeContent(baseIntrinsics.opAnd("tmp_vector", "D0", "one") + " \\");
            fileUtils.writeContent(baseIntrinsics.opSubtract("tmp_vector", "one", "tmp_vector") + " \\");
            fileUtils.writeContent(baseIntrinsics.opAdd("err", "tmp_vector"));
            fileUtils.setMarignLeft(--startMargin);
        }
        fileUtils.blankLine();
    }
    
    private void genContent() {
        int startMargin = 0;
        fileUtils.setMarignLeft(startMargin);
        String declare = String.format("void align_%s( char * query, %s_read_t * read, " +
                        "int query_len, int subject_len, int word_num, int chunk_read_num, int result_index, " +
                        "%s_write_t * score, %s_data_t * dvdh_bit_mem) {", arch.getArchName(), arch.getArchName(),
                arch.getArchName(), arch.getArchName());
        declare = arch.getDeclarePrefix() + declare;
        fileUtils.writeContent(declare);
        
        {
            fileUtils.setMarignLeft(++startMargin);
            fileUtils.writeContent("int h_threshold = threshold + subject_len - query_len;");
            fileUtils.writeContent("int band_length = threshold + h_threshold + 1;");
            fileUtils.writeContent("int band_down = band_length - 1;");
            fileUtils.writeContent("int tid = omp_get_thread_num();");
            fileUtils.writeContent("int start = tid * dvdh_len;");
            fileUtils.writeContent(String.format("%s * peq = & dvdh_bit_mem[start];", wordType));
            fileUtils.writeContent(String.format("%s * tmp_peq = & dvdh_bit_mem[start + CHAR_NUM];", wordType));
            fileUtils.writeContent(String.format("%s * dist;", readType));
            fileUtils.writeContent(String.format("int wv_num = word_num * %s;", vNumStr));
            fileUtils.blankLine();
            
            fileUtils.writeContent("for(int chunk_index = 0; chunk_index < chunk_read_num; chunk_index++) {\n");
            {
                fileUtils.setMarignLeft(++startMargin);
                fileUtils.writeContent(String.format("int score_index =  result_index * %s;", vNumStr));
                fileUtils.writeContent("result_index++;");
                fileUtils.writeContent(String.format("dist = & read[chunk_index * word_num * %s * CHAR_NUM];",
                        vNumStr));
                fileUtils.writeContent(baseIntrinsics.opLoad("peq[0]", "dist"));
                fileUtils.writeContent(baseIntrinsics.opLoad("peq[1]", "(dist + wv_num)"));
                fileUtils.writeContent(baseIntrinsics.opLoad("peq[2]", "(dist + wv_num * 2)"));
                fileUtils.writeContent(baseIntrinsics.opLoad("peq[3]", "(dist + wv_num * 3)"));
                fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[0]", String.format("(dist + %s)", vNumStr)));
                fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[1]", String.format("(dist + wv_num + %s)",
                        vNumStr)));
                fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[2]", String.format("(dist + wv_num * 2 + %s)",
                        vNumStr)));
                fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[3]", String.format("(dist + wv_num * 3 + %s)",
                        vNumStr)));
                fileUtils.blankLine();
                
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " VN", "0"));
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " VP", "0"));
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " X", "0"));
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " D0", "0"));
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " HN", "0"));
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " HP", "0"));
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " all_ones_mask", allOnes));
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " one",
                        baseIntrinsics.getElement().getOne()));
                fileUtils.writeContent(wordType + " err_mask = one;");
                fileUtils.writeContent(wordType + " tmp_vector;");
                fileUtils.blankLine();
                
                fileUtils.writeContent("int i_bd = h_threshold;");
                fileUtils.writeContent("int last_bits = h_threshold;");
                fileUtils.writeContent("int bit_index = 0;");
                //fileUtils.writeContent("int16_t result_mask = 0xffff;");
                fileUtils.writeContent("cmp_result_t cmp_result;");
                fileUtils.writeContent("int query_index = 0;");
                fileUtils.blankLine();
                
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " err", "threshold"));
                fileUtils.writeContent(baseIntrinsics.opSetValue(wordType + " max_err", "threshold + last_bits + 1"));
                fileUtils.blankLine();
                
                fileUtils.writeContent("for(; query_index < threshold; query_index++) {");
                {
                    fileUtils.setMarignLeft(++startMargin);
                    fileUtils.writeContent("int c = query[query_index];");
                    fileUtils.writeContent("cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);");
                    fileUtils.writeContent("move_peq(peq);");
                    fileUtils.writeContent("or_peq(tmp_peq, peq, bit_index, one, band_down);");
                    fileUtils.writeContent("bit_index++;");
                    fileUtils.writeContent("i_bd++;");
                    fileUtils.setMarignLeft(--startMargin);
                }
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                fileUtils.writeContent(String.format("int length = %s < query_len ? %s : query_len; ",
                        wordStr, wordStr));
                fileUtils.writeContent("for(; query_index < length; query_index++) {");
                {
                    fileUtils.setMarignLeft(++startMargin);
                    fileUtils.writeContent("int c = query[query_index];");
                    fileUtils.writeContent("cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);");
                    fileUtils.writeContent("cal_score(tmp_vector, D0, one, err);");
                    fileUtils.writeContent("move_peq(peq);");
                    fileUtils.writeContent("or_peq(tmp_peq, peq, bit_index, one, band_down);");
                    fileUtils.writeContent("bit_index++;");
                    fileUtils.writeContent("i_bd++;");
                    fileUtils.setMarignLeft(--startMargin);
                }
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                // fileUtils.writeContent("tmp_vector = _mm_cmpgt_epi64(err, max_err);");
                // fileUtils.writeContent("int16_t cmp_result = _mm_movemask_epi8(tmp_vector);");
                // fileUtils.writeContent("if(cmp_result == result_mask) {");
                // fileUtils.writeContent("goto end;", startMargin + 1);
                // fileUtils.writeContent("}");
                
                arch.archSpecificOperation(Constants.EARLY_TERMINATE_JUDGE);
                
                fileUtils.blankLine();
                
                fileUtils.writeContent(String.format("if(query_len > %s) {", wordStr));
                {
                    fileUtils.setMarignLeft(++startMargin);
                    fileUtils.writeContent("bit_index = 0;");
                    fileUtils.writeContent("int rest_length = subject_len - i_bd;");
                    fileUtils.writeContent("int batch_count = rest_length / batch_size;");
                    fileUtils.writeContent(String.format("int word_count = rest_length / %s;", wordStr));
                    fileUtils.writeContent(String.format("int word_batch_count = %s / batch_size;", wordStr));
                    fileUtils.writeContent("int batch_index = 0;");
                    fileUtils.writeContent("int word_index = 2;");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[0]", String.format("(dist + word_index * " +
                            "%s)", vNumStr)));
                    fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[1]", String.format("(dist + wv_num + " +
                            "word_index * %s)", vNumStr)));
                    fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[2]", String.format("(dist + wv_num * 2 + " +
                            "word_index *  %s)", vNumStr)));
                    fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[3]", String.format("(dist + wv_num * 3 " +
                            "+ word_index * %s)", vNumStr)));
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(int i = 0; i < word_count; i++) {");
                    {
                        fileUtils.setMarignLeft(++startMargin);
                        fileUtils.writeContent("for(int j = 0; j < word_batch_count; j++) {");
                        {
                            fileUtils.setMarignLeft(++startMargin);
                            fileUtils.writeContent("for(int k = 0; k < batch_size; k+=1) {");
                            {
                                fileUtils.setMarignLeft(++startMargin);
                                fileUtils.writeContent("int c = query[query_index];");
                                fileUtils.writeContent("cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);");
                                fileUtils.writeContent("cal_score(tmp_vector, D0, one, err);");
                                fileUtils.writeContent("move_peq(peq);");
                                fileUtils.writeContent("or_peq(tmp_peq, peq, bit_index, one, band_down);");
                                fileUtils.writeContent("bit_index++;");
                                fileUtils.writeContent("i_bd++; ");
                                fileUtils.writeContent("query_index++;");
                                fileUtils.setMarignLeft(--startMargin);
                            }
                            fileUtils.writeContent("}");
                            fileUtils.blankLine();
                            
                            arch.archSpecificOperation(Constants.EARLY_TERMINATE_JUDGE);
                            fileUtils.writeContent("batch_index++;");
                            fileUtils.setMarignLeft(--startMargin);
                        }
                        fileUtils.writeContent("}");
                        fileUtils.writeContent("bit_index = 0;");
                        fileUtils.writeContent("word_index++;");
                        fileUtils.blankLine();
                        fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[0]", String.format("(dist + word_index * " +
                                "%s)", vNumStr)));
                        fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[1]", String.format("(dist + wv_num + " +
                                "word_index * %s)", vNumStr)));
                        fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[2]", String.format("(dist + wv_num * 2 + " +
                                "word_index *  %s)", vNumStr)));
                        fileUtils.writeContent(baseIntrinsics.opLoad("tmp_peq[3]", String.format("(dist + wv_num * 3 " +
                                "+ word_index * %s)", vNumStr)));
                        fileUtils.setMarignLeft(--startMargin);
                    }
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(; batch_index < batch_count; batch_index++) {");
                    {
                        fileUtils.setMarignLeft(++startMargin);
                        fileUtils.writeContent("for(int k = 0; k < batch_size; k+=1) {");
                        {
                            fileUtils.setMarignLeft(++startMargin);
                            fileUtils.writeContent("int c = query[query_index];");
                            fileUtils.writeContent("cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);");
                            fileUtils.writeContent("cal_score(tmp_vector, D0, one, err);");
                            fileUtils.writeContent("move_peq(peq);");
                            fileUtils.writeContent("or_peq(tmp_peq, peq, bit_index, one, band_down);");
                            fileUtils.writeContent("bit_index++;");
                            fileUtils.writeContent("i_bd++;");
                            fileUtils.writeContent("query_index++;");
                            fileUtils.setMarignLeft(--startMargin);
                        }
                        fileUtils.writeContent("}");
                        fileUtils.blankLine();
                        
                        arch.archSpecificOperation(Constants.EARLY_TERMINATE_JUDGE);
                        fileUtils.setMarignLeft(--startMargin);
                    }
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent("for(; i_bd < subject_len; i_bd++) {");
                    {
                        fileUtils.setMarignLeft(++startMargin);
                        fileUtils.writeContent("int c = query[query_index];");
                        fileUtils.writeContent("cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);");
                        fileUtils.writeContent("cal_score(tmp_vector, D0, one, err);");
                        fileUtils.writeContent("move_peq(peq);");
                        fileUtils.writeContent("or_peq(tmp_peq, peq, bit_index, one, band_down);");
                        fileUtils.writeContent("bit_index++;");
                        fileUtils.writeContent("query_index++;");
                        fileUtils.setMarignLeft(--startMargin);
                    }
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                    
                    arch.archSpecificOperation(Constants.EARLY_TERMINATE_JUDGE);
                    
                    fileUtils.writeContent("for(; query_index < query_len; query_index++) {");
                    {
                        fileUtils.setMarignLeft(++startMargin);
                        fileUtils.writeContent("int c = query[query_index];");
                        fileUtils.writeContent("cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);");
                        fileUtils.writeContent("cal_score(tmp_vector, D0, one, err);");
                        fileUtils.writeContent("move_peq(peq);");
                        fileUtils.setMarignLeft(--startMargin);
                    }
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                    
                    fileUtils.setMarignLeft(--startMargin);
                }
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                
                fileUtils.writeContent("{");
                {
                    fileUtils.setMarignLeft(++startMargin);
                    fileUtils.writeContent(wordType + " min_err = err;");
                    fileUtils.writeContent("for(int i = 0; i <= last_bits; i++) {");
                    {
                        fileUtils.setMarignLeft(++startMargin);
                        fileUtils.writeContent(baseIntrinsics.opRightShift("tmp_vector", "VP", "i"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("tmp_vector", "one"));
                        fileUtils.writeContent(baseIntrinsics.opAdd("err", "tmp_vector"));
                        fileUtils.writeContent(baseIntrinsics.opRightShift("tmp_vector", "VN", "i"));
                        fileUtils.writeContent(baseIntrinsics.opAnd("tmp_vector", "one"));
                        fileUtils.writeContent(baseIntrinsics.opSubtract("err", "tmp_vector"));
                        fileUtils.writeContent(baseIntrinsics.opMin("min_err", "min_err", "err"));
                        fileUtils.setMarignLeft(--startMargin);
                    }
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                    
                    fileUtils.writeContent(String.format("%s * vec = ((%s *) & min_err);", resultType, resultType));
                    fileUtils.writeContent(String.format("for(int i = 0; i < %s; i++) {", vNumStr));
                    {
                        fileUtils.setMarignLeft(++startMargin);
                        fileUtils.writeContent("score[i + score_index] = vec[i];");
                        fileUtils.setMarignLeft(--startMargin);
                    }
                    fileUtils.writeContent("}");
                    fileUtils.blankLine();
                    fileUtils.setMarignLeft(--startMargin);
                }
                fileUtils.writeContent("}");
                fileUtils.blankLine();
                fileUtils.writeContent("end:;");
                fileUtils.setMarignLeft(--startMargin);
            }
            fileUtils.writeContent("}");
            fileUtils.blankLine();
            fileUtils.setMarignLeft(--startMargin);
            
        }
        fileUtils.writeContent("}");
        fileUtils.blankLine();
        fileUtils.setMarignLeft(--startMargin);
    }
    
    
}
