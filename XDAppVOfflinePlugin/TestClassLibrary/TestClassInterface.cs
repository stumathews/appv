using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace TestClassLibrary
{
    /* Interface for COM exposure */
    public interface TestClassInterface
    {
        string SendToAppVService(string payload);

        /* Test old legacy interface -to to be removed */
        int AddAsd(string asd);
        int return5();
        string returnHello();
        string echo(string msg);
    }
}
