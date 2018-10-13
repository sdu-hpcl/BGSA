import org.junit.Test;
import org.sduhpcl.bgsa.util.GeneratorUtils;

/**
 * @author Jikai Zhang
 * @date 2018-01-08
 */
public class ComplementTest {
    
    @Test
    public void testIntToComplement() {
        int number = -5;
        System.out.println(GeneratorUtils.convertIntToComplementCode(number));
    }
}
