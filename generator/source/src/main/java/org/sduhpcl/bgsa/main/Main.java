package org.sduhpcl.bgsa.main;

import gnu.getopt.Getopt;
import gnu.getopt.LongOpt;
import org.sduhpcl.bgsa.arch.*;
import org.sduhpcl.bgsa.generator.BandedMyersGenerator;
import org.sduhpcl.bgsa.generator.BaseGenerator;
import org.sduhpcl.bgsa.generator.BitPAlGenerator;
import org.sduhpcl.bgsa.generator.MyersGenerator;
import org.sduhpcl.bgsa.util.Configuration;
import org.sduhpcl.bgsa.util.FileUtils;
import org.sduhpcl.bgsa.util.GeneratorUtils;
import org.sduhpcl.bgsa.util.ScoreMsg;

import java.util.regex.Pattern;

/**
 * Created by zhangjikai on 16-9-7.
 */
public class Main {
    
    private static void printHelp() {
        StringBuilder builder = new StringBuilder();
        builder.append("Options:\n\n");
        builder.append(String.format("%-30s", "-M,--match <arg>:"));
        builder.append("Specify the match score which should be positive or zero. Default is 2.\n\n");
        builder.append(String.format("%-30s", "-I,--mismatch <arg>: "));
        builder.append("Specify the mismatch score which should be negative. Default is -3.\n\n");
        builder.append(String.format("%-30s", "-G,--gap <arg>: "));
        builder.append("Specify the gap score which should be negative. Default is -5.\n\n");
        builder.append(String.format("%-30s", "-d,--directory <arg>: "));
        builder.append("Specify the directory where to place generated source files.\n\n");
        builder.append(String.format("%-30s", "-n,--name <arg>: "));
        builder.append("Set the name of generated source file.\n\n");
        builder.append(String.format("%-30s", "-t,--type <arg>: "));
        builder.append("Set BitPAl algorithm type. Valid values are: non-packed, packed. Default is packed.\n\n");
        builder.append(String.format("%-30s", "-m,--myers: "));
        builder.append("Using Myers' algorithm. Valid values are: 0, 1. 0 presents weights(0, -1, -1) and 1 presents weights(0, 1, 1). Default is 0. \n\n");
        builder.append(String.format("%-30s", "-a,--arch: "));
        builder.append("Specify the SIMD architecture. Valid values are: none, sse, avx2, knc, avx512. Default is sse. If you want generate kernel source for multiple architectures, you can use '-' to join them as none-sse-avx2 \n\n");
        builder.append(String.format("%-30s", "-e,--element: "));
        builder.append("Specify vector element size. Valid value is 64, 32, 16, 8. Default is 32.\n\n");
        builder.append(String.format("%-30s", "-s,--semi: "));
        builder.append("Using semi-global algorithm.\n\n");
        builder.append(String.format("%-30s", "-b,--banded: "));
        builder.append("Using banded myers algorithm.\n\n");
        builder.append(String.format("%-30s", "-h,--help: "));
        builder.append("Display help Information.\n\n");
        System.out.println(builder.toString());
    }
    
    private static void handleArgs(String[] args) {
        LongOpt longOpts[] = new LongOpt[12];
        longOpts[0] = new LongOpt("help", LongOpt.NO_ARGUMENT, null, 'h');
        longOpts[1] = new LongOpt("directory", LongOpt.REQUIRED_ARGUMENT, null, 'd');
        longOpts[2] = new LongOpt("name", LongOpt.REQUIRED_ARGUMENT, null, 'n');
        longOpts[3] = new LongOpt("match", LongOpt.REQUIRED_ARGUMENT, null, 'M');
        longOpts[4] = new LongOpt("mismatch", LongOpt.REQUIRED_ARGUMENT, null, 'I');
        longOpts[5] = new LongOpt("gap", LongOpt.REQUIRED_ARGUMENT, null, 'G');
        longOpts[6] = new LongOpt("type", LongOpt.REQUIRED_ARGUMENT, null, 't');
        longOpts[7] = new LongOpt("myers", LongOpt.REQUIRED_ARGUMENT, null, 'm');
        longOpts[8] = new LongOpt("arch", LongOpt.REQUIRED_ARGUMENT, null, 'a');
        longOpts[9] = new LongOpt("semi", LongOpt.NO_ARGUMENT, null, 's');
        longOpts[10] = new LongOpt("banded", LongOpt.NO_ARGUMENT, null, 'b');
        longOpts[11] = new LongOpt("element", LongOpt.NO_ARGUMENT, null, 'e');
        
        int c;
        int num;
        String arg;
        Getopt opt = new Getopt("bitpal", args, "M:I:G:d:hn:t:m:a:e:sb", longOpts);
        
        while ((c = opt.getopt()) != -1) {
            switch (c) {
                case 'M':
                    arg = opt.getOptarg();
                    try {
                        num = Integer.parseInt(arg);
                        Configuration.matchScore = num;
                    } catch (Exception e) {
                        System.err.println("-M: You should input an integer.");
                        System.exit(-1);
                    }
                    break;
                
                case 'I':
                    arg = opt.getOptarg();
                    try {
                        num = Integer.parseInt(arg);
                        Configuration.mismatchScore = num;
                    } catch (Exception e) {
                        System.err.println("-I: You should input an integer.");
                        System.exit(-1);
                    }
                    break;
                
                case 'G':
                    arg = opt.getOptarg();
                    try {
                        num = Integer.parseInt(arg);
                        Configuration.indelScore = num;
                    } catch (Exception e) {
                        System.err.println("-G: You should input an integer.");
                        System.exit(-1);
                    }
                    break;
                
                case 'd':
                    arg = opt.getOptarg();
                    Configuration.outputDir = arg;
                    break;
                case 'h':
                    printHelp();
                    System.exit(-1);
                    break;
                case 'n':
                    arg = opt.getOptarg();
                    Configuration.outputFileName = arg;
                    break;
                case 't':
                    arg = opt.getOptarg();
                    
                    if ("non-packed".equals(arg)) {
                        Configuration.isPacked = false;
                    }
                    
                    break;
                case 'm':
                    Configuration.isMyers = true;
                    arg = opt.getOptarg();
                    try {
                        num = Integer.parseInt(arg);
                        if (num == 0) {
                            Configuration.factor = -1;
                        } else {
                            Configuration.factor = 1;
                        }
                    } catch (Exception e) {
                        System.err.println("-m: You should input an integer.");
                        System.exit(-1);
                    }
                    break;
                case 'a':
                    arg = opt.getOptarg();
                    
                    if (arg != null && !"".equals(arg)) {
                        String[] arches = arg.split("-");
                        for (String arch : arches) {
                            switch (arch) {
                                case "none":
                                    Configuration.archTypes.add(Configuration.ArchType.NONE);
                                    break;
                                case "sse":
                                    Configuration.archTypes.add(Configuration.ArchType.SSE);
                                    break;
                                case "avx2":
                                    Configuration.archTypes.add(Configuration.ArchType.AVX2);
                                    break;
                                case "knc":
                                    Configuration.archTypes.add(Configuration.ArchType.KNC);
                                    break;
                                case "avx512":
                                    Configuration.archTypes.add(Configuration.ArchType.AVX512);
                                    break;
                                default:
                                    Configuration.archTypes.add(Configuration.ArchType.SSE);
                                    break;
                            }
                        }
                        
                    }
                    break;
                case 'e':
                    arg = opt.getOptarg();
                    try {
                        num = Integer.parseInt(arg);
                        switch (num) {
                            case 8:
                                Configuration.elementType = Configuration.ElementType.E8;
                                break;
                            case 16:
                                Configuration.elementType = Configuration.ElementType.E16;
                                break;
                            case 64:
                                Configuration.elementType = Configuration.ElementType.E64;
                                break;
                            default:
                                Configuration.elementType = Configuration.ElementType.E32;
                        }
                    } catch (Exception e) {
                        System.err.println("-e: You should input an integer.");
                        System.exit(-1);
                    }
                    break;
                case 's':
                    Configuration.isSemiGlobal = true;
                    break;
                case 'b':
                    Configuration.isBanded = true;
                    break;
                case '?':
                    System.err.println("The option '" + (char) opt.getOptopt() + "' is not valid");
                    printHelp();
                    System.exit(-1);
                    break;
                default:
                    printHelp();
                    System.exit(-1);
                    break;
            }
        }
    }
    
    public static int commonFactor(int matchScore, int mismatchScore, int indelScore) {
        int factor = 1;
        /*if(matchScore == 0) {
            return factor;
        }*/
        
        mismatchScore = Math.abs(mismatchScore);
        indelScore = Math.abs(indelScore);
        
        int minScore = matchScore;
        if (matchScore == 0) {
            minScore = mismatchScore;
        }
        if (mismatchScore < minScore) {
            minScore = mismatchScore;
        }
        if (indelScore < minScore) {
            minScore = indelScore;
        }
        for (int i = 2; i <= minScore; i++) {
            if ((matchScore % i == 0) && (mismatchScore % i == 0) && (indelScore % i == 0)) {
                factor = i;
            }
        }
        return factor;
    }
    
    public static void main(String[] args) {
        if (args.length == 0) {
            printHelp();
            System.exit(-1);
        }
        handleArgs(args);
        
        FileUtils.newInstance(Configuration.outputDir, Configuration.outputFileName);
        
        if (Configuration.archTypes.size() == 0) {
            Configuration.archTypes.add(Configuration.ArchType.SSE);
        }
        
        if (Configuration.isMyers || Configuration.isBanded) {
            Configuration.matchScore = 0;
            Configuration.mismatchScore = Configuration.factor;
            Configuration.indelScore = Configuration.factor;
        }
        
        if (!Configuration.isMyers) {
            Configuration.factor = commonFactor(Configuration.matchScore, Configuration.mismatchScore, Configuration.indelScore);
        }
        
        if (Configuration.factor != 1 && !Configuration.isMyers) {
            Configuration.matchScore = Configuration.matchScore / Configuration.factor;
            Configuration.mismatchScore = Configuration.mismatchScore / Configuration.factor;
            Configuration.indelScore = Configuration.indelScore / Configuration.factor;
        }
        
        
        if (!Configuration.isPacked && Configuration.matchScore == 0 && Configuration.mismatchScore == -1 && Configuration.indelScore == -1) {
            Configuration.isEdit = true;
        }
        
        ScoreMsg.calValues();
        
        GeneratorUtils.genBitIncluded();
        GeneratorUtils.genBitGlobal();
        
        BaseGenerator generator;
        
        if (Configuration.isMyers) {
            generator = new MyersGenerator();
        } else {
            generator = new BitPAlGenerator();
        }
        
        if(Configuration.isBanded) {
            generator = new BandedMyersGenerator();
        }
        
        if (Configuration.archTypes.contains(Configuration.ArchType.NONE)) {
            generator.setArch(new CPUArch());
            generator.genKernel();
        }
        if (Configuration.archTypes.contains(Configuration.ArchType.SSE)) {
            generator.setArch(new SSEArch());
            generator.genKernel();
        }
        if (Configuration.archTypes.contains(Configuration.ArchType.AVX2)) {
            generator.setArch(new AVX2Arch());
            generator.genKernel();
        }
        if (Configuration.archTypes.contains(Configuration.ArchType.KNC)) {
            generator.setArch(new KNCArch());
            generator.genKernel();
        }
        
        if (Configuration.archTypes.contains(Configuration.ArchType.AVX512)) {
            generator.setArch(new KNLArch());
            generator.genKernel();
        }
        
        System.out.println("The file is saved at: " + Configuration.outputDir + System.getProperty("file.separator") + Configuration.outputFileName);
        
    }
}
