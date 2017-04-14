using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace TestClassLibrary
{
    public interface TestClassInterface
    {
        int AddAsd(string asd, ref string responseText);
        int return5();
        string returnHello();
        string echo(string msg);
    }
}
