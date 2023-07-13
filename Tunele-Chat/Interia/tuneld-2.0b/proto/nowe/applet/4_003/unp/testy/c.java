public class c {
    public static void main(String args[]) {
	System.out.println("Test");    
	System.out.println("login("+a("PX\001!")+",("+a("NP\0219")+")");
	System.out.println("1:"+c("u\037#\r\027y\004\"7\fv\006.\034\0")
        );
	}
//    login(getParameter(a("PX\001!")), getParameter(a("NP\0219")));
    private static String a(String s1)
    {
        char ac[] = s1.toCharArray();
        int i1 = ac.length;
        int k1;
        for(int j1 = 0; j1 < i1; j1++)
        {
            switch(j1 % 5)
            {
            case 0: // '\0'
                k1 = 0x3e;
                break;

            case 1: // '\001'
                k1 = 49;
                break;

            case 2: // '\002'
                k1 = 98;
                break;

            case 3: // '\003'
                k1 = 74;
                break;

            default:
                k1 = 39;
                break;
            }
            ac[j1] ^= k1;
        }

        return new String(ac);
    }

private static String h(String s1)
    {
        char ac[] = s1.toCharArray();
        int i1 = ac.length;
        int k1;
        for(int j1 = 0; j1 < i1; j1++)
        {
            switch(j1 % 5)
            {
            case 0: // '\0'
                k1 = 0x45;
                break;

            case 1: // '\001'
                k1 = 97;
                break;

            case 2: // '\002'
                k1 = 23;
                break;

            case 3: // '\003'
                k1 = 111;
                break;

            default:
                k1 = 47;
                break;
            }
            ac[j1] ^= k1;
        }

        return new String(ac);
    }


    private static String b(String s1)
    {
        char ac[] = s1.toCharArray();
        int i1 = ac.length;
        int k1;
        for(int j1 = 0; j1 < i1; j1++)
        {
            switch(j1 % 5)
            {
            case 0: // '\0'
                k1 = 0x1f;
                break;

            case 1: // '\001'
                k1 = 86;
                break;

            case 2: // '\002'
                k1 = 42;
                break;

            case 3: // '\003'
                k1 = 15;
                break;

            default:
                k1 = 53;
                break;
            }
            ac[j1] ^= k1;
        }

        return new String(ac);
    }




    private static String c(String s1)
    {
        char ac[] = s1.toCharArray();
        int i1 = ac.length;
        int k1;
        for(int j1 = 0; j1 < i1; j1++)
        {
            switch(j1 % 5)
            {
            case 0: // '\0'
                k1 = 0x18;
                break;

            case 1: // '\001'
                k1 = 112;
                break;

            case 2: // '\002'
                k1 = 71;
                break;

            case 3: // '\003'
                k1 = 104;
                break;

            default:
                k1 = 101;
                break;
            }
            ac[j1] ^= k1;
        }

        return new String(ac);
    }

}
